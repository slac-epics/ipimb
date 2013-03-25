/* ipimb.cpp */
/* Author:  pengs	 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dbAccess.h>
#include <longSubRecord.h>
#include "drvIpimb.h"
#include "evrTime.h"

#ifdef  __cplusplus
extern "C" {
#endif  /*      __cplusplus     */

extern int IPIMB_DRV_DEBUG;

static int IPIMB_device_list_inited = 0;
static IPIMB_DEVICE_LIST  IPIMB_device_list = { {NULL, NULL}, 0};
static epicsMutexId        IPIMB_device_list_lock = NULL;

int ipimbConfigureByName(char * ipimbName, uint16_t chargeAmpRange,
                         uint16_t calibrationRange, uint32_t resetLength,
                         uint16_t resetDelay, float chargeAmpRefVoltage, 
                         float calibrationVoltage, float diodeBias,
                         uint16_t calStrobeLength, uint32_t trigDelay,
                         uint32_t trigPsDelay, uint32_t adcDelay,
                         DBLINK *trig)
{
    IPIMB_DEVICE  * pdevice = NULL;

    if(ipimbName == NULL || !(pdevice = ipimbFindDeviceByName (ipimbName)) )
    {
        return -1;
    }

    pdevice->ipmBoard.SetTrigger(trig);

    Ipimb::ConfigV2 ipmConf(chargeAmpRange, calibrationRange, resetLength, resetDelay, chargeAmpRefVoltage,
                            calibrationVoltage, diodeBias, calStrobeLength, trigDelay, trigPsDelay, adcDelay);

    if(IPIMB_DRV_DEBUG)
    {
       printf("Dump current configuration\n");
       ipmConf.dump();
    }
    pdevice->ipmConfig = ipmConf;	/* save requested config */
    return pdevice->ipmBoard.configure(pdevice->ipmConfig);
}

/* This function is used to check if the ipimb box name already in our list */
IPIMB_DEVICE * ipimbFindDeviceByName(char * name)
{
    IPIMB_DEVICE  * pdevice = NULL;

    epicsMutexLock(IPIMB_device_list_lock);
    for(pdevice = (IPIMB_DEVICE *)ellFirst((ELLLIST *)&IPIMB_device_list);
        pdevice;
        pdevice = (IPIMB_DEVICE *)ellNext((ELLNODE *)pdevice))
    {
        if ( 0 == strcmp(name, pdevice->name) )   break;
    }
    epicsMutexUnlock(IPIMB_device_list_lock);

    return pdevice;
}

/* This function is used to check if the ttyName already in our list */
IPIMB_DEVICE * ipimbFindDeviceByTtyName(char * ttyName)
{
    IPIMB_DEVICE  * pdevice = NULL;

    epicsMutexLock(IPIMB_device_list_lock);
    for(pdevice = (IPIMB_DEVICE *)ellFirst((ELLLIST *)&IPIMB_device_list);
        pdevice;
        pdevice = (IPIMB_DEVICE *)ellNext((ELLNODE *)pdevice))
    {
        if ( 0 == strcmp(ttyName, pdevice->ttyName) )   break;
    }
    epicsMutexUnlock(IPIMB_device_list_lock);

    return pdevice;
}

extern "C" void BldRegister(unsigned int uPhysicalId, unsigned int uDataType, unsigned int pktsize,
                            int (*func)(int iPvIndex, void* pPvValue, void* payload));

static int ipimbSetPv(int iPvIndex, void* pPvValue, void* payload)
{
    if (iPvIndex) {
        double* pSrcPvValue = (double*) pPvValue;
        float* pDstPvValue = (float*)(sizeof(IpimBoardData) + sizeof(Ipimb::ConfigV2) + (char*) payload) + iPvIndex - 1;
#ifdef __rtems__
        /* Assume big endian! */
        union { float fval; unsigned int uval } tmp;
        tmp.fval = *pSrcPvValue;
        tmp.uval = (tmp.uval&0xFF)<<24    | (tmp.uval&0xFF00)<<8 |
                   (tmp.uval&0xFF0000)>>8 | (tmp.uval&0xFF000000)>>24;
        *pDstPvValue = tmp.fval;
#else
        /* Assume little endian! */
        *pDstPvValue = *pSrcPvValue;
#endif
    } else {
        char name[64];
        sscanf((char *)pPvValue, "@%[^:]:", name);
        IPIMB_DEVICE  *pdevice = ipimbFindDeviceByName(name);
        if (pdevice) {
            memcpy((char *)payload, &pdevice->ipmData, sizeof(IpimBoardData));
            memcpy(sizeof(IpimBoardData) + (char *)payload, &pdevice->ipmConfig, sizeof(Ipimb::ConfigV2));
        }
    }
    return 0;
}

