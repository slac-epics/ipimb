TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build a module library
# Use LIBRARY_IOC_RTEMS for RTEMS only
# Use LIBRARY_IOC_Linux for Linux only
LIBRARY_IOC_Linux = ipimb

# ipimb.dbd will be made up from these files:
DBD += ipimb.dbd

# Add any header files here which should be installed
#INC	+=	drvIpimb.h

# Add any html documentation here
# HTMLS	+=	index.html
# HTMLS +=	ipimb.html
HTMLS_DIR	= .

# Add any source files here
# Note: No lib will be built if no sources are listed here
LIBSRCS += drvIpimb.cpp
LIBSRCS += devIpimb.cpp
LIBSRCS += devIpimbReg.c
LIBSRCS += devIpimbGenSub.c
LIBSRCS += IpimBoard.cc
LIBSRCS += ConfigV2.cc
LIBSRCS += drvIpimbRegister.cpp

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

