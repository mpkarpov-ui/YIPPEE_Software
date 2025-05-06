#include <RH_RF95.h>
#include <SPI.h>

#include <array>
#include <limits>
#include <numeric>
#include <queue>
#include <algorithm>

#include "SerialParser.h"

/* Pins for feather*/
// // Ensure to change depending on wiring
#define RFM95_CS 8
#define RFM95_RST 4
// #define RFM95_EN
#define RFM95_INT 3
#define VoltagePin 14
// #define LED 13 // Blinks on receipt


float RF95_FREQ = 430.00;
float rf95_freq_MHZ = RF95_FREQ;

float current_freq = 0;


struct telem_t {
    // 1 if ok        1 if lock   0=F1, 1=F2      0=Preflight, 1=Flight    Channel 0-F       
    // INIT_NOERR   | GPS_LOCK? | CUR_FLIGHT_NO | FSM_STATE              | TELEM_CHANNEL(4bits) 
    uint8_t status;
  
    // GPS
    int32_t gps_latitude;
    int32_t gps_longitude;
    int32_t gps_altitude;
    int8_t gps_siv_fix_type; // 5 bits SIV, 3 bits fix type
  
    // Barometer
    float baro_altitude;
  };

typedef uint32_t systime_t;
// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);
// For reading from

void setup() {
    while (!Serial)
        ;
    Serial.begin(9600);
    if (!rf95.init()) {
        Serial.println("Failed to init");
        while (1);
    }
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.println("Yippee!");
    if (!rf95.setFrequency(rf95_freq_MHZ)) {
        Serial.println("Failed to set freq");
        while (1);
    }
    
    current_freq = rf95_freq_MHZ;

    rf95.setCodingRate4(5);
    rf95.setSpreadingFactor(7);

    rf95.setPayloadCRC(true);

    rf95.setSignalBandwidth(125000);
    // rf95.setPreambleLength(10);

    Serial.print(R"({"type": "freq_success", "frequency":)");
    Serial.print(current_freq);
    Serial.println("}");
    rf95.setTxPower(23, false);

    Serial.setTimeout(250);
}

// void ChangeFrequency(float freq) {
//     float current_time = millis();
//     rf95.setFrequency(freq);
//     Serial.println(json_command_success);
//     Serial.print(R"({"type": "freq_success", "frequency":)");
//     Serial.print(freq);
//     Serial.println("}");
// }


void loop() {

    if (rf95.available()) {
        uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(telem_t);
        telem_t data;

        if (rf95.recv(buf, &len)) {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(50);
            digitalWrite(LED_BUILTIN, LOW);

            memcpy(&data, buf, len);

            // Serial.println("Fresh packet: ");

            // Decode all the data!

            bool init_noerr = (data.status & (1<<7)) >> 7; // init_noerr is the top bit
            bool gps_locked = (data.status & (1<<6)) >> 6;
            uint8_t fsm_state = (data.status & (0x3 << 4)) >> 4;
            uint8_t telem_chan = (data.status & 0xF);

            char* fsm_state_str = "PREFLIGHT";
            if(fsm_state == 1) {
                fsm_state_str = "FLIGHT";
            }

            if(fsm_state == 2) {
                fsm_state_str = "LANDED";
            }


            uint8_t siv = (data.gps_siv_fix_type & 0xF8) >> 3;
            uint8_t fix_type = data.gps_siv_fix_type & 0x7;
            Serial.print(telem_chan);
            Serial.print(init_noerr ? " OK " : " ERR ");
            Serial.print(fsm_state_str);
            Serial.print(gps_locked ? " LOCKED (" : " NOLOCK (");
            Serial.print(data.gps_latitude);
            Serial.print(" / ");
            Serial.print(data.gps_longitude);
            Serial.print(" / ");
            Serial.print(data.gps_altitude);
            Serial.print(") SIV ");
            Serial.print(siv);
            Serial.print(" FXT ");
            Serial.print(fix_type);
            Serial.print(" BARO ");
            Serial.print(data.baro_altitude, 2);
            Serial.print(" RSSI ");
            Serial.println(rf95.lastRssi());
            

            // // print out buf

            // for(unsigned i = 0; i < len; i++) {
            //     Serial.print(buf[i]);
            // }

            // Serial.println("   <eot>");

        } else {
            Serial.println(Serial.println("Failed to recieve"));
        }
    }

}
