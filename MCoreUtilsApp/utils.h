/********************************************//**
 * @file
 * @brief Header file for utils.c
 * @author Ralph Lange <Ralph.Lange@gmx.de>
 * @copyright
 * Copyright (c) 2012 ITER Organization
 * @copyright
 * Distributed subject to the EPICS_BASE Software License Agreement found
 * in the file LICENSE that is included with this distribution.
 ***********************************************/

#ifndef UTILS_H
#define UTILS_H

#include <sched.h>
#include <unistd.h>

#include <errlog.h>

// TODO: Use libCom call when get-cpus branch is merged
#define NO_OF_CPUS sysconf(_SC_NPROCESSORS_CONF)

#define checkStatus(status,message) \
if((status))  {\
    errlogPrintf("%s error %s\n", (message), strerror((status))); \
}

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Number of digits needed for a single CPU spec.
 *
 * Set in mcoreThreadShowInit().
 */
extern int cpuDigits;

void strToCpuset(cpu_set_t *cpuset, const char *spec);
void cpusetToStr(char *set, size_t len, const cpu_set_t *cpuset);
const char *policyToStr(const int policy);
int strToPolicy(const char *string);

#ifdef __cplusplus
}
#endif

#endif // UTILS_H
