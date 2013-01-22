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
#include <epicsTime.h>

#include "ConfigV2.hh"

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
        void adjustdata() {
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

        IpimBoard(char* serialDevice, IOSCANPVT *ioscan, int physID, unsigned long *trigger,
                  unsigned long *gen);
        ~IpimBoard();

        int get_fd();
        void flush();
        int  qlen();

        void do_read(void);       // The main thread body!
        void do_configure(void);  // The configure thread body!

        int WriteCommand(unsigned short*);
        unsigned ReadRegister(unsigned regAddr);
        void WriteRegister(unsigned regAddr, unsigned regValue);

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
    
    private:
        int   _physID;               // Physical ID
        int   _fd;                   // File descriptor
        char* _serialDevice;         // Name of serial port
        unsigned long*  _trigger;    // PV with event number of trigger
        unsigned long*  _gen;        // PV with generation number of trigger
        IOSCANPVT *_ioscan;
        bool  config_ok;
        
        unsigned short _cmd[2][CRPackets];
        unsigned short _data[2][DataPackets];
        int cbuf, dbuf;
        int cidx, didx;
        epicsTimeStamp ts[2];
        int have_data;

        pthread_mutex_t mutex;
        pthread_cond_t  cmdready;
        pthread_cond_t  confreq;
        pthread_t reader;
        pthread_t configurer;
        int have_reader;
        int have_configurer;
        bool conf_in_progress;
        Ipimb::ConfigV2 newconfig;
        int config_gen;
    };
}
#endif
