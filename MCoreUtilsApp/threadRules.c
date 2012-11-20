/********************************************//**
 * @file
 * @brief Rule-based modification of thread real-time properties.
 * @author Ralph Lange <Ralph.Lange@gmx.de>
 * @copyright
 * Copyright (c) 2012 ITER Organization
 * @copyright
 * Distributed subject to the EPICS_BASE Software License Agreement found
 * in the file LICENSE that is included with this distribution.
 ***********************************************/

/**
 * @file
 *
 * @ingroup threadrules
 * @{
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <regex.h>
#include <string.h>

#include <ellLib.h>
#include <envDefs.h>
#include <errlog.h>
#include <epicsStdio.h>
#include <epicsMath.h>
#include <epicsThread.h>
#include <epicsMutex.h>
#include <shareLib.h>

#include "utils.h"

/// @cond NEVER
#define epicsExportSharedSymbols
/// @endcond
#include "mcoreutils.h"

/// @cond NEVER
extern int epicsThreadGetPosixPriority(epicsThreadOSD *pthreadInfo);
/// @endcond

/**
 * @brief A thread rule.
 *
 * Used to manipulate real-time properties when threads are started.
 * The thread rules are kept in a linked list.
 */
typedef struct threadRule {
    ELLNODE     node;           ///< linked list node
    char       *name;           ///< rule name
    char       *pattern;        ///< regex pattern (string)
    char       *cpus;           ///< cpu set (string)
    regex_t     reg;            ///< regex pattern (compiled)
    char        ch_policy;      ///< flag: change policy
    char        ch_priority;    ///< flag: change priority
    char        ch_affinity;    ///< flag: change affinity
    char        rel_priority;   ///< flag: priority is relative
    int         policy;         ///< policy value
    int         priority;       ///< priority value
    cpu_set_t   cpuset;         ///< cpuset (opaque)
} threadRule;

static ELLLIST threadRules = ELLLIST_INIT;
static epicsMutexId listLock;
static unsigned int cpuspecLen;
static char *sysConfigFile      = "/etc/rtrules";
static ENV_PARAM userHome       = {"HOME","/"};
static ENV_PARAM userConfigFile = {"EPICS_MCORE_USERCONFIG",".rtrules"};

/**
 * @brief Parse the property modifiers into a thread rule.
 *
 * @param prule    rule to set
 * @param policy   scheduling policy to set (* = don't change)
 * @param priority scheduling priority (OSI) to set (* = don't change)
 * @param cpus     cpuset specification to set (* = don't change)
 */
static void parseModifiers(threadRule *prule, const char *policy, const char *priority, const char *cpus)
{
    if (policy && '*' != policy[0] && '\0' != policy[0]) {
        prule->ch_policy = 1;
        prule->policy = strToPolicy(policy);
        if (-1 == prule->policy) {
            prule->ch_policy = prule->policy = 0;
        }
    }
    if (priority && '*' != priority[0] && '\0' != priority[0]) {
        prule->ch_priority = 1;
        if ('+' == priority[0] || '-' == priority[0]) {
            prule->rel_priority = 1;
        }
        prule->priority = atoi(priority);
        if (prule->priority > epicsThreadPriorityMax) prule->priority = epicsThreadPriorityMax;
        if (prule->priority < epicsThreadPriorityMin) prule->priority = epicsThreadPriorityMin;
    }
    if (cpus && '*' != cpus[0] && '\0' != cpus[0]) {
        prule->ch_affinity = 1;
        strToCpuset(&prule->cpuset, cpus);
    }
}

/**
 * @brief Add or replace a thread rule.
 */
long mcoreThreadRuleAdd(const char *name, const char *policy, const char *priority, const char *cpus, const char *pattern)
{
    threadRule *prule;

    prule = calloc(1,sizeof(threadRule));
    if (!prule) {
        errlogPrintf("Memory allocation error\n");
        return -1;
    }

    prule->name    = strdup(name);
    prule->pattern = strdup(pattern);
    prule->cpus    = strdup(cpus);
    if (!prule->name || !prule->pattern || !prule->cpus) {
        errlogPrintf("Memory allocation error\n");
        return -1;
    }

    parseModifiers(prule, policy, priority, cpus);
    regcomp(&prule->reg, prule->pattern, (REG_EXTENDED || REG_NOSUB));

    epicsMutexLock(listLock);
    mcoreThreadRuleDelete(name);
    ellAdd(&threadRules, &prule->node);
    epicsMutexUnlock(listLock);
    return 0;
}

