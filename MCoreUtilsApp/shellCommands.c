/********************************************//**
 * @file
 * @brief iocShell registration of MCoreUtils commands.
 * @author Ralph Lange <Ralph.Lange@gmx.de>
 * @copyright
 * Copyright (c) 2012,2015 ITER Organization
 * @copyright
 * Distributed subject to the EPICS_BASE Software License Agreement found
 * in the file LICENSE that is included with this distribution.
 ***********************************************/

#include <unistd.h>
#include <stdlib.h>

#include <iocsh.h>
#include <epicsExport.h>
#include <epicsThread.h>

#include "mcoreutils.h"

/**
 * @brief Get thread id for string (name or id)
 * @param thread string containing name or id of thread
 * @return EPICS thread id or 0 (no such thread)
 */
static epicsThreadId getThreadIdFor(const char* thread) {
    const char *cp;
    epicsThreadId tid;
    unsigned long ltmp;
    char *endp;

    if (!thread) return 0;
    cp = thread;
    ltmp = strtoul(cp, &endp, 0);
    if (*endp) {
        tid = epicsThreadGetId(cp);
        if (!tid) {
            printf("*** %s is not a valid thread name ***\n", cp);
        }
    }
    else {
        tid = (epicsThreadId) ltmp;
    }
    return tid;
}

static const iocshArg mcoreThreadShowArg0 = {"thread", iocshArgString};
static const iocshArg mcoreThreadShowArg1 = {"level", iocshArgInt};
static const iocshArg *const mcoreThreadShowArgs[] = {
    &mcoreThreadShowArg0,
    &mcoreThreadShowArg1,
};
static const iocshFuncDef mcoreThreadShowDef =
    {"mcoreThreadShow", 2, mcoreThreadShowArgs};
static void mcoreThreadShowCall(const iocshArgBuf * args) {
    epicsThreadId tid;
    unsigned int level = args[1].ival;
    if (!args[0].sval) {
        printf("Missing argument\nUsage: mcoreThreadShow thread [level]\n");
        return;
    }
    tid = getThreadIdFor(args[0].sval);
    if (tid) {
        mcoreThreadShow(  0, level);
        mcoreThreadShow(tid, level);
    }
}

static const iocshArg mcoreThreadShowAllArg0 = {"level", iocshArgInt};
static const iocshArg *const mcoreThreadShowAllArgs[] = {
    &mcoreThreadShowAllArg0,
};
static const iocshFuncDef mcoreThreadShowAllDef =
    {"mcoreThreadShowAll", 1, mcoreThreadShowAllArgs};
static void mcoreThreadShowAllCall(const iocshArgBuf * args) {
    unsigned int level = args[0].ival;
    mcoreThreadShowAll(level);
}

static const iocshArg mcoreThreadRuleAddArg0 = {"name", iocshArgString};
static const iocshArg mcoreThreadRuleAddArg1 = {"policy", iocshArgString};
static const iocshArg mcoreThreadRuleAddArg2 = {"priority", iocshArgString};
static const iocshArg mcoreThreadRuleAddArg3 = {"cpuset", iocshArgString};
static const iocshArg mcoreThreadRuleAddArg4 = {"pattern", iocshArgString};
static const iocshArg *const mcoreThreadRuleAddArgs[] = {
    &mcoreThreadRuleAddArg0,
    &mcoreThreadRuleAddArg1,
    &mcoreThreadRuleAddArg2,
    &mcoreThreadRuleAddArg3,
    &mcoreThreadRuleAddArg4,
};
static const iocshFuncDef mcoreThreadRuleAddDef =
    {"mcoreThreadRuleAdd", 5, mcoreThreadRuleAddArgs};
static void mcoreThreadRuleAddCall(const iocshArgBuf * args) {
    int i;
    char missing = 0;
    for (i=0; i<5; i++) {
        if (NULL == args[i].sval) missing |= 1;
    }
    if (missing) {
        printf("Missing argument\nUsage: mcoreThreadRuleAdd name policy priority cpuset pattern\n");
        return;
    }
    mcoreThreadRuleAdd(args[0].sval, args[1].sval, args[2].sval, args[3].sval, args[4].sval);
}

