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

#edm -x -m "IOC=XRT:R04:IOC:35" -eolc ipimbIocScreens/ipimbIoc.edl &

edm -x -m "IOC=XRT:R38:IOC:43,RECNAME=HFX:DG2:IMB:01" -eolc ipimbScreens/ipimb.edl &
edm -x -m "IOC=XRT:R38:IOC:43,RECNAME=HFX:DG2:IMB:02" -eolc ipimbScreens/ipimb.edl &
edm -x -m "IOC=XRT:R38:IOC:43,RECNAME=XCS:DG3:IMB:03" -eolc ipimbScreens/ipimb.edl &
edm -x -m "IOC=XRT:R38:IOC:43,RECNAME=XCS:DG3:IMB:04" -eolc ipimbScreens/ipimb.edl &
edm -x -m "IOC=XRT:R38:IOC:43,RECNAME=HFX:DG3:IMB:01" -eolc ipimbScreens/ipimb.edl &
edm -x -m "IOC=XRT:R38:IOC:43,RECNAME=HFX:DG3:IMB:02" -eolc ipimbScreens/ipimb.edl &

#edm -x -m "RECNAME=HFX:DG3:IPM:01,IOC=XRT:R38:IOC:43" -eolc ipimbScreens/ipimb.edl
