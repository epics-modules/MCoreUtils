/********************************************//**
 * @file
 * @brief Utility functions for MCoreUtils.
 * @author Ralph Lange <Ralph.Lange@gmx.de>
 * @copyright
 * Copyright (c) 2012 ITER Organization
 * @copyright
 * Distributed subject to the EPICS_BASE Software License Agreement found
 * in the file LICENSE that is included with this distribution.
 ***********************************************/

#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <string.h>

#include <errlog.h>

#include "utils.h"

epicsShareDef int cpuDigits;

/**
 * @brief Convert a cpuset string specification (e.g. "0,2-3") to a cpuset.
 *
 * @param cpuset cpuset to write into
 * @param spec   specification string
 */
void strToCpuset(cpu_set_t *cpuset, const char *spec)
{
    char *buff = strdup(spec);
    char *tok, *save;

    CPU_ZERO(cpuset);

    tok = strtok_r(buff, ",", &save);
    while (tok) {
        int i;
        int from, to;
        from = to = atoi(tok);
        char *sep = strstr(tok, "-");
        if (sep) {
            to = atoi(sep+1);
        }
        for (i = from; i <= to; i++) {
            CPU_SET(i, cpuset);
        }
        tok = strtok_r(NULL, ",", &save);
    }
}

/**
 * @brief Convert a cpuset into its string specification (e.g. "0,2-3").
 *
 * @param set    output buffer to write into
 * @param len    length of @p set
 * @param cpuset cpuset to convert
 */
void cpusetToStr(char *set, size_t len, const cpu_set_t *cpuset)
{
    int cpu = 0;
    int from, to, l;
    char buf[cpuDigits*2+3];

    if (!set || !len) return;
    set[0] = '\0';
    while (cpu < NO_OF_CPUS) {
        while (!CPU_ISSET(cpu, cpuset) && cpu < NO_OF_CPUS) {
            cpu++;
        }
        if (cpu >= NO_OF_CPUS) {
            break;
        }
        from = to = cpu++;
        while (CPU_ISSET(cpu, cpuset) && cpu < NO_OF_CPUS) {
            to = cpu++;
        }
        if (from == to) {
            sprintf(buf, "%d,", from);
        } else {
            sprintf(buf, "%d-%d,", from, to);
        }
        strncat(set, buf, (len - 1 - strlen(set)));
    }
    if ((l = strlen(set))) {
        set[l-1] = '\0';
    }
}

/**
 * @brief Convert scheduling policy to string.
 *
 * @param policy policy to convert
 * @return string representation
 */
const char *policyToStr(const int policy)
{
    switch (policy) {
    case SCHED_OTHER:
        return "OTHER";
    case SCHED_FIFO:
        return "FIFO";
    case SCHED_RR:
        return "RR";
#ifdef SCHED_BATCH
    case SCHED_BATCH:
        return "BATCH";
#endif /* SCHED_BATCH */
#ifdef SCHED_IDLE
    case SCHED_IDLE:
        return "IDLE";
#endif /* SCHED_IDLE */
    default:
        return "?";
    }
}

/**
 * @brief Convert string policy specification to policy.
 *
 * @param string string policy specification
 * @return policy value, or -1 on error
 */
int strToPolicy(const char *string)
{
    int policy = -1;
    if (string == strcasestr(string, "SCHED_")) {
        string += 6;
    }
    if (0 == strncasecmp(string, "OTHER", 1)) {
        policy = SCHED_OTHER;
    } else if (0 == strncasecmp(string, "FIFO", 1)) {
        policy = SCHED_FIFO;
    } else if (0 == strncasecmp(string, "RR", 1)) {
        policy = SCHED_RR;
    }
#ifdef SCHED_BATCH
    else if (0 == strncasecmp(string, "BATCH", 1)) {
        policy = SCHED_BATCH;
    }
#endif /* SCHED_BATCH */
#ifdef SCHED_IDLE
    else if (0 == strncasecmp(string, "IDLE", 1)) {
        policy = SCHED_IDLE;
    }
#endif /* SCHED_IDLE */
    else {
        errlogPrintf("Invalid policy \"%s\"\n", string);
        return -1;
    }
    return policy;
}
