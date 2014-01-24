#include "IpimBoard.hh"
#include "evrTime.h"
#include "unistd.h" //for sleep
#include <iostream>
#include "assert.h"
#include "memory.h"
// for non-blocking attempt
#include <fcntl.h>
#include <cmath>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#define BAD_VALUE 0xfadefade

using namespace std;
using namespace Pds;

const float CHARGEAMP_REF_MAX = 10.0;
const unsigned CHARGEAMP_REF_STEPS = 65536;

const float CALIBRATION_V_MAX = 10.0;
const unsigned CALIBRATION_V_STEPS = 65536;

const float INPUT_BIAS_MAX = 200;
const unsigned INPUT_BIAS_STEPS = 65536;

const int CLOCK_PERIOD = 8;
const float ADC_RANGE = 5.0;
const unsigned ADC_STEPS = 65536;

static const int HardCodedPresampleDelay = 150000;  // settling before presamples
static const int HardCodedAdcDelay = 4000; // time between presamples; this is the default on the board
static const int HardCodedTotalPresampleTime = (8+2)*HardCodedAdcDelay; // 8 presamples plus a bit extra
static const int OldHardCodedADCTime = 100000;
static const unsigned OldVhdlVersion = 0xDEADBEEF;
static const unsigned NewVhdlVersion = 0x00010000;

#define DEBUG_CRC      1
#define DEBUG_COMMAND  2
#define DEBUG_READER   4
#define DEBUG_REGISTER 8
#define DEBUG_SYNC     16
#define DEBUG_DATA     32
#define DEBUG_TC       64
#define DEBUG_TC_V     128
#define DEBUG_CMD      256
extern int IPIMB_BRD_DEBUG;
extern int IPIMB_BRD_ID;
#define DBG_ENABLED(flag)  ((IPIMB_BRD_ID == -1 || IPIMB_BRD_ID == _physID) && \
                            (IPIMB_BRD_DEBUG & (flag)))

static pthread_mutex_t trigger_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct trigger {
    char   name[256];
    int    use_cnt;
    long   saved_trigger;
    DBADDR db_trigger;
} triggers[32];
static int trigger_count = 0;

static unsigned CRC(unsigned short* lst, int length, int _physID)
{
    unsigned crc = 0xffff;
    for (int l=0; l<length; l++) {
        unsigned short word = *lst;
        if (DBG_ENABLED(DEBUG_CRC)) {
            printf("IPIMB%d: In CRC, have word 0x%04x\n", _physID, word);
        }
        lst++;
        // Calculate CRC 0x0421 (x16 + x12 + x5 + 1) for protocol calculation
        unsigned C[16]  = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        unsigned CI[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        unsigned D[16]  = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        for (int i=0; i<16; i++){
            C[i] = (crc & (1 << i)) >> i;
            D[i] = (word & (1 << i)) >> i;
        }
        CI[0] = D[12] ^ D[11] ^ D[8] ^ D[4] ^ D[0] ^ C[0] ^ C[4] ^ C[8] ^ C[11] ^ C[12];
        CI[1] = D[13] ^ D[12] ^ D[9] ^ D[5] ^ D[1] ^ C[1] ^ C[5] ^ C[9] ^ C[12] ^ C[13];
        CI[2] = D[14] ^ D[13] ^ D[10] ^ D[6] ^ D[2] ^ C[2] ^ C[6] ^ C[10] ^ C[13] ^ C[14];
        CI[3] = D[15] ^ D[14] ^ D[11] ^ D[7] ^ D[3] ^ C[3] ^ C[7] ^ C[11] ^ C[14] ^ C[15];
        CI[4] = D[15] ^ D[12] ^ D[8] ^ D[4] ^ C[4] ^ C[8] ^ C[12] ^ C[15];
        CI[5] = D[13] ^ D[12] ^ D[11] ^ D[9] ^ D[8] ^ D[5] ^ D[4] ^ D[0] ^ C[0] ^ C[4] ^  C[5] ^ C[8] ^ C[9] ^ C[11] ^ C[12] ^ C[13];
        CI[6] = D[14] ^ D[13] ^ D[12] ^ D[10] ^ D[9] ^ D[6] ^ D[5] ^ D[1] ^ C[1] ^ C[5] ^ C[6] ^ C[9] ^ C[10] ^ C[12] ^ C[13] ^ C[14];
        CI[7] = D[15] ^ D[14] ^ D[13] ^ D[11] ^ D[10] ^ D[7] ^ D[6] ^ D[2] ^ C[2] ^ C[6] ^ C[7] ^ C[10] ^ C[11] ^ C[13] ^ C[14] ^ C[15];
        CI[8] = D[15] ^ D[14] ^ D[12] ^ D[11] ^ D[8] ^ D[7] ^ D[3] ^ C[3] ^ C[7] ^ C[8] ^ C[11] ^ C[12] ^ C[14] ^ C[15];
        CI[9] = D[15] ^ D[13] ^ D[12] ^ D[9] ^ D[8] ^ D[4] ^ C[4] ^ C[8] ^ C[9] ^ C[12] ^ C[13] ^ C[15];
        CI[10] = D[14] ^ D[13] ^ D[10] ^ D[9] ^ D[5] ^ C[5] ^ C[9] ^ C[10] ^ C[13] ^ C[14];
        CI[11] = D[15] ^ D[14] ^ D[11] ^ D[10] ^ D[6] ^ C[6] ^ C[10] ^ C[11] ^ C[14] ^ C[15];
        CI[12] = D[15] ^ D[8] ^ D[7] ^ D[4] ^ D[0] ^ C[0] ^ C[4] ^ C[7] ^ C[8] ^ C[15];
        CI[13] = D[9] ^ D[8] ^ D[5] ^ D[1] ^ C[1] ^ C[5] ^ C[8] ^ C[9];
        CI[14] = D[10] ^ D[9] ^ D[6] ^ D[2] ^ C[2] ^ C[6] ^ C[9] ^ C[10];
        CI[15] = D[11] ^ D[10] ^ D[7] ^ D[3] ^ C[3] ^ C[7] ^ C[10] ^ C[11];
        crc = 0;
        for (int i=0; i<16; i++){
            crc = ((CI[i] <<i) + crc) & 0xffff;
        }
    }
    //  cout << "CRC calculates:" << crc << endl;
    return crc;
}

IpimBoardCommand::IpimBoardCommand(bool write, unsigned address, unsigned data, int _physID) 
{
    _commandList[0] = address & 0xFF;
    _commandList[1] = data & 0xFFFF;
    _commandList[2] = (data >> 16) & 0xFFFF;
    if (write) {
        _commandList[0] |= 1<<8;
    }
    _commandList[3] = CRC(_commandList, CRPackets-1, _physID);
    if (DBG_ENABLED(DEBUG_COMMAND))
        printf("IPIMB%d data: 0x%x, address 0x%x, write %d, command list: 0x%x, 0x%x, 0x%x 0x%x\n", 
               _physID, data, address, write, 
               _commandList[0], _commandList[1], _commandList[2], _commandList[3]);
}

IpimBoardCommand::~IpimBoardCommand()
{
    //  delete[] _lst;
}

unsigned short* IpimBoardCommand::getAll()
{
    return _commandList;
}

IpimBoardResponse::IpimBoardResponse(unsigned short* packet) 
{
    _addr = packet[0] & 0xFF;
    _data = packet[1] | ((unsigned)packet[2]<<16);
    _checksum = packet[3];
    for (int i = 0; i < CRPackets; i++)
        _respList[i] = packet[i];
}

IpimBoardResponse::~IpimBoardResponse() 
{
}

unsigned IpimBoardResponse::getData() 
{
    return _data;
}

unsigned IpimBoardResponse::getAddr() 
{
    return _addr;
}

static void *ipimb_thread_body(void *p)
{
    IpimBoard *ipimb = (IpimBoard *)p;
    struct sched_param param;

    param.sched_priority = 95;
    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &param)) {
        printf("pthread_setschedparam failed with errno=%d?!?\n", errno);
        fflush(stdout);
    }
    ipimb->do_read();
    return NULL;
}

