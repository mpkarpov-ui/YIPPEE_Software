#pragma once
/**
 * This class is meant wrap the memory chip and define
 * the logging structure for th enetire program
*/ 
#include <Arduino.h>
#include "pins.h"
#include "SST26VF040A.h"
#include "SPI.h"

#define FLASH_PAGE_SIZE 256

enum LOGGER_DATA_LIMITS {
    LOGGER_DATA_MIN_F0 = 0x100,
    LOGGER_DATA_MAX_F0 = 0x40000,
    LOGGER_DATA_MIN_F1 = 0x40000,
    LOGGER_DATA_MAX_F1 = 0x80000
};

struct LoggerStruct {
    int32_t gps_altitude;
    float baro_altitude;
    float temp;
    float pres;
    uint32_t timestamp;
    int32_t lat;
    int32_t lon;

    // Store all other data in a single uint32_t.
    uint32_t aux_data;
    // FSM_STATE  SIV    FIX_TYPE  FLIGHT_NO  TELEM_CHANNEL   <UNUSED>
    // 00         00000  000       0          0000            0 0000 0000 0000 0000
    // 01         23456  789       A          BCDE            F 0123 4567 89AB CDEF
};

struct FlashSettings {
    int last_written_section = 0;
    uint32_t flight0_start_time = 0;
    uint32_t flight1_start_time = 0;
    char telem_channel = '0';
};

class Logger {
    public:
        Logger(uint8_t cs_pin, uint8_t hold_pin, uint8_t wp_pin);

        bool begin();

        bool begin_readonly();

        void write(LoggerStruct data);
        void commit_settings(FlashSettings* data);

        // Returns true if the data was read successfully, false otherwise.
        bool read_raw(uint8_t* data, uint32_t addr, uint32_t len);
        bool read(LoggerStruct* data, uint32_t addr);
        bool read_settings(FlashSettings* data);

        // Returns true if the data was read successfully, false otherwise. Checks if the addr is within the flight range.
        bool read_flightlocked(LoggerStruct* data, uint32_t addr, int flight_num);

        void change_state(int s);

        void reset_flight(int flight_no = 0);

        FlashSettings& get_settings() {
            return settings_;
        }

    private:
        void write_packet(LoggerStruct data); 
        void clear_first_sector(bool fast_erase = false);

        uint8_t cs, hold, wp;

        SST26VF040A flash;

        uint32_t addr;
        long packets_written;

        int last_written_section;
        int oldest_written_section;

        uint32_t data_addr_min;
        uint32_t data_addr_max;

        LoggerStruct packet_array[8];
        int buf_idx;

        FlashSettings settings_;

        int state;
};