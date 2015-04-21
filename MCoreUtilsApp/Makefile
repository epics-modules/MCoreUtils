TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

# The Doxygen-generated documentation is part of the source distribution.
# Re-generate it by calling
#    make doxy

VERSION = 1.2.1

#==================================================
# build a support library  (Linux ONLY)

LIBRARY_IOC_Linux += mcoreutils

# install mcoreutils.dbd into <top>/dbd
DBD += mcoreutils.dbd

# specify all source files to be compiled and added to the library
mcoreutils_SRCS += threadShow.c
mcoreutils_SRCS += threadRules.c
mcoreutils_SRCS += memLock.c
mcoreutils_SRCS += shellCommands.c
mcoreutils_SRCS += utils.c

mcoreutils_LIBS += $(EPICS_BASE_IOC_LIBS)

INSTALL_DOCS += $(INSTALL_HTML)/MCoreUtils
DOCS += MCoreUtils.pdf

USR_CFLAGS = -D_GNU_SOURCE -DVERSION=\"$(VERSION)\"

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

$(INSTALL_DOC)/%: ../documentation/%
	$(ECHO) "Installing doc $@"
	@$(INSTALL) -d -m $(INSTALL_PERMISSIONS) $< $(INSTALL_DOC)

$(INSTALL_HTML)/%: ../documentation/%
	$(ECHO) "Installing html tree $@"
	@$(RMDIR) $@
	@$(MKDIR) -p $(INSTALL_HTML)
	@$(CP) -rT $< $@
	@find $@ -type f -exec chmod $(INSTALL_PERMISSIONS) {} \;

doxy:
	@$(RMDIR) O.Common/doxy
	@$(MKDIR) -p O.Common/doxy
	@$(RMDIR) documentation
	@$(MKDIR) -p documentation/MCoreUtils
	@( cat Doxyfile ; echo "PROJECT_NUMBER=$(VERSION)" ) | doxygen -
	@make -C O.Common/doxy/latex
	@$(CP) O.Common/doxy/latex/refman.pdf documentation/MCoreUtils.pdf
	@$(CP) -rT O.Common/doxy/html documentation/MCoreUtils