static void *ipimb_configure_body(void *p)
{
    IpimBoard *ipimb = (IpimBoard *)p;

    ipimb->do_configure();
    return NULL;
}

IpimBoard::IpimBoard(char* serialDevice, IOSCANPVT *ioscan, int physID, unsigned long* trigger,
                     unsigned long *gen, int polarity)
{
    size_t stacksize;
    struct termios newtio;
    pthread_attr_t attr;

    _physID = physID;
    _serialDevice = strdup(serialDevice);
    _ioscan = ioscan;
    _trigger = trigger;
    _gen = gen;
    _polarity = polarity;

    memset(&newtio, 0, sizeof(newtio));
        
    /* open the device to be blocking */
    _fd = open(serialDevice, O_RDWR | O_NOCTTY);
    if (_fd <0) {
        perror(serialDevice); exit(-1); 
    }

#if 1
    newtio.c_iflag = 0;
#else
    newtio.c_iflag = IGNPAR;        /* Ignore parity errors */
#endif
    newtio.c_oflag = 0;
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD; /* Set baudrate, 8 bits, ignore modem control, enable receiver */
    newtio.c_lflag = 0;         /* Canonical mode is off, so no special characters */
    newtio.c_cc[VMIN]=1;
    newtio.c_cc[VTIME]=0;
    flush();
    tcsetattr(_fd,TCSANOW,&newtio);
  
    memset(_cmd, 0, sizeof(_cmd));
    memset(_data, 0, sizeof(_data));
    cbuf = 0;
    dbuf = 0;
    have_data = 0;

    conf_in_progress = false;
    config_ok = false;
    config_gen = 0;
    trigger_index = -1;
    trigger_user  = 0;

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cmdready, NULL);
    pthread_cond_init(&confreq, NULL);
    pthread_attr_init(&attr);
    pthread_attr_getstacksize(&attr, &stacksize);
    have_reader = !pthread_create(&reader, &attr, ipimb_thread_body, (void *)this);
    have_configurer = !pthread_create(&configurer, &attr, ipimb_configure_body, (void *)this);
}

IpimBoard::~IpimBoard() 
{
    close(_fd);
    delete _serialDevice;
    if (have_reader)
        pthread_kill(reader, SIGKILL);
    if (have_configurer)
        pthread_kill(configurer, SIGKILL);
}

int IpimBoard::get_fd()
{
    return _fd;
}

void IpimBoard::flush() 
{
    if (tcflush(_fd, TCIOFLUSH)) {
        printf("IPIMB%d cannot flush!!!\n", _physID);
        fflush(stdout);
    }
}

int IpimBoard::qlen() 
{
    int len;
    if (ioctl(_fd, FIONREAD, &len))
        return -1;
    else
        return len;
}

