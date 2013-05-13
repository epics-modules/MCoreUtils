/********************************************//**
 * @file
 * @brief New threadShow showing real-time properties.
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
 * @ingroup threadshow
 * @{
 */

#include <stdlib.h>
#include <sched.h>
#include <string.h>
#include <pthread.h>

#include <ellLib.h>
#include <errlog.h>
#include <epicsStdio.h>
#include <epicsEvent.h>
#include <epicsThread.h>
#include <epicsMath.h>
#include <shareLib.h>

#include "utils.h"

/// @cond NEVER
#define epicsExportSharedSymbols
/// @endcond
#include "mcoreutils.h"

static epicsThreadId showThread;
static unsigned int showLevel;
static char *buffer;
static char *cpuspec;
static const char *policies;

/**
 * @brief Print one line of thread info.
 *
 * @param pthreadInfo thread id to print line for, NULL = print header line
 * @param level       verbosity level (unused)
 */
static void mcoreThreadShowPrint(epicsThreadOSD *pthreadInfo, unsigned int level)
{
    if (!pthreadInfo) {
        fprintf(epicsGetStdout(), "            NAME       EPICS ID   "
            "LWP ID   OSIPRI  OSSPRI  STATE  POLICY CPUSET\n");
    } else {
        struct sched_param param;
        int priority = 0;
        int policy;

        policies = "?";
        cpuspec[0] = '?'; cpuspec[1] = '\0';
        if (pthreadInfo->tid) {
            cpu_set_t cpuset;
            int status;
            status = pthread_getschedparam(pthreadInfo->tid,
                                           &policy,
                                           &param);
            if (errVerbose)
                checkStatus(status,"pthread_getschedparam");

            if (!status) {
                priority = param.sched_priority;
                policies = policyToStr(policy);
            }

            status = pthread_getaffinity_np(pthreadInfo->tid,
                                            sizeof(cpu_set_t),
                                            &cpuset);
            if (!status) {
                cpusetToStr(cpuspec, NO_OF_CPUS+2, &cpuset);
            }
        }

        fprintf(epicsGetStdout(),"%16.16s %14p %8lu    %3d%8d %8.8s %7.7s %s\n",
                pthreadInfo->name,
                (void *)pthreadInfo,
                (unsigned long)pthreadInfo->lwpId,
                pthreadInfo->osiPriority, priority,
                pthreadInfo->isSuspended ? "SUSPEND" : "OK",
                policies, cpuspec);
    }
}

/**
 * @brief Calls @c mcoreThreadShowPrint() on the given id using the global verbosity level.
 *
 * @param id id of thread to print information for.
 */
static void mcoreThreadInfo(epicsThreadId id)
{
    mcoreThreadShowPrint(id, showLevel);
}

/**
 * @brief Map callback for showing one thread.
 *
 * Compares id to global thread, and calls @c mcoreThreadInfo() on match.
 * @param id current thread (map argument)
 */
static void mcoreThreadInfoOne(epicsThreadId id)
{
    intptr_t u = id->lwpId;
    if (id == showThread || (epicsThreadId) u == showThread) {
        mcoreThreadInfo(id);
    }
}

/**
 * @brief Show thread info for one thread.
 */
void mcoreThreadShow(epicsThreadId thread, unsigned int level)
{
    if (!thread) {
        mcoreThreadShowPrint(0, level);
        return;
    }
    showThread = thread;
    showLevel = level;
    epicsThreadMap(mcoreThreadInfoOne);
}

/**
 * @brief Show thread info for all threads.
 */
void mcoreThreadShowAll(unsigned int level)
{
    showLevel = level;
    mcoreThreadShowPrint(0, level);
    epicsThreadMap(mcoreThreadInfo);
}

static void once(void *arg)
{
    cpuDigits = (int) log10(NO_OF_CPUS-1) + 1;
    if (!buffer)  buffer  = (char *) calloc(cpuDigits+2, sizeof(char));
    if (!cpuspec) cpuspec = (char *) calloc(NO_OF_CPUS + 2, sizeof(char));
    printf("MCoreUtils version " VERSION "\n");
}

/**
 * @brief Initialization routine.
 */
void mcoreThreadShowInit(void)
{
    static epicsThreadOnceId onceFlag = EPICS_THREAD_ONCE_INIT;
    epicsThreadOnce(&onceFlag, once, NULL);
}

/**
 *@}
 */
