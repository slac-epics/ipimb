TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build an ioc application
# Use PROD_IOC_RTEMS for RTEMS only
# Use PROD_IOC_Linux for Linux only
PROD_IOC_Linux = ipimbIoc

# ipimbIoc.dbd will be created and installed
DBD += ipimbIoc.dbd

# ipimbIoc.dbd will be made up from these files:
ipimbIoc_DBD += base.dbd
ipimbIoc_DBD += iocAdmin.dbd
ipimbIoc_DBD += autosaveSupport.dbd
ipimbIoc_DBD += ipimb.dbd
ipimbIoc_DBD += genSubRecord.dbd
ipimbIoc_DBD += evrSupport.dbd
ipimbIoc_DBD += devMrfEr.dbd
ipimbIoc_DBD += bigselRecord.dbd
ipimbIoc_DBD += bldClient.dbd

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
ipimbIoc_SRCS += ipimbIoc_registerRecordDeviceDriver.cpp
ipimbIoc_SRCS_DEFAULT += ipimbIocMain.cpp

# Add locally compiled object code
#ipimbIoc_SRCS += dbSubExample.c

ipimbIoc_LIBS += devIocStats
ipimbIoc_LIBS += autosave
ipimbIoc_LIBS += genSub
ipimbIoc_LIBS += ipimb
ipimbIoc_LIBS += generalTime
ipimbIoc_LIBS += evrSupport
ipimbIoc_LIBS += devMrfEr
ipimbIoc_LIBS += drvMrf
ipimbIoc_LIBS += mrfApi
ipimbIoc_LIBS += bldClient
ipimbIoc_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

