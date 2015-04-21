/********************************************//**
 * @file
 * @author Ralph Lange <Ralph.Lange@gmx.de>
 * @copyright
 * Copyright (c) 2012,2015 ITER Organization
 * @copyright
 * Distributed subject to the EPICS_BASE Software License Agreement found
 * in the file LICENSE that is included with this distribution.
 ***********************************************/

/**
 * @mainpage EPICS Multi-Core Utilities
 *
 * @section scope Scope of this Document
 * This documentation covers the C API and the iocShell commands of the
 * EPICS Multi-Core Utilities.
 *
 * @section intro_sec Introduction
 * The EPICS Multi-Core Utilities library contains tools that allow
 * tweaking of real-time parameters for EPICS IOC threads running on
 * multi-core processors under the Linux operating system.
 *
 * These tools are intended to set up multi-core IOCs for fast controllers, by:
 * @li Confining either parts or the complete EPICS IOC onto a subset of
 * the available cores, allowing hard real-time applications and threads
 * to run on dedicated cores.
 * @li Changing priorities of callback, driver or communication threads
 * with respect to database processing.
 * @li Selecting real-time scheduling policy (FIFO or Round-Robin)
 * for selected threads.
 * @li Locking the IOC process virtual memory into RAM to avoid swapping.
 *
 * @subsection intro_show Advanced Thread Show Routines
 * An extended version of the @c epicsThreadShow() command, showing
 * scheduling policy and CPU affinity in addition to the usual output.
 *
 * Details can be found in the documentation for module @ref threadshow.
 *
 * @subsection intro_rules Rule Based Real-Time Property Manipulation
 * A module allowing to specify rules, which consist of a regular expression
 * to match the thread name against, and a set of commands that allow to
 * specify the real-time properties of a thread.
 *
 * Whenever the EPICS IOC starts a thread, its name is matched against all
 * existing rules, and for matching rules the commands are applied.
 *
 * Details can be found in the documentation for module @ref threadrules.
 *
 * @warning
 * The default priorities of the EPICS IOC threads are well-chosen.
 * They have been proven to ensure reliable IOC operation and communication,
 * in many installations, under a variety of circumstances.
 * @n
 * Manipulating the real-time properties, especially
 * scheduling policies and priorities, may have unwanted side effects.
 * Use this feature sparingly, and test well.
 *
 * @subsection intro_memlock Memory Locking
 * A module allowing to lock the IOC process virtual memory into RAM.
 * This makes sure that no swapping occurs, and thus avoids page faults
 * which would introduce latency and lead to indeterministic timing.
 *
 * Details can be found in the documentation for module @ref memlock.
 *
 * @section sources Sources
 * The sources are on GitHub at https://github.com/epics-modules/MCoreUtils
 *
 * They can be checked out using
 * ~~~~
 * git clone https://github.com/epics-modules/MCoreUtils.git
 * ~~~~
 *
 * Releases can be found on GitHub (see above)
 * or at http://sourceforge.net/projects/epics/files/mcoreutils/
 *
 * @section requires Requirements
 * @li Linux operating system
 * @li [EPICS](http://www.aps.anl.gov/epics/)
 *     [BASE 3.15](http://www.aps.anl.gov/epics/base/R3-15/index.php) (3.15.1 or later)
 *
 * @section Installation
 * @li Unpack the distribution tar or check out the source tree.
 * @li Run @c make
 * @li To generate a minimal example IOC, run `make -C example`
 *
 * @section usage Usage
 * To use the Multi-Core Utilities in an IOC application tree,
 * you have to add a definition to `.../configure/RELEASE`
 * that points to the location of the @c mcoreutils module.
 *
 * In the directory that builds your IOC binary, the @c Makefile has to
 * make sure the IOC is only built for Linux. Then add the dbd file and
 * the Library, e.g.:
 * ~~~~
 * ...
 * PROD_IOC_Linux = mcutest
 * ...
 * mcutest_DBD += mcoreutils.dbd
 * ...
 * mcutest_LIBS += mcoreutils
 * ...
 * ~~~~
 *
 * That's it. Enjoy!
 */

#ifndef MCOREUTILS_H
#define MCOREUTILS_H

#include <unistd.h>

