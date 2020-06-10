<a target="_blank" href="http://semver.org">![Version][badge.version]</a>
<a target="_blank" href="https://travis-ci.org/github/epics-modules/MCoreUtils">![Travis status][badge.travis]</a>
<a target="_blank" href="https://app.codacy.com/gh/epics-modules/MCoreUtils">![Codacy grade][badge.codacy]</a>

# MCoreUtils
## Real-Time Utilities for EPICS IOCs on Multi-Core Linux

The EPICS Multi-Core Utilities library contains tools that allow tweaking of
real-time parameters for EPICS IOC threads running on multi-core processors
under the Linux operating system.

These tools are intended to set up multi-core IOCs for fast controllers, by:

-   Confining either parts or the complete EPICS IOC onto a subset
    of the available cores, allowing hard real-time applications and threads
    to run on dedicated cores.

-   Changing priorities of callback, driver or communication threads with
    respect to database processing.

-   Selecting real-time scheduling policy (FIFO or Round-Robin) for selected
    threads.

-   Locking the IOC process virtual memory into RAM to avoid swapping.

### Requirements

-   Linux operating system
-   EPICS BASE 3.15 or above (EPICS 7 supported).

### Installation

-   Unpack the distribution tar or check out the source tree
-   Run `make`
-   To generate a minimal example IOC, run `make -C example`.

### Usage

To use the Multi-Core Utilities in an IOC application tree, you have to add a
definition to `.../configure/RELEASE` or `RELEASE.local` that points to the
location of the mcoreutils module.

In the directory that builds your IOC binary, the `Makefile` has to make sure
the IOC is only built for Linux. Then add the dbd file and the Library, e.g.:

```makefile
...
PROD_IOC_Linux = mcutest
...
mcutest_DBD += mcoreutils.dbd
...
mcutest_LIBS += mcoreutils
...
```

That's it. Enjoy!

<!-- Links -->
[badge.version]: https://badge.fury.io/gh/epics-modules%2FMCoreUtils.svg
[badge.travis]: https://travis-ci.org/epics-modules/MCoreUtils.svg?branch=master
[badge.codacy]: https://app.codacy.com/project/badge/Grade/22fd07b642a444f7975e00f27aec2479
