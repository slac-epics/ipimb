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

epicsEnvSet( "ENGINEER", "Sheng Peng (pengs)" )
epicsEnvSet( "LOCATION", "TST:R40:IOC:22" )
epicsEnvSet( "IOCSH_PS1", "ioc-tst-05> " )
< envPaths
cd( "../.." )

# Run common startup commands for linux soft IOC's
< /reg/d/iocCommon/All/pre_linux.cmd

# Register all support components
dbLoadDatabase("dbd/ipimbIoc.dbd")
ipimbIoc_registerRecordDeviceDriver(pdbbase)

ErDebugLevel( 0 )

ipimbAdd("HFX-DG1-IPM-01", "/dev/ttyPS0", "239.255.24.4")

# Initialize PMC EVR
ErConfigure( 0, 0, 0, 0, 1 )

# Load record instances
dbLoadRecords( "db/evr-ipimb.db", "IOC=TST:R40:IOC:22,EVR=TST:R40:EVR:22" )
dbLoadRecords( "db/iocAdmin.db",		"IOC=TST:R40:IOC:22" )
dbLoadRecords( "db/save_restoreStatus.db",	"IOC=TST:R40:IOC:22" )
dbLoadRecords( "db/ioc-tst-05.db")

# Setup autosave
set_savefile_path( "$(IOC_DATA)/$(IOC)/autosave" )
set_requestfile_path( "$(TOP)/autosave" )
save_restoreSet_status_prefix( "TST:R40:IOC:22:" )
save_restoreSet_IncompleteSetsOk( 1 )
save_restoreSet_DatedBackupFiles( 1 )
set_pass0_restoreFile( "autosave_ipimbIoc.sav" )
set_pass1_restoreFile( "autosave_ipimbIoc.sav" )

# Initialize the IOC and start processing records
iocInit()

# Start autosave backups
create_monitor_set( "autosave_ipimbIoc.req", 5, "IOC=TST:R40:IOC:22" )

# All IOCs should dump some common info after initial startup.
< /reg/d/iocCommon/All/post_linux.cmd
