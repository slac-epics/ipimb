#ifndef	drv_Ipimb_H
#define drv_Ipimb_H

/*
 *	Author:  pengs
 *
 *	Abstract:
 *		Provide EPICS support for LCLS IPIMB boxes
 */

#include <epicsVersion.h>
#if EPICS_VERSION>=3 && EPICS_REVISION>=14

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
#include <epicsExport.h>

#include <stringinRecord.h>
#include <aiRecord.h>
#include <longinRecord.h>
#include <biRecord.h>

#else
#error "You need EPICS 3.14 or above because we need OSI support!"
#endif

#include "IpimBoard.hh"
#include "ConfigV2.hh"
// #include "DataV2.hh"  // it seems in IpimBoard.hh, there is already data definition
#include "IpmFexConfigV2.hh"
#include "IpmFexV1.hh"

/*
 * Constants
 */

#define IPIMB_DRV_VERSION "IPIMB Driver Version 1.0"

/******************************************************************************************/
/* We have an thread running to receive all IPIMB packets.                               */
/* It then validates the packet and either puts into or appends to the link list          */
/******************************************************************************************/
#define OPTHREAD_PRIORITY       (90)            /* opthread Priority */
#define OPTHREAD_STACK          (0x20000)       /* opthread Stack Size */
/******************************************************************************************/

/******************************************************************************************/
#define MAX_CA_STRING_SIZE      (40)

typedef ELLLIST IPIMB_DEVICE_LIST;

/* IPIMB device information */
using namespace Pds;

class IPIMB_DEVICE
{
public:
    ELLNODE          node;              /* Linked List Node */

    char             * name;		/* name of IPIMB box */
    char             * ttyName;		/* ttyName of the serial port where IPIMB box is */
    char             * mdestIPString;   /* xxx.xxx.xxx.xxx:ppppp, max 22 bytes, for multicast dest */
    struct in_addr   ipaddr;            /* IP address, we don't keep name here since converting to name may block receive task */

    epicsTimeStamp   timestamp;         /* timestamp for the latest data */
    IOSCANPVT        ioscan;            /* Trigger EPICS record */

    Ipimb::ConfigV2  ipmConfig; 
    IpimBoard	     ipmBoard;
    IpimBoardData    ipmData;

public:
    IPIMB_DEVICE(char * ipmName, char *ipmTtyName, char * ipmMdestIP, int physID, unsigned long *trigger,
                 unsigned long *gen)
        : ipmBoard(ipmTtyName, &ioscan, physID, trigger, gen), ipmData()
    {
        name = epicsStrDup(ipmName);
        ttyName = epicsStrDup(ipmTtyName);
        mdestIPString = epicsStrDup(ipmMdestIP);
        scanIoInit(&ioscan);
    }
    ~IPIMB_DEVICE()
    {
        free(name);
        free(ttyName);
        free(mdestIPString);
    }

private:

};

#ifdef	__cplusplus
extern "C" {
#endif	/*	__cplusplus	*/


/*
 * Function declarations
 */

int ipimbConfigure(IPIMB_DEVICE  * pdevice);
int ipimbConfigureByName(char * ipimbName, uint16_t chargeAmpRange, 
                         uint16_t calibrationRange, uint32_t resetLength, 
                         uint16_t resetDelay, float chargeAmpRefVoltage, 
                         float calibrationVoltage, float diodeBias,
                         uint16_t calStrobeLength, uint32_t trigDelay, 
                         uint32_t trigPsDelay, uint32_t adcDelay,
                         DBLINK *trig);
IPIMB_DEVICE * ipimbFindDeviceByName(char * name);
IPIMB_DEVICE * ipimbFindDeviceByTtyName(char * ttyName);
    int		ipimbAdd(char * name, char * ttyName, char * mdestIP, unsigned int physID,
                         unsigned int dtype, char *trigger );

#ifdef	__cplusplus
}
#endif	/*	__cplusplus	*/
#endif	/*	ipimbH	*/