int	 ipimbAdd(char *name, char *ttyName, char *mdestIP, unsigned int physID, unsigned int dtype,
                  char *trigger, int polarity)
{
    IPIMB_DEVICE  * pdevice = NULL;
    DBADDR trigaddr;
    static unsigned long ev140 = 140;

    if(!IPIMB_device_list_inited)
    {
        ellInit((ELLLIST *) & IPIMB_device_list);
        IPIMB_device_list_lock = epicsMutexMustCreate();

        IPIMB_device_list_inited = 1;
    }

    if(name == NULL || ipimbFindDeviceByName(name))
    {
        printf("ipimb box [%s] is already installed!\n", name);
        epicsThreadSuspendSelf();
    }

    if(ttyName == NULL || ipimbFindDeviceByTtyName(ttyName))
    {
        printf("Serial port [%s] is already in use!\n", ttyName);
        epicsThreadSuspendSelf();
    }

    // TODO: mdestIP needs to check and convert and setup
    if (dbNameToAddr(trigger, &trigaddr)) {
        printf("No PV trigger named %s, using constant event 140!\n", trigger);
        pdevice = new IPIMB_DEVICE(name, ttyName, mdestIP, physID, &ev140, &ev140, polarity);
    } else {
        unsigned long *trig = (unsigned long *) trigaddr.pfield;
        printf("Found PV trigger for IPIMB%d %s at %p (gen at %p)\n", 
               physID, trigger, trig, trig + MAX_EV_TRIGGERS);
        pdevice = new IPIMB_DEVICE(name, ttyName, mdestIP, physID,
                                   trig, trig + MAX_EV_TRIGGERS, polarity);
    }

    /* Add to the device linked list */
    epicsMutexLock(IPIMB_device_list_lock);
    ellAdd((ELLLIST *)&IPIMB_device_list, (ELLNODE *)pdevice);
    epicsMutexUnlock(IPIMB_device_list_lock);

    BldRegister(physID, dtype, sizeof(IpimBoardData) + sizeof(Ipimb::ConfigV2) + sizeof(Lusi::IpmFexV1),
                ipimbSetPv);

    printf( "ipimb box [%s] at [%s] added.\n", name, ttyName );
    return(0);
}

/**************************************************************************************************/
/* Here we supply the driver report function for epics                                            */
/**************************************************************************************************/
static  long    IPIMB_EPICS_Report(int level);

const struct drvet drvIPIMB = {2,                              /*2 Table Entries */
                              (DRVSUPFUN) IPIMB_EPICS_Report,  /* Driver Report Routine */
                              NULL}; /* Driver Initialization Routine */

epicsExportAddress(drvet,drvIPIMB);

/* implementation */
static long IPIMB_EPICS_Report(int level)
{
    IPIMB_DEVICE * pdevice;

    printf("\n"IPIMB_DRV_VERSION"\n\n");

    if(!IPIMB_device_list_inited)
    {
        printf("IPIMB linked list is not inited yet!\n\n");
        return 0;
    }

    if(level > 0)   /* we only get into link list for detail when user wants */
    {
        for(pdevice=(IPIMB_DEVICE *)ellFirst((ELLLIST *)&IPIMB_device_list); pdevice; pdevice = (IPIMB_DEVICE *)ellNext((ELLNODE *)pdevice))
        {
            printf("\tIPIMB box [%s] is installed on [%s]\n", pdevice->name, pdevice->ttyName);
        }
    }

    return 0;
}

#ifdef  __cplusplus
}
#endif  /*      __cplusplus     */
