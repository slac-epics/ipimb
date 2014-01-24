TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#----------------------------------------------------
#  Optimization of db files using dbst (DEFAULT: NO)
#DB_OPT = YES

#----------------------------------------------------
# Create and install (or just install)
# databases, templates, substitutions like this

#----------------------------------------------------
# Create and install (or just install)
# Archive files, templates, substitutions like this
#
ARCHIVE += archive_ipimbIoc.archive
ARCHIVE += archive_ipimb.archive
REQ += autosave_ipimb.req

# Nothing to build for autosave, just install from $AUTOSAVE/db
DB_INSTALLS += $(AUTOSAVE)/db/save_restoreStatus.db

# Grab a few things from the IPIMB module.
DB_INSTALLS += $(IPIMB)/db/bldFanout.db
DB_INSTALLS += $(IPIMB)/db/bldSettings.db
DB_INSTALLS += $(IPIMB)/db/ipimb.db
DB_INSTALLS += $(IPIMB)/db/ipimb_iocAdmin.db
DB_INSTALLS += $(IPIMB)/db/evr-ipimb.db

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

