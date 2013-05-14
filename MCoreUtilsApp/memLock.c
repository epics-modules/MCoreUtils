/********************************************//**
 * @file
 * @brief Locking process memory into RAM.
 * @author Ralph Lange <Ralph.Lange@gmx.de>
 * @author Dirk Zimoch <Dirk.Zimoch@psi.ch>
 * @copyright
 * Copyright (c) 2012 Paul Scherrer Institut
 * Copyright (c) 2013 ITER Organization
 * @copyright
 * Distributed subject to the EPICS_BASE Software License Agreement found
 * in the file LICENSE that is included with this distribution.
 ***********************************************/

/**
 * @file
 *
 * @ingroup memlock
 * @{
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

#include <errlog.h>
#include <shareLib.h>

/// @cond NEVER
#define epicsExportSharedSymbols
/// @endcond
#include "mcoreutils.h"

void mcoreMLock(void) {
    if (mlockall(MCL_CURRENT|MCL_FUTURE)) {
        errlogPrintf("mlockall error %s\n", strerror(errno));
    }
}

void mcoreMUnlock(void) {
    if (munlockall()) {
        errlogPrintf("munlockall error %s\n", strerror(errno));
    }
}

/**
 *@}
 */
