/**
 * This class is meant wrap the memory chip and define
 * the logging structure for th enetire program
*/ 
#include <Arduino.h>
#include "pins.h"
#include "SST26VF040A.h"
#include "SPI.h"

struct LoggerStruct {
    float gps_altitude;
    float baro_altitude;
    float temp;
    float pres;
    uint32_t timestamp;
    uint32_t lat;
    uint32_t lon;

    // Store all other data in a single uint32_t.
    uint32_t aux_data;
    // FSM_STATE  SIV    FIX_TYPE  FLIGHT_NO  CHANNEL <UNUSED>
    // 00         00000  000       0          0000     0 0000 0000 0000 0000
    // 01         23456  789       A          BCDE     F 0123 4567 89AB CDEF

};

class Logger {
    public:
        Logger(uint8_t cs_pin, uint8_t hold_pin, uint8_t wp_pin);

        bool begin();

        bool begin_readonly();

        void write(LoggerStruct data);
        void read(LoggerStruct* data, uint32_t addr);

        void change_state(int s);

        void reset_flight();


    private:
        void write_packet(LoggerStruct data); 

        uint8_t cs, hold, wp;

        SST26VF040A flash;

        uint32_t addr;
        long packets_written;

        int last_written_section;

        uint32_t data_addr_min;
        uint32_t data_addr_max;

        LoggerStruct packet_array[8];
        int buf_idx;

        int state;
};