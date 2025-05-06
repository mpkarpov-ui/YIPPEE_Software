#pragma once
// This file lets us offload the memory from the onboard flash chip onto our computer for data processing.
// This runs instead of the main function, and pre-empts the flight logic.
#include <Arduino.h>
#include <SPI.h>
#include "logger.h"
#include "pins.h"
#include "buzzer.h"


uint32_t STAT_LED_FLASH = 0;
bool STAT_LED_FLASH_STATE = true;
void YIPPEE_MEM_OFFLOAD_SETUP(Logger* mem) {

    // Set up all pins
    pinMode(BUZZER, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(MEM_CS, OUTPUT);
    pinMode(MEM_HOLD, OUTPUT);
    pinMode(MEM_WRITE_PROTECT, OUTPUT);

    digitalWrite(LED_BLUE, HIGH);
    BUZZ_INIT();
    Serial.begin(9600);
    while(!Serial) {
        if(millis() > STAT_LED_FLASH) {
            STAT_LED_FLASH = millis() + 500;
            STAT_LED_FLASH_STATE = !STAT_LED_FLASH_STATE;
            digitalWrite(LED_BLUE, STAT_LED_FLASH_STATE ? HIGH : LOW);
        }
    }
    digitalWrite(LED_BLUE, LOW);
    Serial.println("======= YIPPEE MEM OFFLOAD =======");
    Serial.println("YIPPEE Serial detected!");
    Serial.println("Initializing ...");

    // Flash init
    SPI.begin();
    digitalWrite(MEM_HOLD, HIGH);
    digitalWrite(MEM_WRITE_PROTECT, HIGH);
    bool flash_init_stat = mem->begin_readonly();

    // Fail loudly.
    if(!flash_init_stat) {
        Serial.println("Failed to init Flash!");
        digitalWrite(LED_BLUE, HIGH); // Flashing blue is flash failure
        BUZZ_FAIL_MEM();

        while(1) {
            digitalWrite(LED_BLUE, HIGH);
            delay(50);
            digitalWrite(LED_BLUE, LOW);
            delay(50);
        };
    }


    Serial.println("READY");
}

void PRINT_CSV_HEADER() {
    Serial.println("mem_addr,lat,lon,gps_altitude,baro_altitude,temp,pres,timestamp,aux_data,realtime");
}
void PRINT_CSV(size_t addr, uint32_t start_time, LoggerStruct* data) {

    // We want to print lat/long in dd.dddd... format, so we need to convert it,
    float lat = (float)data->lat / 10000000.0f;
    float lon = (float)data->lon / 10000000.0f;

    // GPS altitude is in mm, so we convert it to m as well
    float gps_altitude = (float)data->gps_altitude / 1000.0f;

    // Timestamp is in milliseconds since start of program, but we have the launch start time.
    // So we will keep both the timestamp and the time since launch.
    float delta = ((int32_t)data->timestamp - (int32_t)start_time) / 1000.0f;

    // Print the data in CSV format
    Serial.print(addr, HEX);
    Serial.print(",");
    Serial.print(lat, DEC);
    Serial.print(",");
    Serial.print(lon, DEC);
    Serial.print(",");
    Serial.print(gps_altitude, DEC);
    Serial.print(",");
    Serial.print(data->baro_altitude, DEC);
    Serial.print(",");
    Serial.print(data->temp, DEC);
    Serial.print(",");
    Serial.print(data->pres, DEC);
    Serial.print(",");
    Serial.print(data->timestamp, DEC);
    Serial.print(",");
    Serial.print(data->aux_data, DEC);
    Serial.print(",");
    Serial.println(delta);
    
}

void PRINT_SETTINGS(FlashSettings* settings) {
    Serial.print("Flight 0 start time: ");
    Serial.println(settings->flight0_start_time, DEC);
    Serial.print("Flight 1 start time: ");
    Serial.println(settings->flight1_start_time, DEC);
    Serial.print("Telemetry channel: ");
    Serial.println(settings->telem_channel, DEC);
    Serial.print("Last written section: ");
    Serial.println(settings->last_written_section, DEC);
}


void YIPPEE_MEM_OFFLOAD_LOOP(Logger* mem) {
    // Read a string from the serial port and execute a switch depending on the string.
    // This is a blocking function, so it will wait until the string is received.

    if(Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim(); // Remove whitespace
        if(command == "R0") {
            Serial.println("# Command: Read flight 0 data.");

            // Read settings
            Serial.print("# Reading settings... ");
            FlashSettings settings;
            mem->read_settings(&settings);
            Serial.print("# Flight 0 start time: ");
            Serial.println(settings.flight0_start_time, DEC);

            // Actually read the data
            LoggerStruct data;
            size_t data_start = LOGGER_DATA_LIMITS::LOGGER_DATA_MIN_F0;
            size_t offset = 0x000000;
            PRINT_CSV_HEADER();
            while (mem->read_flightlocked(&data, data_start+offset, 0))
            {
                PRINT_CSV(data_start+offset, settings.flight0_start_time, &data);
                offset += sizeof(LoggerStruct);
            }
        
            Serial.println("<DONE>");
            Serial.print("# Read data size: ");
            Serial.print(offset, DEC);
            Serial.println("B");
            Serial.println("# Done!");
        } else if (command == "R1") { 
            Serial.println("# Command: Read flight 1 data.");

            // Read settings
            Serial.print("# Reading settings... ");
            FlashSettings settings;
            mem->read_settings(&settings);
            Serial.print("# Flight 1 start time: ");
            Serial.println(settings.flight1_start_time, DEC);

            // Actually read the data
            LoggerStruct data;
            size_t data_start = LOGGER_DATA_LIMITS::LOGGER_DATA_MIN_F1;
            size_t offset = 0x000000;
            PRINT_CSV_HEADER();
            while (mem->read_flightlocked(&data, data_start+offset, 1))
            {
                PRINT_CSV(data_start+offset, settings.flight1_start_time, &data);
                offset += sizeof(LoggerStruct);
            }

            Serial.println("<DONE>");
            Serial.print("# Read data size: ");
            Serial.print(offset, DEC);
            Serial.println("B");

        } else if (command == "D0") {
            Serial.println("# Deleting data from flight 0.");
            mem->reset_flight(0);
            Serial.println("<DONE>");
            Serial.println("# Deleted data from flight 0.");
        } else if (command == "SET") {
            FlashSettings settings;

            mem->read_settings(&settings);
            Serial.println("# Current settings: ");
            PRINT_SETTINGS(&settings);
            Serial.println("<DONE>");
        } else if (command == "D1") {
            Serial.println("# Deleting data from flight 1.");
            mem->reset_flight(1);
            Serial.println("<DONE>");
            Serial.println("# Deleted data from flight 1.");
        } else if(command == "DUMP") {
            Serial.println("# Dumping all data.");
            // Serial.println("# Hex bytes (256/1pg bytes per line):");
            
            for(int i = 0; i < 0x80000; i += 0x100) {
                uint8_t data[256];
                mem->read_raw(data, i, 256);
                for(int j = 0; j < 256; j++) {
                    Serial.print(data[j], HEX);
                    Serial.print(" ");
                }
                Serial.println();
            }
            Serial.println("<DONE>");
            Serial.println("# Dumped all data.");

        } else if(command == "MEMRST") {
            Serial.println("# Resetting memory.");
            mem->reset_flight(0);
            mem->reset_flight(1);

            FlashSettings settings;
            settings.flight0_start_time = 0;
            settings.flight1_start_time = 0;
            settings.telem_channel = '0';
            settings.last_written_section = 0;
            mem->commit_settings(&settings);

            Serial.println("<DONE>");
            Serial.println("# Memory reset.");

        } else if(command == "HELP") {
            Serial.println("Commands: R0, R1, D0, D1, DUMP, MEMRST, HELP");
            Serial.println("<DONE>");
        } else {
            Serial.println("Unknown command.");
            Serial.println("<DONE>");
        }
    }
}

