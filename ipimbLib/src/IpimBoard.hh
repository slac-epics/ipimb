#ifndef PDS_IPIMBOARD
#define PDS_IPIMBOARD

#include <arpa/inet.h>

#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <time.h>
#include <dbScan.h>
#include <epicsThread.h>
#include <epicsTime.h>
#include <dbAccess.h>

#include "ConfigV2.hh"
#include "timesync.h"

#define IPIMB_Q_SIZE 8
#define IPIMB_Q_MASK 7

#define BAUDRATE B115200
/*
 * All data is transmitted in three byte chunks which contain two bytes of data:
 *      1 SOF EOF Command D[3:0]
 *      0 0 D[9:4]
 *      0 1 D[15:10]
 */
#define FIRST(b)   (((b) & 0x80) == 0x80)
#define SECOND(b)  (((b) & 0xc0) == 0x00)
#define THIRD(b)   (((b) & 0xc0) == 0x40)
#define SOFR(b)    (((b) & 0xc0) == 0xc0)
#define EOFR(b)    (((b) & 0xa0) == 0xa0)
#define COMMAND(b) ((b) & 0x10)
/*
 * Command/Response are four-word structures:
 *      Zero(7) W(1) Addr[7:0]
 *      Data[15:0]
 *      Data[31:16]
 *      Checksum[15:0]
 */
#define CRPackets 4
/*
 * Original data packets had the form:
 *      triggerCounter[63:48]
 *      triggerCounter[47:32]
 *      triggerCounter[31:16]
 *      triggerCounter[15:0]
 *      config
 *      ch0
 *      ch1
 *      ch2
 *      ch3
 *      checksum
 *
 * Now, however, data packets are fixed 16 word structures:
 *      triggerCounter[63:48]
 *      triggerCounter[47:32]
 *      triggerCounter[31:16]
 *      triggerCounter[15:0]
 *      config0
 *      config1
 *      config2
 *      ch0
 *      ch1
 *      ch2
 *      ch3
 *      ch0_ps
 *      ch1_ps
 *      ch2_ps
 *      ch3_ps
 *      checksum
 */
#define DataPackets 16

static const int NChannels = 4;
static const int BufferLength = 100;

namespace Pds {
    class IpimBoardData {
    public:
        void* operator new (size_t sz, void* v) {
            return v;
        }
        IpimBoardData() {};
        uint64_t triggerCounter() {
            return (((uint64_t)tc[0]) << 48) | (((uint64_t)tc[1]) << 32) | (((uint64_t)tc[2]) << 16) | tc[3];
        }
        void adjustdata(int polarity) {
            /*
             * Long explanation of what is going on here...
             *
             * When the DAQ converts from IpimBoardPsData to IpimBoardData,
             * if baselinesubtraction is 1 (the default), it calculates:
             *
             *     diff = max(0, int(-1*polarity*presample + polarity*samplechannel[i]))
             *     if (polarity == 1)
             *         return diff;
             *     else
             *         return 65535 - diff
             *
             * Now, we can simplify this somewhat:
             *     max(0, int(-1*polarity*presample + polarity*samplechannel[i])) = 
             *     max(0, polarity * int(samplechannel[i] - presample))
             * So if polarity == 1, we return:
             *     max(0, int(samplechannel[i] - presample)) = 
             *     (presample < samplechannel[i]) ? (samplechannel[i] - presample) : 0
             * And if polarity == -1, we return:
             *     65535 - max(0, -1 * int(samplechannel[i] - presample))
             *     65535 + min(0, int(samplechannel[i] - presample)) =
             *     (presample > samplechannel[i]) ? (65535 + samplechannel[i] - presample) : 65535
             *
             * If polarity == 0, we just skip all of this mess and don't do any
             * baseline subtraction at all.
             */
            if (!polarity)
                return;
            if (polarity == -1) {
                if (ch0_ps > ch0)
                    ch0 = 65535 + ch0 - ch0_ps;
                else
                    ch0 = 65535;
                if (ch1_ps > ch1)
                    ch1 = 65535 + ch1 - ch1_ps;
                else
                    ch1 = 65535;
                if (ch2_ps > ch2)
                    ch2 = 65535 + ch2 - ch2_ps;
                else
                    ch2 = 65535;
                if (ch3_ps > ch3)
                    ch3 = 65535 + ch3 - ch3_ps;
                else
                    ch3 = 65535;
            } else {
                if (ch0_ps < ch0)
                    ch0 = ch0 - ch0_ps;
                else
                    ch0 = 0;
                if (ch1_ps < ch1)
                    ch1 = ch1 - ch1_ps;
                else
                    ch1 = 0;
                if (ch2_ps < ch2)
                    ch2 = ch2 - ch2_ps;
                else
                    ch2 = 0;
                if (ch3_ps < ch3)
                    ch3 = ch3 - ch3_ps;
                else
                    ch3 = 0;
            }
        }
        uint16_t tc[4];
        uint16_t config0;
        uint16_t config1;
        uint16_t config2;
        uint16_t ch0;
        uint16_t ch1;
        uint16_t ch2;
        uint16_t ch3;
        uint16_t ch0_ps;
        uint16_t ch1_ps;
        uint16_t ch2_ps;
        uint16_t ch3_ps;
        uint16_t checksum;
    };

