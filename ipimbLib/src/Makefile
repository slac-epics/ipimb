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
ipimb_DBD += ipimb.dbd
ipimb_DBD += bigselRecord.dbd

# Add any header files here which should be installed
#INC	+=	drvIpimb.h

# Add any html documentation here
# HTMLS	+=	index.html
# HTMLS +=	ipimb.html
HTMLS_DIR	= .

# Add any source files here
# Note: No lib will be built if no sources are listed here
ipimb_SRCS += bigselRecord.c
ipimb_SRCS += drvIpimb.cpp
ipimb_SRCS += devIpimb.cpp
ipimb_SRCS += devIpimbReg.c
ipimb_SRCS += devIpimbGenSub.c
ipimb_SRCS += IpimBoard.cc
ipimb_SRCS += ipimbsync.cc
ipimb_SRCS += ConfigV2.cc
ipimb_SRCS += drvIpimbRegister.cpp

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

