#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <epicsVersion.h>

#if EPICS_VERSION>=3 && EPICS_REVISION>=14
#include <epicsExport.h>
#endif
#include <epicsTypes.h>
#include <epicsEvent.h>
#include <epicsMutex.h>
#include <epicsMessageQueue.h>
#include <epicsThread.h>
#include <epicsString.h>
#include <epicsTime.h>
#include <osiSock.h>
#include <ellLib.h>
#include <errlog.h>
#include <callback.h>
#include <cantProceed.h>

#include <dbCommon.h>
#include <dbDefs.h>
#include <dbScan.h>
#include <dbAccess.h>
#include <recSup.h>
#include <recGbl.h>
#include <devSup.h>
#include <devLib.h>
#include <drvSup.h>

#include <envDefs.h>
#include <alarm.h>
#include <link.h>
#include <special.h>
#include <cvtTable.h>
#include <registryFunction.h>
#include <genSubRecord.h>

#ifdef  __cplusplus
extern "C" {
#endif  /*      __cplusplus     */

extern int IPIMB_DRV_DEBUG;
extern int ipimbConfigureByName (char * ipimbName, uint16_t chargeAmpRange,
                                 uint16_t calibrationRange, uint32_t resetLength,
                                 uint16_t resetDelay, float chargeAmpRefVoltage,
                                 float calibrationVoltage, float diodeBias,
                                 uint16_t calStrobeLength, uint32_t trigDelay,
                                 uint32_t trigPsDelay, uint32_t adcDelay,
                                 DBLINK *trig);

long ipimbConfigInit(struct genSubRecord *psub)
{

    /* We don't do anything here because we concern any type, size and value change during fly */
    if (psub->tpro) printf("\n!!! ipimbConfigInit %s was called !!!\n", psub->name);

    /* We don't really need dpvt in this case */
    psub->dpvt = NULL;

    return 0;
}

long ipimbConfigProc(struct genSubRecord *psub)
{
    int rtn;
    /* We don't care link here, so we only care value, type and size */

#if 0
    /* Not really used, so let's avoid the warning */
    uint32_t triggerCountLow    = * ( (uint32_t *) (psub->a));
    uint32_t triggerCountHigh   = * ( (uint32_t *) (psub->b));
    uint32_t serialIDLow        =  * ( (uint32_t *) (psub->c));
    uint32_t serialIDHigh       = * ( (uint32_t *) (psub->d));
#endif
    uint16_t chargeAmpRange     = * ( (uint16_t *) (psub->e));
    uint16_t calibrationRange   = * ( (uint16_t *) (psub->f));
    uint32_t resetLength        = *( (uint32_t *) (psub->g) );
    uint16_t resetDelay         = * ((uint16_t *) (psub->h));
    float chargeAmpRefVoltage   = *( (float *) (psub->i) );
    float calibrationVoltage    = *( (float *) (psub->j) );
    float diodeBias             = *( (float *) (psub->k) );
#if 0
    /* Not really used, so let's avoid the warning */
    uint16_t status             = * ((uint16_t *) (psub->l));
    uint16_t errors             = * ((uint16_t *) (psub->m));
#endif
    uint16_t calStrobeLength    = * ((uint16_t *) (psub->n));
    uint32_t trigDelay          = * ((uint32_t *) (psub->o));
    uint32_t trigPsDelay        = * ((uint32_t *) (psub->p));
    uint32_t adcDelay           = *((uint32_t *) (psub->q));

    char * ipimbName  = (char *) (psub->u);

/*    if (psub->tpro) */
        printf("\n!!! ipimbConfigProc [%s] was called for ipimb box [%s] !!!\n", psub->name, ipimbName);

    /* get dataType from INPA and bkgdSubtract from INPC and gain from INPE */
    if(psub->fta != DBF_ULONG || psub->noa != 1 || psub->ftb != DBF_ULONG || psub->nob != 1 || psub->ftc != DBF_ULONG || psub->noc != 1 ||
       psub->ftd != DBF_ULONG || psub->nod != 1 || psub->fte != DBF_USHORT || psub->noe != 1 || psub->ftf != DBF_USHORT || psub->nof != 1 ||
       psub->ftg != DBF_ULONG || psub->nog != 1 || psub->fth != DBF_USHORT || psub->noh != 1 || psub->fti != DBF_FLOAT || psub->noi != 1 ||
       psub->ftj != DBF_FLOAT || psub->noj != 1 || psub->ftk != DBF_FLOAT || psub->nok != 1 || psub->ftl != DBF_USHORT || psub->nol != 1 ||
       psub->ftm != DBF_USHORT || psub->nom != 1 || psub->ftn != DBF_USHORT || psub->non != 1 || psub->fto != DBF_ULONG || psub->noo != 1 ||
       psub->ftp != DBF_ULONG || psub->nop != 1 || psub->ftq != DBF_ULONG || psub->noq != 1)
    {
        recGblSetSevr(psub, LINK_ALARM, INVALID_ALARM);
        printf("There is illegal input for INPs of [%s]!\n", psub->name);
        return -1;
    }
    /* no need to do value check */

    rtn = ipimbConfigureByName(ipimbName, chargeAmpRange, calibrationRange, resetLength,
                               resetDelay, chargeAmpRefVoltage, calibrationVoltage,
                               diodeBias, calStrobeLength, trigDelay, trigPsDelay,
                               adcDelay, &psub->inpr);

    return rtn;
}

#if EPICS_VERSION>=3 && EPICS_REVISION>=14
epicsRegisterFunction(ipimbConfigInit);
epicsRegisterFunction(ipimbConfigProc);
#endif

#ifdef  __cplusplus
}
#endif  /*      __cplusplus     */

