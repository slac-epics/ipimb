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

#include <drvIpimb.h>

#ifdef  __cplusplus
extern "C" {
#endif  /*      __cplusplus     */

extern int IPIMB_DRV_DEBUG;
extern int IPIMB_DEV_DEBUG;

#define MAX_CA_STRING_SIZE (40)

/* define function flags */
typedef enum {
        IPIMB_AI_DATA,
} IPIMBFUNC;

static struct PARAM_MAP
{
        char param[MAX_CA_STRING_SIZE];
        int  funcflag;
} param_map[1] = {
    {"DATA", IPIMB_AI_DATA}
};
#define N_PARAM_MAP (sizeof(param_map)/sizeof(struct PARAM_MAP))

typedef struct IPIMB_DEVDATA
{
    IPIMB_DEVICE    * pdevice;
    unsigned short      chnlnum;
    int         funcflag;
} IPIMB_DEVDATA;

/* This function will be called by all device support */
/* The memory for IPIMB_DEVDATA will be malloced inside */
static int IPIMB_DevData_Init(dbCommon * precord, char * ioString)
{
    int         count;
    unsigned int         loop;

    char        boxname[MAX_CA_STRING_SIZE];
    IPIMB_DEVICE    * pdevice;
    int         chnlnum;
    char        param[MAX_CA_STRING_SIZE];
    int         funcflag = 0;

    IPIMB_DEVDATA *   pdevdata;

    /* param check */
    if(precord == NULL || ioString == NULL)
    {
        if(!precord) errlogPrintf("No legal record pointer!\n");
        if(!ioString) errlogPrintf("No INP/OUT field for record %s!\n", precord->name);
        return -1;
    }

    /* analyze INP/OUT string */
    count = sscanf(ioString, "%[^:]:%i:%[^:]", boxname, &chnlnum, param);
    if (count != 3)
    {
        errlogPrintf("Record %s INP/OUT string %s format is illegal!\n", precord->name, ioString);
        return -1;
    }

    pdevice = ipimbFindDeviceByName(boxname);
    if( !pdevice )
    {
        errlogPrintf("Record [%s] with IPIMB [%s] is not registered!\n", precord->name, boxname);
        return -1;
    }

    if(chnlnum < 0 || chnlnum > 3 )
    {
        errlogPrintf("Record %s channel number %d is out of range for IPIMB %s!\n", precord->name, chnlnum, boxname);
        return -1;
    }

    for(loop=0; loop<N_PARAM_MAP; loop++)
    {
        if( 0 == strcmp(param_map[loop].param, param) )
        {
            funcflag = param_map[loop].funcflag;
            break;
        }
    }
    if(loop >= N_PARAM_MAP)
    {
        errlogPrintf("Record %s param %s is illegal!\n", precord->name, param);
        return -1;
    }

    pdevdata = (IPIMB_DEVDATA *)callocMustSucceed(1, sizeof(IPIMB_DEVDATA), "Init record for IPIMB");

    pdevdata->pdevice = pdevice;
    pdevdata->chnlnum = chnlnum;
    pdevdata->funcflag = funcflag;

    precord->dpvt = (void *)pdevdata;
    return 0;
}


long init_ai( struct aiRecord * pai)
{
    pai->dpvt = NULL;

    if (pai->inp.type!=INST_IO)
    {
        recGblRecordError(S_db_badField, (void *)pai, "devAiIPIMB Init_record, Illegal INP");
        pai->pact=TRUE;
        return (S_db_badField);
    }
    pai->eslo = (pai->eguf - pai->egul)/(float)0x10000;
    pai->roff = 0x0;

    if(IPIMB_DevData_Init((dbCommon *) pai, pai->inp.value.instio.string) != 0)
    {
        errlogPrintf("Fail to init devdata for record %s!\n", pai->name);
        recGblRecordError(S_db_badField, (void *) pai, "Init devdata Error");
        pai->pact = TRUE;
        return (S_db_badField);
    }

    return 0;
}


/** for sync scan records  **/
long ai_ioint_info(int cmd,aiRecord *pai,IOSCANPVT *iopvt)
{
    IPIMB_DEVDATA * pdevdata = (IPIMB_DEVDATA *)(pai->dpvt);

    *iopvt = pdevdata->pdevice->ioscan;
    return 0;
}

long read_ai(struct aiRecord *pai)
{
    IPIMB_DEVDATA * pdevdata = (IPIMB_DEVDATA *)(pai->dpvt);

    int status=-1;

    switch(pdevdata->funcflag)
    {
    case IPIMB_AI_DATA:
      {
        switch(pdevdata->chnlnum)
            {
            case 0:
                pai->rval = pdevdata->pdevice->ipmData._ch0;
                break;
            case 1:
                pai->rval = pdevdata->pdevice->ipmData._ch1;
                break;
            case 2:
                pai->rval = pdevdata->pdevice->ipmData._ch2;
                break;
            case 3:
            default:
                pai->rval = pdevdata->pdevice->ipmData._ch3;
                break;
            }
        status = pdevdata->pdevice->ipmBoard.dataDamage();
      }
      break;
    }

    if(status)
    {
        if(IPIMB_DEV_DEBUG) printf("Data damaged for record [%s]\n", pai->name);
        recGblSetSevr(pai, READ_ALARM, INVALID_ALARM);
        return 0;
    }
    else
    {
        pai->udf=FALSE;
        return 0;/******** convert ****/
    }
}

long ai_lincvt(struct aiRecord   *pai, int after)
{

        if(!after) return(0);
        /* set linear conversion slope*/
        pai->eslo = (pai->eguf - pai->egul)/(float)0x10000;
        pai->roff = 0x0;
        return(0);
}

#ifdef  __cplusplus
}
#endif  /*      __cplusplus     */

#if 0
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

#endif
                   
