#include <stdio.h>
#include <string.h>

#include <epicsVersion.h>

#if EPICS_VERSION>=3 && EPICS_REVISION>=14
#include <epicsExport.h>
#endif

#include <devLib.h>
#include <dbAccess.h>
#include <dbScan.h>
#include <callback.h>
#include <cvtTable.h>
#include <link.h>
#include <recSup.h>
#include <recGbl.h>
#include <devSup.h>
#include <drvSup.h>
#include <dbCommon.h>
#include <alarm.h>
#include <cantProceed.h>
#include <aiRecord.h>

#ifdef  __cplusplus
extern "C" {
#endif  /*      __cplusplus     */

extern long init_ai( struct aiRecord * pai);
extern long ai_ioint_info(int cmd,aiRecord *pai,IOSCANPVT *iopvt);
extern long read_ai(struct aiRecord *pai);
extern long ai_lincvt(struct aiRecord   *pai, int after);


#ifdef  __cplusplus
}
#endif  /*      __cplusplus     */

struct IPIMB_DEV_SUP_SET
{
    long            number;
    DEVSUPFUN       report;
    DEVSUPFUN       init;
    DEVSUPFUN       init_record;
    DEVSUPFUN       get_ioint_info;
    DEVSUPFUN       read_ai;
    DEVSUPFUN       special_linconv;
} devAiIPIMB = {6, NULL, NULL, init_ai, ai_ioint_info, read_ai, ai_lincvt};

#if EPICS_VERSION>=3 && EPICS_REVISION>=14
epicsExportAddress(dset, devAiIPIMB);
#endif

                   