    class IpimBoardCommand {
    public:
        IpimBoardCommand(bool write, unsigned address, unsigned data, int _physID);
        ~IpimBoardCommand();
        unsigned short* getAll();
    
    private:
        unsigned short _commandList[CRPackets];
    };

    class IpimBoardResponse {
    public:
        IpimBoardResponse(unsigned short* packet);
        ~IpimBoardResponse();
    
        unsigned getData();
        unsigned getAddr();
    private:
        unsigned _addr;
        unsigned _data;
        unsigned _checksum;
        unsigned short _respList[CRPackets];
    };

    class IpimBoard {
        friend class ipimbSyncObject;
    public:
        enum {
            timestamp0          = 0x00,
            timestamp1          = 0x01,
            serid0              = 0x02,
            serid1              = 0x03,
            adc0                = 0x04,
            adc1                = 0x05,
            rg_config           = 0x06,     // 16 bit!
            cal_rg_config       = 0x07,     // 16 bit!
            reset               = 0x08,     // 31:12 - assert, 11:0 - delay
            bias_data           = 0x09,     // 16 bit!
            cal_data            = 0x0a,     // 16 bit!
            biasdac_data_config = 0x0b,     // 16 bit!
            status              = 0x0c,     // 16 bit!
            errors              = 0x0d,     // 16 bit!
            cal_strobe          = 0x0e,     // 16 bit!
            trig_delay          = 0x0f,     // 16 bit!
            trig_ps_delay       = 0x10,
            adc_delay           = 0x11,
            vhdl_version        = 0x17
        };

        IpimBoard(char* serialDevice, IOSCANPVT *ioscan, int physID, epicsUInt32 *trigger,
                  epicsUInt32 *gen, int polarity, char *delay, char *name, char *sync);
        ~IpimBoard();

        void do_configure(void);  // The configure thread body!

        int WriteCommand(unsigned short*);
        void SendCommandResponse(void);
        unsigned ReadRegister(unsigned regAddr);
        void WriteRegister(unsigned regAddr, unsigned regValue);

        void sendData(epicsTimeStamp &t);
        IpimBoardData *getData(epicsTimeStamp *t);

        bool configure(Ipimb::ConfigV2& config);

        void SetTriggerCounter(unsigned start0, unsigned start1);
        unsigned GetTriggerCounter1();
        void SetCalibrationMode(bool*);
        void SetCalibrationDivider(unsigned*);
        void SetCalibrationPolarity(bool*);
        void SetChargeAmplifierRef(float refVoltage);
        void SetCalibrationVoltage(float calibrationVoltage);
        void SetChargeAmplifierMultiplier(unsigned*);
        void oldSetChargeAmplifierMultiplier(unsigned*);
        void SetInputBias(float biasVoltage);
        void SetChannelAcquisitionWindow(uint32_t acqLength, uint16_t acqDelay);
        void SetTriggerDelay(uint32_t triggerDelay);
        void SetTriggerPreSampleDelay(uint32_t triggerPreSampleDelay);
        void CalibrationStart(unsigned calStrobeLength);
    