/**
 * @brief Delete a thread rule.
 */
void mcoreThreadRuleDelete(const char *name)
{
    threadRule *prule;

    epicsMutexLock(listLock);
    prule = (threadRule *) ellFirst(&threadRules);
    while (prule) {
        if (0 == strcmp(name, prule->name)) {
            ellDelete(&threadRules, &prule->node);
            epicsMutexUnlock(listLock);
            free(prule->name);
            free(prule->pattern);
            free(prule->cpus);
            regfree(&prule->reg);
            free(prule);
            return;
        }
        prule = (threadRule *) ellNext(&prule->node);
    }
    epicsMutexUnlock(listLock);
}

/**
 * @brief Print a comprehensive list of the thread rules.
 */
void mcoreThreadRulesShow(void)
{
    threadRule *prule;
    const int buflen = 128; //FIXME should be ~ NO_OF_CPUS
    char buf[buflen];

    epicsMutexLock(listLock);
    prule = (threadRule *) ellFirst(&threadRules);
    if (!prule) {
        fprintf(epicsGetStdout(), "No rules defined.\n");
        epicsMutexUnlock(listLock);
        return;
    }
    fprintf(epicsGetStdout(), "            NAME  PRIO POLICY %-*s PATTERN\n", cpuspecLen, "AFFINITY");
    while (prule) {
        cpusetToStr(buf, buflen, &prule->cpuset);
        fprintf(epicsGetStdout(), "%16s  ",
                prule->name);
        if (prule->ch_priority) {
            if (prule->rel_priority) {
                fprintf(epicsGetStdout(), "%+4d ", prule->priority);
            } else {
                fprintf(epicsGetStdout(), "%4d ", prule->priority);
            }
        } else {
            fprintf(epicsGetStdout(), "   * ");
        }
        fprintf(epicsGetStdout(),"%6s %-*s %s\n",
                prule->ch_policy?policyToStr(prule->policy):"*",
                cpuspecLen, prule->ch_affinity?buf:"*",
                prule->pattern
                );
        prule = (threadRule *) ellNext(&prule->node);
    }
    epicsMutexUnlock(listLock);
}

/**
 * @brief Modify a thread's real-time properties according to the specified thread rule.
 *
 * @param id EPICS thread id
 * @param prule thread rule to use
 */
static void modifyRTProperties(epicsThreadId id, threadRule *prule)
{
    int status;
    unsigned int priority;

    if (prule->ch_policy || prule->ch_priority) {
        status = pthread_attr_getschedparam(&id->attr, &id->schedParam);
        if (errVerbose)
            checkStatus(status,"pthread_attr_getschedparam");
        status = pthread_attr_getschedpolicy(&id->attr, &id->schedPolicy);
        if (errVerbose)
            checkStatus(status,"pthread_attr_getschedpolicy");

        if (prule->ch_policy) {
            id->schedPolicy = prule->policy;
            status = pthread_attr_setschedpolicy(&id->attr, id->schedPolicy);
            if (errVerbose)
                checkStatus(status,"pthread_attr_setschedpolicy");
            if (SCHED_FIFO == prule->policy || SCHED_RR == prule->policy) {
                id->isRealTimeScheduled = 1;
            } else {
                id->isRealTimeScheduled = 0;
            }
        }

        if (prule->ch_priority) {
            if (prule->rel_priority) {
                priority = id->osiPriority + prule->priority;
                if (priority > epicsThreadPriorityMax) priority = epicsThreadPriorityMax;
                if (priority < epicsThreadPriorityMin) priority = epicsThreadPriorityMin;
            } else {
                priority = prule->priority;
            }
            id->osiPriority = priority;
            id->schedParam.sched_priority = epicsThreadGetPosixPriority(id);
            status = pthread_attr_setschedparam(&id->attr, &id->schedParam);
            if (errVerbose)
                checkStatus(status,"pthread_attr_setschedparam");
        }

        status = pthread_setschedparam(id->tid, id->schedPolicy, &id->schedParam);
        if (errVerbose)
            checkStatus(status,"pthread_setschedparam");
    }

    if (prule->ch_affinity) {
        status = pthread_attr_setaffinity_np(&id->attr,
                                             sizeof(cpu_set_t),
                                             &prule->cpuset);
        if (errVerbose)
            checkStatus(status,"pthread_attr_setaffinity_np");
        status = pthread_setaffinity_np(id->tid,
                                        sizeof(cpu_set_t),
                                        &prule->cpuset);
        if (errVerbose)
            checkStatus(status,"pthread_setaffinity_np");
    }
}