#include <epicsThread.h>
#include <shareLib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup threadshow Real-Time threadShow Routines
 * @brief Add two new threadShow functions that show scheduling policy and CPU affinity.
 * @{
 *
 * Adds two new threadShow functions that, in addition to the
 * properties shown by @c epicsThreadShow() and @c epicsThreadShowAll(),
 * print the scheduling policy, and the CPU affinity of each thread.
 *
 * Uses the @c epicsThreadMap() call to have a hook function being called
 * for every thread, which prints out the thread properties.
 */

/**
 * @brief Initialization routine.
 *
 * Must be called before using any of the other functions, which is done when
 * registering the iocsh commands.
 */
epicsShareFunc void mcoreThreadShowInit(void);

/**
 * @brief @b iocShell: Show thread info for one thread.
 *
 * Sets the global thread and level variables, and calls the map function.
 * @param thread id of thread to show
 * @param level  verbosity level
 *
 * @par IOC Shell
 * <tt><b>mcoreThreadShow thread level</b></tt>
 * <table border="0">
 * <tr><td>@c thread</td><td>thread name or id</td></tr>
 * <tr><td>@c level</td><td>verbosity level</td></tr>
 * </table>
 */
epicsShareFunc void mcoreThreadShow(epicsThreadId thread, unsigned int level);

/**
 * @brief @b iocShell: Show thread info for all threads.
 *
 * @param level verbosity level
 *
 * @par IOC Shell
 * <tt><b>mcoreThreadShowAll level</b></tt>
 * <table border="0">
 * <tr><td>@c level</td><td>verbosity level</td></tr>
 * </table>
 */
epicsShareFunc void mcoreThreadShowAll(unsigned int level);

/**
 * @}
 */

/**
 * @defgroup threadrules Rule-Based Thread Properties
 * @brief Allow user-specified rules that modify real-time properties of EPICS threads.
 * @{
 *
 * Implements a library that uses rules to modify real-time properties of
 * EPICS threads:
 * @li <em>Scheduling policy</em>@n
 * Scheduling mechanism used for this thread.
 * When POSIX scheduling is enabled, the default mechanism is @c SCHED_FIFO,
 * but @c SCHED_OTHER and @c SCHED_RR are also supported.
 * @li <em>Scheduling priority</em>@n
 * OSI priority value that gets converted to the system's real-time priority schema.
 * @li <em>CPU Affinity</em>@n
 * Set of CPUs that this thread is allowed to run on.
 *
 * This is achieved by creating a linked list of rules, which consist of a regular
 * expression pattern and modification instructions.
 * A hook function is added to the EPICS thread creation module.
 * The hook is called from every thread as part of its creation,
 * matches the regular expression patterns of all rules against the
 * name of the newly created thread, and applies the modifications of all
 * rules that match.
 *
 * See man pages for
 * <a href="http://www.kernel.org/doc/man-pages/online/pages/man3/pthread_setschedparam.3.html">pthread_setschedparam(3)</a>
 * and <a href="http://www.kernel.org/doc/man-pages/online/pages/man2/sched_setscheduler.2.html">sched_setscheduler(2)</a>
 * for details on scheduling policy and priority,
 * <a href="http://www.kernel.org/doc/man-pages/online/pages/man3/pthread_setaffinity_np.3.html">pthread_setaffinity_np(3)</a>
 * and <a href="http://www.kernel.org/doc/man-pages/online/pages/man2/sched_setaffinity.2.html">sched_setaffinity(2)</a>
 * for details on CPU affinity.
 *
 * @par Configuration Files
 * The module tries to read a system configuration file (@c /etc/rtrules)
 * and a user configuration file (default: <tt>$HOME/.rtrules</tt>) to create the initial
 * list of thread rules.
 * @par
 * The file format is based on the format of the @c /etc/rtgroups file on RHEL-MRG.
 * Each line has the format
 * @par
 * <tt><b>name:policy:priority:affinity:pattern</b></tt>
 * @par
 * <table border="0">
 * <tr><td>@c name</td><td>name of the rule</td></tr>
 * <tr><td>@c policy</td><td>scheduling policy to set for the thread
 * (first letter, not case sensitive), @c * = don't change @n</td></tr>
 * <tr><td>@c priority</td><td>scheduling priority to set for the thread
 * (a @c + or @c - sign adds to the current priority), @c * = don't change</td></tr>
 * <tr><td>@c affinity</td><td>CPUs to set the thread's affinity to (use @c , and @c - to specify
 * multiple CPUs and ranges, e.g. @c 0,3-5), @c * = don't change</td></tr>
 * <tr><td>@c pattern</td><td>regular expression pattern to match thread names against, see man page for
 * <a href="http://www.kernel.org/doc/man-pages/online/pages/man7/regex.7.html">regex(7)</a> for details</td></tr>
 * </table>
 * @par
 * Lines starting with @c # (comments), and empty lines (containing only whitespace) are ignored.
 *
 * @par Environment Variables
 * <dl>
 * <dt>`HOME`</dt>
 * <dd>location of the @c HOME directory (default: `/`)</dd>
 * <dt>`EPICS_MCORE_USERCONFIG`</dt>
 * <dd>name of user configuration file, relative to the @c HOME directory (default: `.rtrules`)</dd>
 * </dl>
 *
 * @par Linux Security
 * To change its scheduling policy and priority, under modern Linux systems the process must have an @c rtprio
 * entry in the pam limits module configuration.
 * @par
 * See the <a href="http://linux.die.net/man/5/limits.conf">limits.conf(5)</a>
 * man page for details.
 *
 * @par Known Issues
 * A thread calling @c epicsThreadSetPriority() to set its priority while running may override
 * the priorities defined in the rules at any time.
 */

/**
 * @brief @b iocShell: Modify a thread's real-time properties.
 *
 * @param id       EPICS thread id
 * @param policy   scheduling policy to set (@c * = don't change)
 * @param priority scheduling priority (OSI) to set (a @c + or @c - sign adds to the current priority,
 * @c * = don't change)
 * @param cpus     cpuset specification to set (use @c , and @c - to specify multiple CPUs and ranges,
 * @c * = don't change)
 *
 * @par IOC Shell
 * <tt><b>mcoreThreadModify thread policy priority cpus</b></tt>
 * <table border="0">
 * <tr><td>@c thread</td><td>thread name or id</td></tr>
 * <tr><td>@c policy</td><td>scheduling policy to set (@c * = don't change)</td></tr>
 * <tr><td>@c priority</td><td>scheduling priority (OSI) to set (a @c + or @c - sign adds to the current priority,
 * @c * = don't change)</td></tr>
 * <tr><td>@c cpus</td><td>cpuset specification to set (use @c , and @c - to specify multiple CPUs and ranges,
 * @c * = don't change)</td></tr>
 * </table>
 */
epicsShareFunc void mcoreThreadModify(epicsThreadId id,
                                      const char *policy,
                                      const char *priority,
                                      const char *cpus);

/**
 * @brief Initialization routine.
 *
 * Must be called before using any of the other functions, which is done when
 * registering the iocsh commands.
 */
epicsShareFunc void mcoreThreadRulesInit();

/**
 * @brief @b iocShell: Add or replace a thread rule.
 *
 * @param name     rule name (identifier)
 * @param policy   scheduling policy to set (@c * = don't change)
 * @param priority scheduling priority (OSI) to set (a @c + or @c - sign adds to the current priority,
 *                 @c * = don't change)
 * @param cpus     cpuset specification to set (use @c , and @c - to specify multiple CPUs and ranges,
 *                 @c * = don't change)
 * @param pattern  <a href="http://www.kernel.org/doc/man-pages/online/pages/man7/regex.7.html">regex(7)</a>
 *                 pattern to match thread names against
 * @return (OK, ERROR) as (0,-1)
 *
 * @par IOC Shell
 * <tt><b>mcoreThreadRuleAdd name policy priority cpus pattern</b></tt>
 * <table border="0">
 * <tr><td>@c name</td><td>rule name (identifier)</td></tr>
 * <tr><td>@c policy</td><td>scheduling policy to set (@c * = don't change)</td></tr>
 * <tr><td>@c priority</td><td>scheduling priority (OSI) to set (a @c + or @c - sign adds to the current priority,
 * @c * = don't change)</td></tr>
 * <tr><td>@c cpus</td><td>cpuset specification to set (use @c , and @c - to specify multiple CPUs and ranges,
 * @c * = don't change)</td></tr>
 * <tr><td>@c pattern</td>
 * <td><a href="http://www.kernel.org/doc/man-pages/online/pages/man7/regex.7.html">regex(7)</a> pattern
 * to match thread names against</td></tr>
 * </table>
 */
epicsShareFunc long mcoreThreadRuleAdd(const char *name,
                                       const char *policy,
                                       const char *priority,
                                       const char *cpus,
                                       const char *pattern);

/**
 * @brief @b iocShell: Delete a thread rule.
 *
 * @param name name (identifier) of the rule to delete
 *
 * @par IOC Shell
 * <tt><b>mcoreThreadRuleDelete name</b></tt>
 * <table border="0">
 * <tr><td>@c name</td><td>name (identifier) of the rule to delete</td></tr>
 * </table>
 */
epicsShareFunc void mcoreThreadRuleDelete(const char *name);

/**
 * @brief @b iocShell: Print a comprehensive list of the thread rules.
 *
 * Rule names are shortened to 16 characters.

 * @par IOC Shell
 * <tt><b>mcoreThreadRulesShow</b></tt>
 */
epicsShareFunc void mcoreThreadRulesShow(void);

/**
 * @}
 */

/**
 * @defgroup memlock Memory Locking
 * @brief Add functions for locking the process memory into RAM.
 * @{
 *
 * Adds functions that allow locking and unlocking the process virtual
 * memory into RAM to make sure no page faults occur, which would
 * introduce unpredictable interruptions and latency.
 *
 * See man page for
 * <a href="http://www.kernel.org/doc/man-pages/online/pages/man2/mlockall.2.html">mlockall(2)</a>
 * for more details on memory locking.

 * @par Linux Security
 * To allow locking all its memory, under modern Linux systems the process must have a @c memlock
 * entry in the pam limits module configuration.
 * @par
 * See the <a href="http://linux.die.net/man/5/limits.conf">limits.conf(5)</a>
 * man page for details.
 */

/**
 * @brief @b iocShell: Lock all process virtual memory into RAM.

 * @par IOC Shell
 * <tt><b>mcoreMLock</b></tt>
 */
epicsShareFunc void mcoreMLock(void);

/**
 * @brief @b iocShell: Unlock process virtual memory from RAM.

 * @par IOC Shell
 * <tt><b>mcoreMUnlock</b></tt>
 */
epicsShareFunc void mcoreMUnlock(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif // MCOREUTILS_H
