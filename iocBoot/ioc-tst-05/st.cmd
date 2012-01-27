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
epicsEnvSet( "LOCATION", "TST:R01:IOC:22" )
epicsEnvSet( "IOCSH_PS1", "ioc-tst-05> " )
< envPaths
cd( "../.." )

# Run common startup commands for linux soft IOC's
< /reg/d/iocCommon/All/pre_linux.cmd

# Register all support components
dbLoadDatabase("dbd/ipimbIoc.dbd")
ipimbIoc_registerRecordDeviceDriver(pdbbase)

ErDebugLevel( 0 )

# Call it physical id 40 (bogus!) with datatype Id_SharedIpimb (35)
ipimbAdd("TST-R01-PIM-01", "/dev/ttyPS0", "239.255.24.40", 40, 35)

# Initialize PMC EVR
ErConfigure( 0, 0, 0, 0, 1 )

# Load record instances
dbLoadRecords( "db/evr-ipimb.db",		"IOC=TST:R01:IOC:22,EVR=TST:R01:EVR:22" )
dbLoadRecords( "db/iocAdmin.db",		"IOC=TST:R01:IOC:22" )
dbLoadRecords( "db/save_restoreStatus.db",	"IOC=TST:R01:IOC:22" )
dbLoadRecords( "db/bldSettings.db",             "IOC=TST:R01:IOC:22" )
dbLoadRecords( "db/ipimb.db",                   "RECNAME=TST:R01:PIM:01,BOX=TST-R01-PIM-01");

# Setup autosave
set_savefile_path( "$(IOC_DATA)/$(IOC)/autosave" )
set_requestfile_path( "$(TOP)/autosave" )
save_restoreSet_status_prefix( "TST:R01:IOC:22:" )
save_restoreSet_IncompleteSetsOk( 1 )
save_restoreSet_DatedBackupFiles( 1 )
set_pass0_restoreFile( "ioc-tst-05.sav" )
set_pass1_restoreFile( "ioc-tst-05.sav" )

# Initialize the IOC and start processing records
iocInit()

# Start autosave backups
create_monitor_set( "ioc-tst-05.req", 5, "IOC=TST:R01:IOC:22:" )

# BldConfig sAddr uPort uMaxDataSize sInterfaceIp uSrcPyhsicalId iDataType sBldPvTrigger sBldPvFiducial sBldPvList
# EVENT14CNT is EVR event 140
BldConfig( "239.255.24.40", 10148, 512, 0, 40, 35, "TST:R01:EVR:22:EVENT14CNT", "TST:R01:PIM:01:YPOS", "TST:R01:IOC:22:PATTERN.L", "TST:R01:PIM:01:CH0_RAW.INP,TST:R01:PIM:01:CH0,TST:R01:PIM:01:CH1,TST:R01:PIM:01:CH2,TST:R01:PIM:01:CH3,TST:R01:PIM:01:SUM,TST:R01:PIM:01:XPOS,TST:R01:PIM:01:YPOS" )
BldSetDebugLevel(1)
BldStart()
BldIsStarted()
BldShowConfig()
# Don't really send BLDs for now!
BldStop()

# All IOCs should dump some common info after initial startup.
< /reg/d/iocCommon/All/post_linux.cmd