static void threadStartHook (epicsThreadId id)
{
    threadRule *prule;

    epicsMutexLock(listLock);
    prule = (threadRule *) ellFirst(&threadRules);
    if (!prule) {
        epicsMutexUnlock(listLock);
        return;
    }
    while (prule) {
        if (0 == regexec(&prule->reg, id->name, 0, NULL, 0)) {
            modifyRTProperties(id, prule);
        }
        prule = (threadRule *) ellNext(&prule->node);
    }
    epicsMutexUnlock(listLock);
}

/**
 * @brief Modify a thread's real-time properties.
 */
void mcoreThreadModify(epicsThreadId id, const char *policy, const char *priority, const char *cpus)
{
    threadRule rule;

    assert(id);
    parseModifiers(&rule, policy, priority, cpus);
    modifyRTProperties(id, &rule);
}

/**
 * @brief Read a set of thread rules from a file.
 *
 * @param file
 * @return number of rules read
 */
static int readRulesFromFile(const char *file)
{
    const int linelen = 256;
    const char sep = ':';
    char line[linelen];
    unsigned int lineno = 0;
    char *args[5];           // rtgroups format -- name:policy:priority:affinity:pattern
    int count = 0;

    FILE *fp = fopen(file, "r");
    if (NULL == fp) {
        if (errVerbose)
            errlogPrintf("mcoreThreadRules: can't open rules file %s\n", file);
    } else {
        while (fgets(line, linelen, fp)) {
            int i;
            char *sp;
            char *cp;
            lineno++;
            args[0] = cp = line;
            cp += strspn(cp, " \t\r\n");   // trim leading whitespace and empty lines
            if (*cp == '#' || *cp == '\0')
                continue;
            for (i = 1; i < 5; i++) {
                sp = strchr(cp, sep);
                if (!sp) {
                    errlogPrintf("mcoreThreadRules: error parsing line %d of file %s\n", lineno, file);
                    return count;
                }
                *sp++ = '\0';
                args[i] = cp = sp;
            }
            if ((sp = strpbrk(cp, "\n\r"))) {
                *sp = '\0';
            }
            mcoreThreadRuleAdd(args[0], args[1], args[2], args[3], args[4]);
            count++;
        }
        fclose (fp);
    }
    return count;
}

static void once(void *arg)
{
    const int len = 256;
    char userFile[len];
    char userRel[len];
    int count;

    cpuspecLen = (int) (log10(NO_OF_CPUS-1) + 2) * NO_OF_CPUS / 2;
    if (cpuspecLen < 10)
        cpuspecLen = 10;
    listLock = epicsMutexMustCreate();

    envGetConfigParam(&userHome, sizeof(userFile), userFile);
    envGetConfigParam(&userConfigFile, sizeof(userRel), userRel);
    if (userFile[strlen(userFile)-1] != '/')
        strcat(userFile, "/");
    strncat(userFile, userRel, len-strlen(userFile)-1);

    count = readRulesFromFile(sysConfigFile);
    printf("MCoreUtils: Read %d thread rule(s) from %s\n", count, sysConfigFile);

    count = readRulesFromFile(userFile);
    printf("MCoreUtils: Read %d thread rule(s) from %s\n", count, userFile);

    epicsThreadHookAdd(threadStartHook);
}

/**
 * @brief Initialization routine.
 */
void mcoreThreadRulesInit(void)
{
    static epicsThreadOnceId onceFlag = EPICS_THREAD_ONCE_INIT;
    epicsThreadOnce(&onceFlag, once, NULL);
}

/**
 *@}
 */
