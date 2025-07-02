#include <stdio.h>
#include <string.h>

#include <epicsVersion.h>

#include <epicsExport.h>

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
#include <biRecord.h>

#ifdef  __cplusplus
extern "C" {
#endif  /*      __cplusplus     */

extern long init_ai(struct aiRecord *pai);
extern long ai_ioint_info(int cmd,aiRecord *pai,IOSCANPVT *iopvt);
extern long read_ai(struct aiRecord *pai);
extern long ai_lincvt(struct aiRecord   *pai, int after);

extern long init_bi(struct biRecord *pbi);
extern long read_bi(struct biRecord *pbi);

#ifdef  __cplusplus
}
#endif  /*      __cplusplus     */

struct AI_IPIMB_DEV_SUP_SET
{
    long            number;
    DEVSUPFUN       report;
    DEVSUPFUN       init;
    DEVSUPFUN       init_record;
    DEVSUPFUN       get_ioint_info;
    DEVSUPFUN       read_ai;
    DEVSUPFUN       special_linconv;
} devAiIPIMB = {6, NULL, NULL, init_ai, ai_ioint_info, read_ai, ai_lincvt};

struct BI_IPIMB_DEV_SUP_SET
{
    long            number;
    DEVSUPFUN       report;
    DEVSUPFUN       init;
    DEVSUPFUN       init_record;
    DEVSUPFUN       get_ioint_info;
    DEVSUPFUN       read_bi;
} devBiIPIMB = {5, NULL, NULL, init_bi, NULL, read_bi};

epicsExportAddress(dset, devAiIPIMB);
epicsExportAddress(dset, devBiIPIMB);