void IpimBoard::do_read()
{
    int rd, len, blen, i, j, cmd;
    unsigned short *p;
    unsigned crc;
    unsigned char rdbuf[3 * DataPackets];
    uint64_t expected_cnt = 1, current_cnt;
    unsigned long long idx = -1;
    int incr = 1, in_sync = 0;
    epicsTimeStamp evt_time;
    int status = 0;
    int trigevent = *_trigger;
    unsigned int gen = *_gen;
    int eventvalid = trigevent > 0 && trigevent < 256;
    int do_print = 0;
    int did_skip = 0;
    
    if (eventvalid)
        printf("IPIMB@%d: Setting event trigger to %d\n", _physID, trigevent);
    else
        printf("IPIMB@%d: Event trigger %d is invalid!\n", _physID, trigevent);
    
    for (;;) {
        rd = read(_fd, rdbuf, 1);
        if (rd <= 0 || !SOFR(rdbuf[0])) { /* Skip until we are in sync.  Do we ever want to quit?!? */
            if (rd > 0 && (DBG_ENABLED(DEBUG_READER|DEBUG_SYNC))) {
                did_skip = 1;
                fprintf(stderr, "IPIMB%d: Ser R: 0x%02x (skipped)\n", _physID, rdbuf[0]);
                fflush(stderr);
            }
            continue;
        }

        if (DBG_ENABLED(DEBUG_TC_V) && did_skip) {
            printf("IPIMB%d skipped!\n", _physID);
            fflush(stdout);
        }
        did_skip = 0;
        if (DBG_ENABLED(DEBUG_TC_V) && !COMMAND(rdbuf[0])) {
            printf("IPIMB%d data read @ fid 0x%x\n", _physID, lastfid);
            fflush(stdout);
        }

        if (gen != *_gen) {
            /* The trigger event changed, so print a message and force a resync! */
            trigevent = *_trigger;
            gen = *_gen;
            in_sync = 0;

            if (eventvalid) {
                eventvalid = trigevent > 0 && trigevent < 256;
                if (eventvalid)
                    printf("IPIMB@%d: Setting event trigger to %d\n", _physID, trigevent);
                else
                    printf("IPIMB@%d: Event trigger %d is invalid!\n", _physID, trigevent);
            } else {
                eventvalid = trigevent > 0 && trigevent < 256;
                if (eventvalid)
                    printf("IPIMB@%d: Setting event trigger to %d\n", _physID, trigevent);
            }
            fflush(stdout);
        }

        /*
         * At this point, we have one byte of a packet.
         *
         * If we are in sync, then we just go process it.
         *
         * If we are not in_sync and the event isn't valid, we can also go process it, because
         * either it's data (which we'll drop), or it's a command response (which we need to
         * send back).
         *
         * If we are not in sync and the event is valid, we need to get into sync, so:
         *     - We drop what we have (incrementing expected_cnt if it's data).
         *     - We wait for the next data to arrive and get the time from lastfid.
         *     - We get the current time.  This should be shortly before the lastfid time.  If
         *       not, adjust backwards or wait, accordingly!
         *
         * If the user tries to reconfigure or if we receive a bad fiducial, we just flush and
         * start over.
         */
        if (!in_sync && eventvalid) {
            int savefid, newfid, cnt;

            if (DBG_ENABLED(DEBUG_TC)) {
                printf("IPIMB%d resynchronizing at actual fiducial 0x%x.\n",
                       _physID, lastfid);
                fflush(stdout);
            }

            /* Get rid of our current packet! */
            flush();
            if (!COMMAND(rdbuf[0])) {
                if (DBG_ENABLED(DEBUG_TC)) {
                    printf("IPIMB%d is flushing expected data packet 0x%llx.\n",
                           _physID, (long long unsigned int) expected_cnt);
                    fflush(stdout);
                }
                expected_cnt++;
            }

            savefid = lastfid; 

            /* Wait for either a reconfigure or something new to appear in the queue */
            for (cnt = 1; cnt < 5000; cnt++) {
                struct timespec req = {0, 1000000}; /* 1 ms */
                nanosleep(&req, NULL);
                if (gen != *_gen)
                    break;
                if (qlen()) {
                    newfid = lastfid;
                    /*
                     * The fastest we go is 120Hz (3 fiducials).  If we see our "next"
                     * packet in less than 2 fiducials, we're really seeing more of the
                     * SAME packet.  Therefore, just flush again!
                     */
                    if (FID_DIFF(newfid, savefid) <= 2) {
                        if (DBG_ENABLED(DEBUG_TC)) {
                            printf("IPIMB%d double flush?!?\n", _physID);
                            fflush(stdout);
                        }
                        flush();
                    } else
                        break;
                }
            }

            if (cnt == 5000 || gen != *_gen) {
                /* This is just bad.  Flush and hope for better the next time we're here. */
                if (DBG_ENABLED(DEBUG_TC)) {
                    printf("IPIMB%d resync failed, restarting!\n", _physID);
                    fflush(stdout);
                }
                flush();
                continue;
            }
            /*
             * lastfid is saved in the event interrupt handler.  It is probably the
             * most accurate timestamp we have.  It should be "close" to the trigger
             * timestamp.
             */
            savefid = lastfid; 

            /* Get the current time! */
            status = evrTimeGetFifo(&evt_time, trigevent, &idx, MAX_TS_QUEUE);
            newfid = evt_time.nsec & 0x1ffff;
            if (newfid == 0x1ffff) {
                if (DBG_ENABLED(DEBUG_TC)) {
                    printf("IPIMB%d: Bad fiducial at time %08x:%08x!\n", _physID,
                           evt_time.secPastEpoch, evt_time.nsec);
                    fflush(stdout);
                }
                continue;
            }

            if (DBG_ENABLED(DEBUG_TC)) {
                printf("IPIMB%d resync sees timestamp fiducial 0x%x at actual fiducial 0x%x.\n",
                       _physID, newfid, savefid);
                fflush(stdout);
            }

            if (FID_GT(newfid, savefid)) {
                /*
                 * The timestamp is more recent than the most recent fiducial?!?
                 *
                 * We must have prematurely rotated the timestamp buffers.  As long
                 * as we are close, nothing is seriously wrong here.
                 */
                if (FID_DIFF(newfid, savefid) >= 2) {
                    printf("IPIMB%d is looking into the distant future?!?\n", _physID);
                    fflush(stdout);
                    /*
                     * I'd write code to fix this, but we should never be
                     * here in the first place!
                     */
                    flush();
                    continue;
                }
            } else {
                /*
                 * Our trigger was close to savefid, but the timestamps have not been rotated
                 * yet.  Wait for them to rotate until we get something close!
                 */
                while (FID_DIFF(savefid, newfid) > 2) {
                    unsigned long long idx2;
                    if (DBG_ENABLED(DEBUG_TC)) {
                        printf("IPIMB%d has savefid=0x%x, newfid=0x%x, idx=%lld.  Waiting...\n",
                               _physID, savefid, newfid, idx);
                        fflush(stdout);
                    }
                    do
                        status = evrTimeGetFifo(&evt_time, trigevent, &idx2, MAX_TS_QUEUE);
                    while (idx == idx2 || gen != *_gen);
                    idx = idx2;
                    newfid = evt_time.nsec & 0x1ffff;
                    if (gen != *_gen || newfid == 0x1ffff) {
                        if (DBG_ENABLED(DEBUG_TC)) {
                            printf("IPIMB%d loop broken by reconfig or bad fiducial, restarting.\n",
                                   _physID);
                            fflush(stdout);
                        }
                        break;
                    } else {
                        if (DBG_ENABLED(DEBUG_TC)) {
                            printf("IPIMB%d loop has idx=%lld, newfid=0x%x\n", 
                                   _physID, idx, newfid);
                            fflush(stdout);
                        }
                    }
                }
                if (gen != *_gen || newfid == 0x1ffff) {
                    /* This is just bad.  Flush and hope for better the next time we're here. */
                    if (DBG_ENABLED(DEBUG_TC)) {
                        printf("IPIMB%d resync failed with fiducial 0x%x, restarting!\n",
                               _physID, (evt_time.nsec & 0x1ffff));
                        fflush(stdout);
                    }
                    flush();
                    continue;
                }
            }
            /*
             * We should probably check that we are, indeed, close.  What if newfid skipped
             * *past* our real fiducial?  (Unlikely, I know.)
             */
            if (DBG_ENABLED(DEBUG_TC)) {
                printf("IPIMB%d resync established with index %lld at timestamp fiducial 0x%x at actual fiducial 0x%x.\n",
                       _physID, idx, newfid, savefid);
                fflush(stdout);
            }
            in_sync = 1;
            expected_cnt++;
            flush();
            do_print = 1; /* Talk the next time through the loop! */
            continue;
        }

        /* Read the entire packet */
        cmd = COMMAND(rdbuf[0]);
        len = cmd ? CRPackets : DataPackets;
        blen = 3 * len;
        do {
            int newrd = read(_fd, &rdbuf[rd], blen - rd);
            if (newrd > 0)
                rd += newrd;
        } while (rd != blen);

        /* Decode it! */
        if (!EOFR(rdbuf[blen - 3])) {/* No end of frame?!? */
            continue;
        }
        p = cmd ? _cmd[cbuf] : _data[dbuf];
        for (i = 0, j = 0; i < len; i++, j += 3) {
            if (DBG_ENABLED(DEBUG_READER)) {
                fprintf(stderr, "IPIMB%d: Ser R: 0x%02x 0x%02x 0x%02x --> 0x%04x\n", _physID,
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
                fprintf(stderr, "IPIMB%d: Skipping to next SOF.\n", _physID);
            }
            continue;
        }

        /* Check the CRC! */
        crc = CRC(p, len - 1, _physID);
        if (crc != p[len - 1]) {
            if (DBG_ENABLED(DEBUG_READER|DEBUG_SYNC|DEBUG_CRC)) {
                printf("IPIMB%d: data CRC check failed (got 0x%04x wanted 0x%04x)\n",
                       _physID, p[len - 1], crc);
            }
            continue;
        }

        /* Deliver it! */
        if (cmd) {
            pthread_mutex_lock(&mutex);
            cbuf = cbuf ? 0 : 1;
            pthread_cond_signal(&cmdready); /* Wake up whoever did the read! */
            if (DBG_ENABLED(DEBUG_READER|DEBUG_DATA)) {
                fprintf(stderr, "IPIMB%d: Got command response packet (gen=%d, _gen=%ld, trigevent=%d, _trigger=%ld, eventvalid=%d).\n",
                        _physID, gen, *_gen, trigevent, *_trigger, eventvalid);
            }
            pthread_mutex_unlock(&mutex);
        } else {
            IpimBoardData *d = new (_data[dbuf]) IpimBoardData();
            d->adjustdata(_polarity);

            if (DBG_ENABLED(DEBUG_READER|DEBUG_DATA)) {
                fprintf(stderr, "IPIMB%d: Got data packet in %d.\n", _physID, dbuf);
            }

            /* Check if this is the next expected data packet. */
            current_cnt = d->triggerCounter();
            if (current_cnt != expected_cnt) {
                if (DBG_ENABLED(DEBUG_TC)) {
                    printf("IPIMB%d trigger count: expected 0x%llx, have 0x%llx.\n", 
                           _physID, (long long unsigned int)expected_cnt,
                           (long long unsigned int) current_cnt);
                    fflush(stdout);
                }
                incr = current_cnt - expected_cnt + 1;
                if (incr <= 0 || incr > 5) {
                    /* If we are too far off, just start over! */
                    if (DBG_ENABLED(DEBUG_TC) && current_cnt != 1) {
                        printf("IPIMB%d: resynchronizing timestamps!\n", _physID);
                        fflush(stdout);
                    }
                    in_sync = 0;
                }
            } else
                incr = 1;
            expected_cnt = current_cnt + incr;

            /* If we're really in sync, get the time and deliver the data! */
            if (in_sync) {
                status = evrTimeGetFifo(&ts[dbuf], trigevent, &idx, incr);
                if (status) {
                    if (DBG_ENABLED(DEBUG_TC)) {
                        printf("IPIMB%d has an invalid timestamp, resynching!\n", _physID);
                        fflush(stdout);
                    }
                    in_sync = 0;
                    do_print = 0;
                }
                if (do_print) {
                    unsigned int newfid = ts[dbuf].nsec & 0x1ffff;
                    if (newfid == 0x1ffff) {
                        if (DBG_ENABLED(DEBUG_TC)) {
                            printf("IPIMB%d has a bad fiducial (lastfid = 0x%x), resynching!\n",
                                   _physID, lastfid);
                            fflush(stdout);
                        }
                        in_sync = 0;
                    } else if (FID_DIFF(lastfid, ts[dbuf].nsec & 0x1ffff) >= 2) {
                        /*
                         * We're way off, when we didn't expect to be.  Let's do this
                         * again, shall we?
                         */
                        if (DBG_ENABLED(DEBUG_TC)) {
                            printf("IPIMB%d recynch failed with index %lld at fiducial 0x%x (lastfid = 0x%x), current packet=0x%llx, retrying!\n",
                                   _physID, idx, ts[dbuf].nsec & 0x1ffff, lastfid, 
                                   (long long unsigned int) current_cnt);
                            fflush(stdout);
                        }
                        in_sync = 0;
                    } else {
                        /* Success!!! */
                        if (DBG_ENABLED(DEBUG_TC)) {
                            printf("IPIMB%d is fully resynched with index %lld at fiducial 0x%x (lastfid = 0x%x), current packet=0x%llx.\n",
                                   _physID, idx, ts[dbuf].nsec & 0x1ffff, lastfid, 
                                   (long long unsigned int) current_cnt);
                            fflush(stdout);
                        }
                    }
                    do_print = 0;
                } else {
                    if (DBG_ENABLED(DEBUG_TC_V)) {
                        printf("IPIMB%d: idx %lld, fid 0x%x, pkt 0x%llx.\n",
                               _physID, idx, ts[dbuf].nsec & 0x1ffff,
                               (long long unsigned int) current_cnt);
                        fflush(stdout);
                    }
                }
            }

            if (in_sync) {
                if (ts[0].nsec == ts[1].nsec && (ts[0].nsec & 0x1ffff) != 0x1ffff) {
                    printf("IPIMB@%d: TS[0] = %08x.%08x, TS[1] = %08x.%08x, idx=%lld, incr=%d, status=%d!\n",
                           _physID, ts[0].secPastEpoch, ts[0].nsec, ts[1].secPastEpoch, ts[1].nsec,
                           idx, incr, status);
                    fflush(stdout);
                }
                dbuf = dbuf ? 0 : 1;
                have_data = 1;
                scanIoRequest(*_ioscan);         /* Let'em know we have data! */
            }
        }
    }
}

int IpimBoard::WriteCommand(unsigned short* cList)
{
    unsigned char wrdata[3*CRPackets];   // Buffer for writing

    for (int i=0; i<CRPackets; i++) {
        unsigned data = *cList++;
        wrdata[3*i  ] = 0x90 | (data & 0xf) | (i == 0 ? 0x40 : 0) | (i == 3 ? 0x20 : 0);
        wrdata[3*i+1] = (data>>4) & 0x3f;
        wrdata[3*i+2] = 0x40 | ((data>>10) & 0x3f);
        if (DBG_ENABLED(DEBUG_COMMAND)) {
            printf("Out: 0x%04x %c%c%c\n", data,
                   (wrdata[3*i]&0x40) ? 'S' : ' ', (wrdata[3*i]&0x20) ? 'E' : ' ', (wrdata[3*i]&0x10) ? 'C' : ' ');
            printf("Ser W: 0x%02x 0x%02x 0x%02x\n", wrdata[3*i], wrdata[3*i+1], wrdata[3*i+2]);
        }
    }

    int res = write(_fd, wrdata, 12);
    if (res != 12) {
        printf( "write failed: %d bytes of 12 written\n", res);
        return -1;
    }
    return 0;
}

#define REGNAME(n) (((n) > vhdl_version) ? "???" : regnames[n])
static char *regnames[] = {
    "timestamp0",
    "timestamp1",
    "serid0",
    "serid1",
    "adc0",
    "adc1",
    "rg_config",
    "cal_rg_config",
    "reset",
    "bias_data",
    "cal_data",
    "biasdac_data_config",
    "status",
    "errors",
    "cal_strobe",
    "trig_delay",
    "trig_ps_delay",
    "adc_delay",
    "???",
    "???",
    "???",
    "???",
    "???",
    "vhdl_version",
};

unsigned IpimBoard::ReadRegister(unsigned regAddr)
{
    IpimBoardCommand cmd = IpimBoardCommand(false, regAddr, 0, _physID);
    struct timeval tp;
    struct timespec ts;
    int do_write = 1, i;

    pthread_mutex_lock(&mutex);
    for (i = 0; i < 5; i++) {
        if (do_write && WriteCommand(cmd.getAll())) {
            pthread_mutex_unlock(&mutex);
            return BAD_VALUE;
        }
        gettimeofday(&tp, NULL);
        ts.tv_sec  = tp.tv_sec + 5;                /* 5s timeout!!! */
        ts.tv_nsec = tp.tv_usec * 1000 + 25000000; /* 25ms timeout */
        if (pthread_cond_timedwait(&cmdready, &mutex, &ts) == ETIMEDOUT) {
            do_write = 1;
            continue;
        }
        IpimBoardResponse resp = IpimBoardResponse(_cmd[cbuf ? 0 : 1]);
        if (resp.getAddr() == regAddr) {
            unsigned result = resp.getData();
            if (DBG_ENABLED(DEBUG_REGISTER)) {
                printf("ReadRegister(%s) --> 0x%x (%d)\n", REGNAME(regAddr), result, result);
            }
            pthread_mutex_unlock(&mutex);
            return result;
        } else {
            i--; /* Don't count this, it was a previous read! */
            do_write = 0;
        }
    }
    pthread_mutex_unlock(&mutex);
    return BAD_VALUE;
}

void IpimBoard::WriteRegister(unsigned regAddr, unsigned regValue)
{
    IpimBoardCommand cmd = IpimBoardCommand(true, regAddr, regValue, _physID);
    WriteCommand(cmd.getAll());
    struct timespec req = {0, 5000000}; // 5 ms
    nanosleep(&req, NULL);
    if (DBG_ENABLED(DEBUG_REGISTER)) {
        printf("WriteRegister(%s, 0x%x) done.\n", REGNAME(regAddr), regValue);
    }
}

IpimBoardData *IpimBoard::getData(epicsTimeStamp *t)
{
    if (!have_data)
        return NULL;
    else {
        int which = dbuf ? 0 : 1;
        *t = ts[which];
        return new (_data[which]) IpimBoardData();
    }
}

bool IpimBoard::configure(Ipimb::ConfigV2& config)
{
    pthread_mutex_lock(&mutex);
    newconfig = config;
    config_gen++;
    conf_in_progress = true;
    /* Check if we are using the trigger data structure already! */
    pthread_mutex_lock(&trigger_mutex);
    if (trigger_user == 0) {
        if (triggers[trigger_index].use_cnt++ == 0) {
            /* If we're the first one in, save the trigger state and turn it off! */
            long options = 0, nRequest = 1, newval = 0;
            dbGetField(&triggers[trigger_index].db_trigger, DBR_LONG, 
                       &triggers[trigger_index].saved_trigger, &options, &nRequest, NULL);
            dbPutField(&triggers[trigger_index].db_trigger, DBR_LONG, &newval, 1);
        }
        trigger_user = 1;
    }
    pthread_mutex_unlock(&trigger_mutex);
    pthread_cond_signal(&confreq); /* Wake up the configuration task */
    pthread_mutex_unlock(&mutex);
    return true;
}

void IpimBoard::do_configure()
{
    int current_gen = 0;    /* Must match initial value of config_gen! */
    Ipimb::ConfigV2 config;

    for (;;) {
        pthread_mutex_lock(&mutex);
        while (current_gen == config_gen) {
            conf_in_progress = false;
            pthread_cond_wait(&confreq, &mutex);
        }
        current_gen = config_gen;
        config = newconfig;
        pthread_mutex_unlock(&mutex);

        config_ok = true;
    
        WriteRegister(errors, 0xffff);
        unsigned vhdlVersion = ReadRegister(vhdl_version);

        if (vhdlVersion == BAD_VALUE) {
            printf("read of board failed - check serial and power connections of fd %d, device %s\nGiving up on configuration\n", _fd, _serialDevice);
            config_ok = false;
            continue;
        }

        // hope to never want to calibrate in the daq environment
        bool lstCalibrateChannels[4] = {false, false, false, false};
        SetCalibrationMode(lstCalibrateChannels);
  
        uint64_t tcnt = config.triggerCounter();
        unsigned start0 = (unsigned) ((0xFFFFFFFF00000000ULL&tcnt)>>32);
        unsigned start1 = (unsigned) (0xFFFFFFFF&tcnt);
        SetTriggerCounter(start0, start1);

        SetChargeAmplifierRef(config.chargeAmpRefVoltage());

        if (vhdlVersion != NewVhdlVersion) {
            printf("Expected vhdl version 0x%x, found 0x%x, old version is 0x%x\n", NewVhdlVersion, vhdlVersion, OldVhdlVersion);
            config_ok = false;
        }
        unsigned multiplier = (unsigned) config.chargeAmpRange();
        unsigned mult_chan0 = ((multiplier&0xf)+1)%0xf;  // Board interprets 0 as no capacitor
        unsigned mult_chan1 = (((multiplier>>4)&0xf)+1)%0xf;
        unsigned mult_chan2 = (((multiplier>>8)&0xf)+1)%0xf;
        unsigned mult_chan3 = (((multiplier>>12)&0xf)+1)%0xf;
        printf("Configuring gain with %d, %d, %d, %d\n", mult_chan0, mult_chan1, mult_chan2, mult_chan3);
        if(mult_chan0*mult_chan1*mult_chan2*mult_chan3 == 0) {
            printf("Do not understand gain configuration 0x%x, byte values restricted to 0-14\n", multiplier);
            config_ok = false;
        }
        unsigned inputAmplifier_pF[4] = {mult_chan0, mult_chan1, mult_chan2, mult_chan3};
        SetChargeAmplifierMultiplier(inputAmplifier_pF);
        //  SetCalibrationVoltage(float calibrationVoltage);
        SetInputBias(config.diodeBias());
        SetChannelAcquisitionWindow(config.resetLength(), config.resetDelay());
        SetTriggerDelay((unsigned) config.trigDelay());
        SetTriggerPreSampleDelay(HardCodedPresampleDelay);
        printf("Have hardcoded presample delay to %d us\n", HardCodedPresampleDelay/1000);
        if ((HardCodedPresampleDelay+HardCodedTotalPresampleTime)>config.trigDelay()) {
            config_ok = false;
            printf("Sample delay %d is too short - must be at least %d earlier than %d\n", config.trigDelay(), HardCodedTotalPresampleTime, HardCodedPresampleDelay);
        }
        SetAdcDelay(HardCodedAdcDelay);
        printf("Have hardcoded adc delay to %f us\n", float(HardCodedAdcDelay)/1000);
        //  CalibrationStart((unsigned) config.calStrobeLength());

        config.setTriggerPreSampleDelay(HardCodedPresampleDelay);
        config.setAdcDelay(HardCodedAdcDelay);

        config.setSerialID(GetSerialID());
        config.setErrors(GetErrors());
        config.setStatus((ReadRegister(status) & 0xffff0000)>>16);
  
        config.dump();

        printf("have flushed fd after IpimBoard configure\n");
    }
}

void IpimBoard::SetTriggerCounter(unsigned start0, unsigned start1)
{
    WriteRegister(timestamp0, start0);
    WriteRegister(timestamp1, start1);
}

unsigned IpimBoard::GetTriggerCounter1()
{
    return ReadRegister(timestamp1);
}

uint64_t IpimBoard::GetSerialID() 
{  
    uint32_t id0 = ReadRegister(serid0);
    uint32_t id1 = ReadRegister(serid1);
    uint64_t id = ((uint64_t)id0<<32) + id1;
    //  printf("id0: 0x%lx, id1: 0x%lx, id: 0x%llx\n", id0, id1, id);
    return id;
}

uint16_t IpimBoard::GetErrors() {
    return ReadRegister(errors);
}

uint16_t IpimBoard::GetStatus() {
    return ReadRegister(status)>>16;
}

void IpimBoard::SetCalibrationMode(bool* channels)
{
    unsigned val = ReadRegister(cal_rg_config);
    //  printf("have retrieved config val 0x%x\n", val);
    val &= 0x7777;
    int shift = 3;
    for (int i=0; i<4; i++) {
        bool b = channels[i];
        if (b) 
            val |= 1<<shift;
        shift += 4;
    }
    WriteRegister(cal_rg_config, val);
}

void IpimBoard::SetCalibrationDivider(unsigned* channels) 
{
    unsigned val = ReadRegister(cal_rg_config);
    val &= 0xcccc;
    int shift = 0;
    for (int i=0; i<4; i++) {
        unsigned setting = channels[i];
        if (setting>2) {
            printf("IpimBoard error: Cal divider setting %d is illegal\n",setting);
            config_ok = false;
        }
        else 
            val |= (setting+1)<<shift;
        shift+=4;
    }
    WriteRegister(cal_rg_config, val);
}

void IpimBoard::SetCalibrationPolarity(bool* channels) 
{
    unsigned val = ReadRegister(cal_rg_config);
    val &= 0xbbbb;
    int shift = 2;
    for (int i=0; i<4; i++) {
        bool b = channels[i];
        if (b)
            val |= 1<<shift;
        shift+=4;
    }
    WriteRegister(cal_rg_config, val);
}

void IpimBoard::SetChargeAmplifierRef(float refVoltage)
{
    //Adjust the reference voltage for the charge amplifier
    unsigned i = unsigned((refVoltage/CHARGEAMP_REF_MAX)*(CHARGEAMP_REF_STEPS-1));
    if (i >= CHARGEAMP_REF_STEPS) {
        printf("IpimBoard error: calculated %d charge amp ref steps, allow %d; based on requested voltage %f\n", i, CHARGEAMP_REF_STEPS, refVoltage);
        //    _damage |= 1<<ampRef_damage; 
        config_ok = false;
        return;
    }
    WriteRegister(bias_data, i);
}

void IpimBoard::SetCalibrationVoltage(float calibrationVoltage) 
{
    unsigned i = unsigned((calibrationVoltage/CALIBRATION_V_MAX)*(CALIBRATION_V_STEPS-1));
    if (i >= CALIBRATION_V_STEPS) {
        printf("IpimBoard error: invalid calibration bias of %fV, max is %fV", calibrationVoltage, CALIBRATION_V_MAX);
        config_ok = false;
        return;
        //_damage |= calVoltage_damage;
    }
    //raise RuntimeError, 
    WriteRegister(cal_data, i);
    struct timespec req = {0, 10000000}; // 10 ms
    nanosleep(&req, NULL);
}

void IpimBoard::SetChargeAmplifierMultiplier(unsigned* channels) 
{
    unsigned val = 0;
    for (int i=0; i<4; i++) {
        val |= channels[i]<<i*4;
    }
    WriteRegister(rg_config, val);
}

void IpimBoard::oldSetChargeAmplifierMultiplier(unsigned* channels) 
{
    unsigned val = ReadRegister(rg_config);
    val &= 0x8888;
    int shift = 0;
    for (int i=0; i<4; i++) {
        unsigned setting = channels[i];
        if (setting<=1)
            val |= 4<<shift;
        else if (setting<=100)
            val |= 2<<shift;
        else if (setting<=10000)
            val |= 1<<shift;
        shift+=4;
    }
    WriteRegister(rg_config, val);
}

void IpimBoard::SetInputBias(float biasVoltage) 
{
    unsigned i = unsigned((biasVoltage/INPUT_BIAS_MAX)*(INPUT_BIAS_STEPS-1));
    if (i >= INPUT_BIAS_STEPS) {
        printf("IpimBoard error: calculated input bias setting register setting %d, allow %d; based on requested voltage %f\n", i, INPUT_BIAS_STEPS, biasVoltage);
        config_ok = false;
        return;
        //    _damage |= 1<<diodeBias_damage;
    }
    unsigned originalSetting = ReadRegister(biasdac_data_config);
    WriteRegister(biasdac_data_config, i);
    if (i != originalSetting) {
        printf("Have changed input bias setting from 0x%x to 0x%x, pausing to allow diode bias to settle\n", (int)originalSetting, i);
        struct timespec req = {0, 999999999}; // 1 s
        for (int j=0; j<5; j++) {
            nanosleep(&req, NULL);
        }
    }
}

void IpimBoard::SetChannelAcquisitionWindow(uint32_t acqLength, uint16_t acqDelay) 
{
    uint32_t length = (acqLength+CLOCK_PERIOD-1)/CLOCK_PERIOD;
    if (length > 0xfffff) {
        printf("IpimBoard error: acquisition window cannot be more than %dns\n", (0xfffff*CLOCK_PERIOD));
        config_ok = false;
        return;
        //    _damage |= 1 << acqWindow_damage;
    } 
    unsigned delay = (acqDelay+CLOCK_PERIOD-1)/CLOCK_PERIOD;
    if (delay > 0xfff) {
        printf("IpimBoard warning: acquisition window cannot be delayed more than %dns\n", 0xfff*CLOCK_PERIOD);
        config_ok = false;
        return;
    }
    WriteRegister(reset, (length<<12) | (delay & 0xfff));
}

void IpimBoard::SetTriggerDelay(uint32_t triggerDelay) 
{
    unsigned delay = (triggerDelay+CLOCK_PERIOD-1)/CLOCK_PERIOD;
    if (delay > 0xffff) {
        printf("IpimBoard warning: trigger delay cannot be more than %dns\n", 0xffff*CLOCK_PERIOD);
        config_ok = false;
        return;
    }
    WriteRegister(trig_delay, delay);
}

void IpimBoard::SetAdcDelay(uint32_t adcDelay) 
{
    unsigned delay = (adcDelay+CLOCK_PERIOD-1)/CLOCK_PERIOD;
    if (delay > 0xffff) {
        printf("IpimBoard warning: adc delay cannot be more than %dns\n", 0xffff*CLOCK_PERIOD);
        config_ok = false;
        return;
    }
    WriteRegister(adc_delay, delay);
}

void IpimBoard::SetTriggerPreSampleDelay(uint32_t triggerPsDelay)
{
    unsigned delay = (triggerPsDelay+CLOCK_PERIOD-1)/CLOCK_PERIOD;
    if (delay > 0xffff) {
        printf("IpimBoard warning: trigger presample delay cannot be more than %dns\n", 0xffff*CLOCK_PERIOD);
        config_ok = false;
        return;
    }
    WriteRegister(trig_ps_delay, delay);
}

void IpimBoard::CalibrationStart(unsigned calStrobeLength) 
{
    unsigned length = (calStrobeLength+CLOCK_PERIOD-1)/CLOCK_PERIOD;
    if (length > 0xffff) {
        printf("IpimBoard error: strobe cannot be more than %dns", 0xffff*CLOCK_PERIOD);
        config_ok = false;
        return;
    }
    WriteRegister(cal_strobe, length);
}

void IpimBoard::SetTrigger(DBLINK *trig)
{
    int i;
    char *name;

    if (trigger_index < 0) {
        if (trig->type != DB_LINK) {
            printf("IpimBoard error: trigger link does not point to PV?\n");
            return;
        }
        name = trig->value.pv_link.pvname;
        pthread_mutex_lock(&trigger_mutex);
        for (i = 0; i < trigger_count; i++)
            if (!strcmp(name, triggers[i].name))
                break;
        if (i == trigger_count) { /* It's new! */
            strcpy(triggers[i].name, name);
            triggers[i].use_cnt = 0;
            triggers[i].saved_trigger = -1;
            dbNameToAddr(triggers[i].name, &triggers[i].db_trigger);
            trigger_count++;
        }
        trigger_index = i;
        pthread_mutex_unlock(&trigger_mutex);
    }
}

/* We assume that we already have the IpimBoard mutex here! */
void IpimBoard::RestoreTrigger(void)
{
    pthread_mutex_lock(&trigger_mutex);
    if (--triggers[trigger_index].use_cnt == 0) {
        /* Last one out, turn off the lights! */
        dbPutField(&triggers[trigger_index].db_trigger, DBR_LONG,
                   &triggers[trigger_index].saved_trigger, 1);
    }
    trigger_user = 0;
    pthread_mutex_unlock(&trigger_mutex);
}
