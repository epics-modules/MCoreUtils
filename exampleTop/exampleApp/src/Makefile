TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the test IOC application  (Linux ONLY)

PROD_IOC_Linux = mcutest
# mcutest.dbd will be created and installed
DBD += mcutest.dbd

# mcutest.dbd will be made up from these files:
mcutest_DBD += base.dbd

# Include dbd files from all support applications:
mcutest_DBD += mcoreutils.dbd

# Add all the support libraries needed by this IOC
mcutest_LIBS += mcoreutils

# mcutest_registerRecordDeviceDriver.cpp derives from mcutest.dbd
mcutest_SRCS += mcutest_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
mcutest_SRCS_DEFAULT += mcutestMain.cpp
mcutest_SRCS_vxWorks += -nil-

# Finally link to the EPICS Base libraries
mcutest_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

