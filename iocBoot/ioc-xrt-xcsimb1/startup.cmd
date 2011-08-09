#!/bin/bash

# Setup the IOC user environment
source /reg/d/iocCommon/All/fee_env.sh

# Make sure the IOC's data directories are ready for use
export IOC="ioc-xrt-xcsimb1"
$RUNUSER "mkdir -p $IOC_DATA/$IOC/autosave"
$RUNUSER "mkdir -p $IOC_DATA/$IOC/archive"
$RUNUSER "mkdir -p $IOC_DATA/$IOC/iocInfo"

# Make sure permissions are correct
$RUNUSER "chmod ug+w -R $IOC_DATA/$IOC"

# For release
#cd $EPICS_SITE_TOP/modules/ipimb/R0.1.0/iocBoot/ioc-xrt-xcsimb1

# Copy the archive file to iocData
$RUNUSER "cp ../../archive/$IOC.archive $IOC_DATA/$IOC/archive"

# Launch the IOC
$RUNUSER "$PROCSERV --logfile $IOC_DATA/$IOC/iocInfo/ioc.log --name $IOC 30001 ../../bin/linux-x86/ipimbIoc st.cmd"
