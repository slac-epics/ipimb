#!../../bin/linux-x86/ipimbIoc

## ToDo: The following substitutions can be done via makeBaseApp.pl
## If they weren't, do them before releasing your IOC
##
## Replace _ USER _ with your userid
##
## Replace _ APPNAME _ with the name of the application
##
## Replace _ IOC _ with the network name of the IOC
##
## Replace _ IOCPVROOT _ with the PV prefix used for
## iocAdmin PV's on this IOC
## ex. AMO:R15:IOC:23
##

epicsEnvSet( "ENGINEER", "Michael Browne (mcbrowne)" )
epicsEnvSet( "LOCATION", "XRT:R04:IOC:35" )
epicsEnvSet( "IOCSH_PS1", "ioc-xrt-xcsimb1> " )
< envPaths
cd( "../.." )

# Run common startup commands for linux soft IOC's
< /reg/d/iocCommon/All/pre_linux.cmd

# Register all support components
dbLoadDatabase("dbd/ipimbIoc.dbd")
ipimbIoc_registerRecordDeviceDriver(pdbbase)

ErDebugLevel( 0 )

# Call it physical id 4 (HxxUm6Imb01) with datatype Id_SharedIpimb (35)
ipimbAdd("HXX-UM6-PIM-01", "/dev/ttyPS0", "239.255.24.4", 4, 35)

# Initialize PMC EVR
ErConfigure( 0, 0, 0, 0, 1 )

# Load record instances
dbLoadRecords( "db/evr-ipimb.db", "IOC=XRT:R04:IOC:35,EVR=XRT:R04:EVR:35" )
dbLoadRecords( "db/iocAdmin.db",		"IOC=XRT:R04:IOC:35" )
dbLoadRecords( "db/save_restoreStatus.db",	"IOC=XRT:R04:IOC:35" )
dbLoadRecords( "db/bldSettings.db",             "IOC=XRT:R04:IOC:35" )
dbLoadRecords( "db/ipimb.db",                   "RECNAME=HXX:UM6:PIM:01,BOX=HXX-UM6-PIM-01,CH0=HXX:UM6:PIM:01:CH0,CH1=HXX:UM6:PIM:01:CH1,CH2=HXX:UM6:PIM:01:CH2,CH3=HXX:UM6:PIM:01:CH3")

# Setup autosave
set_savefile_path( "$(IOC_DATA)/$(IOC)/autosave" )
set_requestfile_path( "$(TOP)/autosave" )
save_restoreSet_status_prefix( "XRT:R04:IOC:35:" )
save_restoreSet_IncompleteSetsOk( 1 )
save_restoreSet_DatedBackupFiles( 1 )
set_pass0_restoreFile( "autosave_ipimbIoc.sav" )
set_pass1_restoreFile( "autosave_ipimbIoc.sav" )

# Initialize the IOC and start processing records
iocInit()

# Start autosave backups
create_monitor_set( "autosave_ipimbIoc.req", 5, "IOC=XRT:R04:IOC:35" )

# BldConfig sAddr uPort uMaxDataSize sInterfaceIp uSrcPyhsicalId iDataType sBldPvTrigger sBldPvFiducial sBldPvList
# EVENT14CNT is EVR event 140
BldConfig( "239.255.24.4", 10148, 512, 0, 4, 35, "XRT:R04:EVR:35:EVENT14CNT", "XRT:R04:PIM:01:YPOS", "XRT:R04:IOC:35:PATTERN.L", "XRT:R04:PIM:01:CH0_RAW.INP,XRT:R04:PIM:01:CH0,XRT:R04:PIM:01:CH1,XRT:R04:PIM:01:CH2,XRT:R04:PIM:01:CH3,XRT:R04:PIM:01:SUM,XRT:R04:PIM:01:XPOS,XRT:R04:PIM:01:YPOS" )
BldSetDebugLevel(1)
BldStart()
BldIsStarted()
BldShowConfig()
# Don't really send BLDs for now!
BldStop()

# All IOCs should dump some common info after initial startup.
< /reg/d/iocCommon/All/post_linux.cmd
