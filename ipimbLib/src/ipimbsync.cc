#include"IpimBoard.hh"
#include"evrTime.h"

using namespace std;
using namespace Pds;

ipimbSyncObject::ipimbSyncObject(IpimBoard *_ipimb)
{
    DBADDR delayaddr;
    static double statdelay = 0.0;
    double *pdelay;

    ipimb = _ipimb;
    lasttrig = 0;
    curincr = 1;
    _physID = _ipimb->_physID;

    epicsMutexLock(ipimb->start_mutex);
    // OK, let's go!
    epicsMutexUnlock(ipimb->start_mutex);

    if (dbNameToAddr(ipimb->_delay, &delayaddr)) {
        printf("No delay PV named %s, using constant 0!\n", ipimb->_delay);
        pdelay = &statdelay;
    } else {
        pdelay = (double *) delayaddr.pfield;
    }

	epicsUInt32		timingMode	= 0; 	// 0=LCLS1, 1=LCLS2
    SetParams( &timingMode, ipimb->_trigger, ipimb->_gen, pdelay, ipimb->_syncpv);
}

// Get some data and return it encapsulated.
DataObject *ipimbSyncObject::Acquire(void)
{
    int rd, cmd, len, blen;
    unsigned short *p;
    unsigned char rdbuf[3 * DataPackets];
    unsigned int crc = 0;
    int did_skip = 0;
    int i, j;

    for (;;) {
        /*
         * Look for a start of frame.
         */
        rd = read(ipimb->_fd, rdbuf, 1);
        if (rd <= 0 || !SOFR(rdbuf[0])) {
            if (rd > 0 && (DBG_ENABLED(DEBUG_READER|DEBUG_SYNC))) {
                fprintf(stderr, "IPIMB %s: Ser R: 0x%02x (skipped)\n", ipimb->_name, rdbuf[0]);
                fflush(stderr);
            }
            did_skip = 1;
            continue;
        }
        if (DBG_ENABLED(DEBUG_TC_V) && did_skip) {
            printf("IPIMB %s skipped!\n", ipimb->_name);
            fflush(stdout);
        }
        did_skip = 0;
        if (DBG_ENABLED(DEBUG_TC_V) && !COMMAND(rdbuf[0])) {
            printf("IPIMB %s data read @ fid 0x%x\n", ipimb->_name, lastfid);
            fflush(stdout);
        }

        /*
         * Read the entire packet, either command or data.
         */
        cmd = COMMAND(rdbuf[0]);
        len = cmd ? CRPackets : DataPackets;
        blen = 3 * len;
        do {
            int newrd = read(ipimb->_fd, &rdbuf[rd], blen - rd);
            if (newrd > 0)
                rd += newrd;
        } while (rd != blen);
        if (!EOFR(rdbuf[blen - 3]))
            continue;

        /*
         * Decode the packet into an IpimBoard buffer.
         */
        p = ipimb->GetBuffer(cmd);
        for (i = 0, j = 0; i < len; i++, j += 3) {
            if (DBG_ENABLED(DEBUG_READER)) {
                fprintf(stderr, "IPIMB %s: Ser R: 0x%02x 0x%02x 0x%02x --> 0x%04x\n", ipimb->_name,
                        rdbuf[j+0], rdbuf[j+1], rdbuf[j+2],
                        (rdbuf[j+0] & 0xf) | ((rdbuf[j+1] & 0x3f)<<4) | ((rdbuf[j+2] & 0x3f)<<10));
            }
            if (!FIRST(rdbuf[j]) || !SECOND(rdbuf[j+1]) || !THIRD(rdbuf[j+2]))
                break;
            if (COMMAND(rdbuf[j]) != cmd)
                break;
            p[i] = (rdbuf[j] & 0xf) | ((rdbuf[j+1] & 0x3f)<<4) | ((rdbuf[j+2] & 0x3f)<<10);
        }
        if (i != len) {
            if (DBG_ENABLED(DEBUG_READER|DEBUG_SYNC)) {
                fprintf(stderr, "IPIMB %s: Skipping to next SOF.\n", ipimb->_name);
            }
            continue;
        }

        /*
         * Check the CRC!
         */
        crc = CRC(p, len - 1, _physID);
        if (crc != p[len - 1]) {
            if (DBG_ENABLED(DEBUG_READER|DEBUG_SYNC|DEBUG_CRC)) {
                printf("IPIMB %s: data CRC check failed (got 0x%04x wanted 0x%04x)\n",
                       ipimb->_name, p[len - 1], crc);
            }
            continue;
        }

        /* Deliver it! */
        if (cmd)
            ipimb->SendCommandResponse();
        else {
            IpimBoardData *d = new (p) IpimBoardData();
            d->adjustdata(ipimb->_polarity);

            if (DBG_ENABLED(DEBUG_READER|DEBUG_DATA)) {
                fprintf(stderr, "IPIMB %s: Got data packet.\n", ipimb->_name);
            }

            uint64_t curtrig = d->triggerCounter();
            curincr = curtrig - lasttrig;
            lasttrig = curtrig;

            return new DataObject(d);
        }
    }
}

// Check for errors or other conditions.  Return 0 if OK, -1 if we need a resync.
int ipimbSyncObject::CheckError(DataObject *dobj)
{
    return 0;
}

int ipimbSyncObject::CountIncr(DataObject *dobj)
{
    return curincr;
}

// Receive the data and timestamp.
void ipimbSyncObject::QueueData(DataObject *dobj, epicsTimeStamp &evt_time)
{
    ipimb->sendData(evt_time);
}

void ipimbSyncObject::DebugPrint(DataObject *dobj)
{
}