        uint64_t GetSerialID();
        uint16_t GetStatus();
        uint16_t GetErrors();
        void SetAdcDelay(uint32_t adcDelay);

        bool isConfiguring(void) { return conf_in_progress; }
        bool isConfigOK(void)    { return config_ok; }
        void lock(void)          { pthread_mutex_lock(&mutex); }
        void unlock(void)        { pthread_mutex_unlock(&mutex); }
        static void ipimbStart(void) { if (!started) { epicsMutexUnlock(start_mutex); started = true; }; }

        void SetTrigger(DBLINK *trig);
        void RestoreTrigger(void);

        unsigned short *GetBuffer(int cmd) { return cmd ? _cmd[cwr & IPIMB_Q_MASK]
                                                        : _data[dwr & IPIMB_Q_MASK]; }
    
    private:
        char* _serialDevice;         // Name of serial port
        int   _polarity;             // -1 = negative-going, 1 = positive-going,
                                     //  0 = no baseline subtraction.
        IOSCANPVT *_ioscan;
        bool  config_ok;
        static bool  started;
        
        epicsTimeStamp ts[IPIMB_Q_SIZE];
        int have_data;

        static epicsMutexId trigger_mutex;
        static epicsMutexId start_mutex;
        pthread_mutex_t mutex;       // MCB - Sigh. EPICS seems to willfully misunderstand
                                     // the use of locks with pthread_cond_t.
        pthread_cond_t  cmdready;
        pthread_cond_t  confreq;
        epicsThreadId reader;
        epicsThreadId configurer;
        bool conf_in_progress;
        Ipimb::ConfigV2 newconfig;
        int config_gen;
        int trigger_index;
        int trigger_user;
    protected:
        int   _physID;               // Physical ID
        char *_name;
        int   _fd;                   // File descriptor
        epicsUInt32*  _trigger;      // PV with event number of trigger
        epicsUInt32*  _gen;          // PV with generation number of trigger
        char *_delay;                // PV with acquisition delay estimate
        unsigned short _cmd[IPIMB_Q_SIZE][CRPackets];
        unsigned short _data[IPIMB_Q_SIZE][DataPackets];
        unsigned long long crd, cwr;
        unsigned long long drd, dwr;
        char *_syncpv;
    };

    class ipimbSyncObject : public SyncObject {
    public:
        ipimbSyncObject(IpimBoard *_ipimb);
        ~ipimbSyncObject()                 {};
        int Init(void);
        DataObject *Acquire(void);
        int CheckError(DataObject *dobj);
        const char *Name(void)             { return ipimb->_name; }
        int Attributes(void)               { return HasCount; }
        int CountIncr(DataObject *dobj);
        void QueueData(DataObject *dobj, epicsTimeStamp &evt_time);
        void DebugPrint(DataObject *dobj);
    private:
        IpimBoard *ipimb;
        uint64_t   lasttrig;         /* Count of the last successful Acquire. */
        int        curincr;          /* Event increment between the last two successful Acquires. */
        int        _physID;          /* Physical ID */
    };
}

#define DEBUG_CRC      1
#define DEBUG_COMMAND  2
#define DEBUG_READER   4
#define DEBUG_REGISTER 8
#define DEBUG_SYNC     16
#define DEBUG_DATA     32
#define DEBUG_TC       64
#define DEBUG_TC_V     128
#define DEBUG_CMD      256
extern "C" int IPIMB_BRD_DEBUG;
extern "C" int IPIMB_BRD_ID;
#define DBG_ENABLED(flag)  ((IPIMB_BRD_ID == -1 || IPIMB_BRD_ID == _physID) && \
                            (IPIMB_BRD_DEBUG & (flag)))

unsigned CRC(unsigned short* lst, int length, int _physID);

#endif
