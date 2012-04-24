#!../../bin/linux-x86/ipimbIoc

# This is a sample st.cmd file for the IPIMB IOC.
# It currently supports one IPIMB, but is easily extensible to many.
# The following variables need to be defined:
#     IOCNAME   - The name of the IOC.
#     IOCBASE   - The basename of most of the IOCs PVs.
#     EVRBASE   - The basename of the EVR-specific PVs.
#     IPIMBBASE - The basename of the IPIMB-specific PVs.
#     IPIMBNAME - The name of the IPIMB.
#     IPIMBPORT - The name of the IPIMB serial port.
#     IPIMBPHID - The physical ID of the IPIMB (for BLD generation).
#     TRIGGER   - The EVR trigger used (0-3)
#     EVRTYPE   - The type of EVR (1 = MRF, 15 = SLAC)
epicsEnvSet("IOCNAME", "ioc-xrt-xcsimb1")
epicsEnvSet("IOCBASE", "XRT:R04:IOC:35")
epicsEnvSet("EVRBASE", "XRT:R04:EVR:35")
epicsEnvSet("IPIMBBASE", "HXX:UM6:PIM:01")
epicsEnvSet("IPIMBNAME", "HXX-UM6-PIM-01")
epicsEnvSet("IPIMBPORT", "/dev/ttyPS0")
epicsEnvSet("IPIMBPHID", "4")
epicsEnvSet("TRIGGER", "0")
epicsEnvSet("TRIGNAME", "$(EVRBASE):Triggers.A")
epicsEnvSet("EVRTYPE", "1")
epicsEnvSet("FP0L", "$(IPIMBBASE)"
epicsEnvSet("FP1L", "")
epicsEnvSet("FP2L", "")
epicsEnvSet("FP3L", "")
#############################################################
epicsEnvSet( "ENGINEER", "Michael Browne (mcbrowne)" )
epicsEnvSet( "LOCATION", "$(IOCBASE)" )
epicsEnvSet( "IOCSH_PS1", "$(IOCNAME)> " )
< envPaths
cd( "../.." )

# Run common startup commands for linux soft IOC's
< /reg/d/iocCommon/All/pre_linux.cmd

# Register all support components
dbLoadDatabase("dbd/ipimbIoc.dbd")
ipimbIoc_registerRecordDeviceDriver(pdbbase)

ErDebugLevel( 0 )

# Initialize PMC EVR
ErConfigure( 0, 0, 0, 0, $(EVRTYPE) )

# Load EVR record instances
dbLoadRecords( "db/evr-ipimb.db", "IOC=$(IOCBASE),EVR=$(EVRBASE)" )

# Datatype is Id_SharedIpimb (35), Version 1, so 65536+35 = 65571
ipimbAdd("$(IPIMBNAME)", "$(IPIMBPORT)", "239.255.24.$(IPIMBPHID)", $(IPIMBPHID), 65571, $(TRIGNAME))

# Load remaining record instances
dbLoadRecords( "db/ipimb_iocAdmin.db",		"IOC=$(IOCBASE)" )
dbLoadRecords( "db/save_restoreStatus.db",	"IOC=$(IOCBASE)" )
dbLoadRecords( "db/bldSettings.db",             "IOC=$(IOCBASE),BLDNO=0" )
dbLoadRecords( "db/ipimb.db",                   "RECNAME=$(IPIMBBASE),BOX=$(IPIMBNAME),TRIGGER=$(EVRBASE):CTRL.DG$(TRIGGER)E")

# Setup autosave
set_savefile_path( "$(IOC_DATA)/$(IOC)/autosave" )
set_requestfile_path( "$(TOP)/autosave" )
save_restoreSet_status_prefix( "$(IOCBASE):" )
save_restoreSet_IncompleteSetsOk( 1 )
save_restoreSet_DatedBackupFiles( 1 )
set_pass0_restoreFile( "$(IOCNAME).sav" )
set_pass1_restoreFile( "$(IOCNAME).sav" )

# Initialize the IOC and start processing records
iocInit()

# Start autosave backups
create_monitor_set( "$(IOCNAME).req", 5, "IOC=$(IOCBASE)" )

# BldConfig sAddr uPort uMaxDataSize sInterfaceIp uSrcPyhsicalId iDataType sBldPvTrigger sBldPvFiducial sBldPvList
# EVENT14CNT is EVR event 140
BldSetID(0)
BldConfig( "239.255.24.$(IPIMBPHID)", 10148, 512, 0, $(IPIMBPHID), 65571, "$(IPIMBBASE):CURRENTFID", "$(IPIMBBASE):YPOS", "$(IPIMBBASE):CURRENTFID", "$(IPIMBBASE):CH0_RAW.INP,$(IPIMBBASE):CH0,$(IPIMBBASE):CH1,$(IPIMBBASE):CH2,$(IPIMBBASE):CH3,$(IPIMBBASE):SUM,$(IPIMBBASE):XPOS,$(IPIMBBASE):YPOS" )
BldSetDebugLevel(1)
# Uncomment the next line for BLD generation.
# BldStart()
BldShowConfig()

# Configure the IPIMB.
dbpf $(IPIMBBASE):DoConfig 1
epicsThreadSleep(1)

# Fix up the EVR descriptions!
dbpf $(EVRBASE):FP0L.DESC "$(FP0L)"
dbpf $(EVRBASE):FP1L.DESC "$(FP1L)"
dbpf $(EVRBASE):FP1L.DESC "$(FP2L)"
dbpf $(EVRBASE):FP1L.DESC "$(FP3L)"

# All IOCs should dump some common info after initial startup.
< /reg/d/iocCommon/All/post_linux.cmd