static const iocshArg mcoreThreadRuleDeleteArg0 = {"name", iocshArgString};
static const iocshArg *const mcoreThreadRuleDeleteArgs[] = {
    &mcoreThreadRuleDeleteArg0,
};
static const iocshFuncDef mcoreThreadRuleDeleteDef =
    {"mcoreThreadRuleDelete", 1, mcoreThreadRuleDeleteArgs};
static void mcoreThreadRuleDeleteCall(const iocshArgBuf * args) {
    if (NULL == args[0].sval) {
        printf("Missing argument\nUsage: mcoreThreadRuleDelete name\n");
        return;
    }
    mcoreThreadRuleDelete(args[0].sval);
}

static const iocshFuncDef mcoreThreadRulesShowDef =
    {"mcoreThreadRulesShow", 0, NULL};
static void mcoreThreadRulesShowCall(const iocshArgBuf * args) {
    mcoreThreadRulesShow();
}

static const iocshArg mcoreThreadModifyArg0 = {"thread", iocshArgString};
static const iocshArg mcoreThreadModifyArg1 = {"policy", iocshArgString};
static const iocshArg mcoreThreadModifyArg2 = {"priority", iocshArgString};
static const iocshArg mcoreThreadModifyArg3 = {"cpuset", iocshArgString};
static const iocshArg *const mcoreThreadModifyArgs[] = {
    &mcoreThreadModifyArg0,
    &mcoreThreadModifyArg1,
    &mcoreThreadModifyArg2,
    &mcoreThreadModifyArg3,
};
static const iocshFuncDef mcoreThreadModifyDef =
    {"mcoreThreadModify", 4, mcoreThreadModifyArgs};
static void mcoreThreadModifyCall(const iocshArgBuf * args) {
    epicsThreadId tid;
    int i;
    char missing = 0;

    for (i = 0; i < 4; i++) {
        if (NULL == args[i].sval) missing |= 1;
    }
    if (missing) {
        printf("Missing argument\nUsage: mcoreThreadModify thread  policy  priority  cpuset\n");
        return;
    }
    tid = getThreadIdFor(args[0].sval);
    if (tid)
        mcoreThreadModify(tid, args[1].sval, args[2].sval, args[3].sval);
}

static const iocshFuncDef mcoreMLockDef =
    {"mcoreMLock", 0, NULL};
static void mcoreMLockCall(const iocshArgBuf * args) {
    mcoreMLock();
}

static const iocshFuncDef mcoreMUnlockDef =
    {"mcoreMUnlock", 0, NULL};
static void mcoreMUnlockCall(const iocshArgBuf * args) {
    mcoreMUnlock();
}

static void mcoreRegister(void)
{
    static int firstTime = 1;
    if(!firstTime) return;
    firstTime = 0;

    mcoreThreadShowInit();
    mcoreThreadRulesInit();
    iocshRegister(&mcoreThreadShowDef,       mcoreThreadShowCall);
    iocshRegister(&mcoreThreadShowAllDef,    mcoreThreadShowAllCall);
    iocshRegister(&mcoreThreadRuleAddDef,    mcoreThreadRuleAddCall);
    iocshRegister(&mcoreThreadRuleDeleteDef, mcoreThreadRuleDeleteCall);
    iocshRegister(&mcoreThreadRulesShowDef,  mcoreThreadRulesShowCall);
    iocshRegister(&mcoreThreadModifyDef,     mcoreThreadModifyCall);
    iocshRegister(&mcoreMLockDef,            mcoreMLockCall);
    iocshRegister(&mcoreMUnlockDef,          mcoreMUnlockCall);
}
/// @cond NEVER
epicsExportRegistrar(mcoreRegister);
/// @endcond
