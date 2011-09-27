#! /bin/bash

# Setup edm environment
export EPICS_SITE_TOP=/reg/g/pcds/package/epics/3.14
export EPICS_HOST_ARCH=$($EPICS_SITE_TOP/base/current/startup/EpicsHostArch.pl)

export EDMFILES=$EPICS_SITE_TOP/extensions/current/templates/edm
export EDMFILTERS=$EPICS_SITE_TOP/extensions/current/templates/edm
export EDMHELPFILES=$EPICS_SITE_TOP/extensions/current/src/edm/helpFiles
export EDMLIBS=$EPICS_SITE_TOP/extensions/current/lib/$EPICS_HOST_ARCH
export EDMOBJECTS=$EPICS_SITE_TOP/extensions/current/templates/edm
export EDMPVOBJECTS=$EPICS_SITE_TOP/extensions/current/templates/edm
export EDMUSERLIB=$EPICS_SITE_TOP/extensions/current/lib/$EPICS_HOST_ARCH
export EDMACTIONS=/reg/g/pcds/package/tools/edm/config
export EDMWEBBROWSER=mozilla
export PATH=$PATH:$EPICS_SITE_TOP/extensions/current/bin/$EPICS_HOST_ARCH
export EDMDATAFILES=".:.."

cd /reg/g/pcds/package/epics/3.14/modules/ipimb/Development

edm -x -m "IOC=XRT:R04:IOC:35,RECNAME=HXX:UM6:PIM:01" -eolc ipimbScreens/ipimb.edl &
cd /reg/g/pcds/package/epics/3.14/modules/event/R3.2.0-1.7.0
edm -x -m "IOC=XRT:R04:IOC:35,EVR=XRT:R04:EVR:35" evrscreens/evr.edl  &
