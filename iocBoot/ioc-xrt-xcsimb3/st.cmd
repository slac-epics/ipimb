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

epicsEnvSet( "ENGINEER", "dflath" )
epicsEnvSet( "LOCATION", "XRT:R38:IOC:43" )
epicsEnvSet( "IOCSH_PS1", "ioc-xrt-xcsimb3> " )
< envPaths
cd( "../.." )

# Run common startup commands for linux soft IOC's
< /reg/d/iocCommon/All/pre_linux.cmd

# Register all support components
dbLoadDatabase("dbd/ipimbIoc.dbd")
ipimbIoc_registerRecordDeviceDriver(pdbbase)

ipimbAdd("HFX-DG2-IMB-01", "/dev/ttyPS1", "239.255.24.6")
ipimbAdd("HFX-DG2-IMB-02", "/dev/ttyPS2", "239.255.24.7")
ipimbAdd("XCS-DG3-IMB-03", "/dev/ttyPS3", "239.255.24.8")
ipimbAdd("XCS-DG3-IMB-04", "/dev/ttyPS4", "239.255.24.9")
ipimbAdd("HFX-DG3-IMB-01", "/dev/ttyPS5", "239.255.24.10")
ipimbAdd("HFX-DG3-IMB-02", "/dev/ttyPS6", "239.255.24.11")

# Load record instances
dbLoadRecords( "db/iocAdmin.db",		"IOC=XRT:R38:IOC:43" )
dbLoadRecords( "db/save_restoreStatus.db",	"IOC=XRT:R38:IOC:43" )
dbLoadRecords( "db/ioc-xrt-xcsimb3.db")
dbLoadRecords( "db/ipimb.db, "RECNAME=HFX:DG2:IMB:01,BOX=HFX-DG2-IMB-01")
dbLoadRecords( "db/ipimb.db, "RECNAME=HFX:DG2:IMB:02,BOX=HFX-DG2-IMB-02")
dbLoadRecords( "db/ipimb.db, "RECNAME=XCS:DG3:IMB:03,BOX=XCS-DG3-IMB-03")
dbLoadRecords( "db/ipimb.db, "RECNAME=XCS:DG3:IMB:04,BOX=XCS-DG3-IMB-04")
dbLoadRecords( "db/ipimb.db, "RECNAME=HFX:DG3:IMB:01,BOX=HFX-DG3-IMB-01")
dbLoadRecords( "db/ipimb.db, "RECNAME=HFX:DG3:IMB:02,BOX=HFX-DG3-IMB-02")

# Setup autosave
set_savefile_path( "$(IOC_DATA)/$(IOC)/autosave" )
set_requestfile_path( "$(TOP)/autosave" )
save_restoreSet_status_prefix( "XRT:R38:IOC:43:" )
save_restoreSet_IncompleteSetsOk( 1 )
save_restoreSet_DatedBackupFiles( 1 )
set_pass0_restoreFile( "autosave_ipimbIoc.sav" )
set_pass1_restoreFile( "autosave_ipimbIoc.sav" )

# Initialize the IOC and start processing records
iocInit()

# Start autosave backups
create_monitor_set( "autosave_ipimbIoc.req", 5, "IOC=XRT:R38:IOC:43" )

# All IOCs should dump some common info after initial startup.
< /reg/d/iocCommon/All/post_linux.cmd
