/* ipimb.cpp */
/* Author:  pengs	 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "drvIpimb.h"

#ifdef  __cplusplus
extern "C" {
#endif  /*      __cplusplus     */

extern int IPIMB_DRV_DEBUG;

static int IPIMB_device_list_inited = 0;
static IPIMB_DEVICE_LIST  IPIMB_device_list = { {NULL, NULL}, 0};
static epicsMutexId        IPIMB_device_list_lock = NULL;


static void IPIMB_Receive_Task(IPIMB_DEVICE  * pdevice)
{
    int jj;
    while(TRUE)
    {
        if(pdevice->requestConf) // if request re-configuration
        {
            epicsMutexLock(pdevice->mutex_lock);
            printf("Start to do configuration for [%s]\n", pdevice->name);
            /* Sigh.  These are not initialized by the configuration!!! */
            pdevice->ipmBoard.setBaselineSubtraction(0, 1);
            pdevice->ipmBoard.configure(pdevice->ipmConfig);

            struct timespec req = {0, 3000000000}; // 3000 ms
            nanosleep(&req, NULL);

            pdevice->confState = 1;
            pdevice->requestConf = 0;
            printf("Finish doing configuration for [%s]\n", pdevice->name);

            epicsMutexUnlock(pdevice->mutex_lock);
        }
        else if(pdevice->confState) // if configured
        {
            if(IPIMB_DRV_DEBUG) printf("waiting data ...\n");
            //epicsMutexLock(pdevice->mutex_lock);  //no lock due to the risk of dead loop
        
            pdevice->ipmData = pdevice->ipmBoard.WaitData();
            //TODO: get timestamp and pulseID
            //TODO: BLD
            //TODO: Calculate Fex
            if(IPIMB_DRV_DEBUG && jj++ == 120)
            {
               jj = 0;
               pdevice->ipmData.dumpRaw();
            }

            epicsTimeStamp evt_time;
            epicsTimeGetEvent(&evt_time, 1);
            //printf("nsec is 0x%08X\n", evt_time.nsec);
            scanIoRequest(pdevice->ioscan);

            //epicsMutexUnlock(pdevice->mutex_lock);
        }
        struct timespec req = {0, 3000000}; // 3 ms
        nanosleep(&req, NULL);
    }
}

/* Configure board immediately */
int ipimbConfigure(IPIMB_DEVICE  * pdevice)
{
    bool rtn;
    if(IPIMB_DRV_DEBUG)
    {
       pdevice->ipmConfig.dump();
    }
    epicsMutexLock(pdevice->mutex_lock);
    rtn = pdevice->ipmBoard.configure(pdevice->ipmConfig);

    struct timespec req = {0, 300000000}; // 300 ms
    nanosleep(&req, NULL);
    pdevice->confState = 1;
    epicsMutexUnlock(pdevice->mutex_lock);
    return rtn;
}

int ipimbConfigureByName(char * ipimbName, uint16_t chargeAmpRange, uint16_t calibrationRange, uint32_t resetLength, uint16_t resetDelay,
                           float chargeAmpRefVoltage, float calibrationVoltage, float diodeBias,
                          uint16_t calStrobeLength, uint32_t trigDelay, uint32_t trigPsDelay, uint32_t adcDelay)
{
    bool rtn;
    IPIMB_DEVICE  * pdevice = NULL;

    if(ipimbName == NULL || !(pdevice = ipimbFindDeviceByName (ipimbName)) )
    {
        return -1;
    }

    Pds::Ipimb::ConfigV2 ipmConf(chargeAmpRange, calibrationRange, resetLength, resetDelay, chargeAmpRefVoltage, calibrationVoltage, diodeBias,
                          calStrobeLength, trigDelay, trigPsDelay, adcDelay);

    if(IPIMB_DRV_DEBUG)
    {
       printf("Dump current configuration\n");
       ipmConf.dump();
    }
#if 0
    epicsMutexLock(pdevice->mutex_lock);
    pdevice->ipmConfig = ipmConf;
    printf("start to do configuration\n");
    rtn = pdevice->ipmBoard.configure(ipmConf);

    struct timespec req = {0, 300000000}; // 300 ms
    nanosleep(&req, NULL);
    pdevice->confState = 1;
    printf("finish doing configuration\n");
    epicsMutexUnlock(pdevice->mutex_lock);
#else
    epicsMutexLock(pdevice->mutex_lock);
    pdevice->ipmConfig = ipmConf;	/* save requested config */
    pdevice->requestConf = 1;	/* put request so we have no mutex issue between configure and reading */
    epicsMutexUnlock(pdevice->mutex_lock);
#endif
    return rtn;
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

int	 ipimbAdd(char * name, char *ttyName, char * mdestIP)
{
    IPIMB_DEVICE  * pdevice = NULL;

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
    pdevice = new IPIMB_DEVICE(name, ttyName, mdestIP);

    /* Add to the device linked list */
    epicsMutexLock(IPIMB_device_list_lock);
    ellAdd((ELLLIST *)&IPIMB_device_list, (ELLNODE *)pdevice);
    epicsMutexUnlock(IPIMB_device_list_lock);


    /* Create thread */
    epicsThreadMustCreate("ipimb_Rcvr", OPTHREAD_PRIORITY, OPTHREAD_STACK, (EPICSTHREADFUNC)IPIMB_Receive_Task, (void *)pdevice);

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

