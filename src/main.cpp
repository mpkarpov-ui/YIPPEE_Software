#include <Arduino.h>
#ifdef TEST_ENV
  #include <test_util.h>
#endif
#include "pins.h"
#include "buzzer.h"
#include "logger.h"
#include "HopeHM.h"
#include <Wire.h>
#include <MPL3115A2.h>
#include <SparkFun_u-blox_GNSS_v3.h>
#include <SST26VF040A.h>

// Check the config below to allow for booting with failures
// #define ALLOW_SETUP_FAILURES

// 500ms / 2hz
#define LOG_FREQUENCY 500

// Threshold above pad that we detect launch
#define LAUNCH_THRESHOLD_ALT 50 // m
// Times we must read the launch threshold before we consider it a launch
#define CONSECUTIVE_LAUNCH_THRESHOLD 3

MPL3115A2 baro;
SFE_UBLOX_GNSS_SERIAL gps;
Logger mem(MEM_CS, MEM_HOLD, MEM_WRITE_PROTECT);
HopeHM lora(LORA_RESET, LORA_SLEEP, LORA_CONFIG);

struct board_data_t {
  bool init_no_err = true;

  // GPS
  bool gps_fix_ok = false;
  uint8_t gps_fix_type = 0;
  uint8_t gps_siv = 0;
  int32_t gps_latitude = 0;
  int32_t gps_longitude = 0;
  int32_t gps_altitude = 0;

  // Baro
  int32_t baro_altitude = 0;
  int32_t baro_temp = 0;
  int32_t baro_pressure = 0;

  int32_t initial_baro_altitude = 0;

  // Flight data
  uint8_t FSM_state = 0; // 0=Preflight, 1= Transition, 2=Flight
  uint8_t flight_number = 0; // 0=F1, 1=F2
};

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
  int32_t baro_altitude;
};

board_data_t global_data;
uint32_t NEXT_GPS_FLASH;
uint32_t NEXT_GPS_DATA;
uint32_t NEXT_TELEM_TRANSMIT;
uint32_t NEXT_LOGGER_COMMIT;
uint32_t NEXT_BAROMETER_READ;
uint32_t CONSECUTIVE_BAROMETER_READS = 0;
bool GPS_LIGHT_STATE = false;
bool TELE_LIGHT_STATE = false;
bool LAST_GPS_OK_STATE = false;

telem_t make_packet(const board_data_t& data) {
  char lora_channel = lora.get_channel();
  telem_t packet;
  packet.status = 0x00 & (lora_channel & 0x0F); // Mask out the channel bits
  packet.status |= (data.init_no_err & 0x01) << 7; // Set the init_no_err bit

  packet.gps_latitude = data.gps_latitude;
  packet.gps_longitude = data.gps_longitude;
  packet.gps_altitude = data.gps_altitude;
  packet.gps_siv_fix_type = (data.gps_siv << 3) | (data.gps_fix_type & 0x07); // SIV in upper bits, fix type in lower bits
  packet.baro_altitude = data.baro_altitude;
  return packet;
}

LoggerStruct make_logger_packet(const board_data_t& data) {
  LoggerStruct packet;
  packet.gps_altitude = data.gps_altitude;
  packet.baro_altitude = data.baro_altitude;
  packet.temp = data.baro_temp;
  packet.pres = data.baro_pressure;
  packet.timestamp = millis();
  packet.lat = data.gps_latitude;
  packet.lon = data.gps_longitude;
  packet.aux_data = ((data.FSM_state & 0x03) << 30) | ((data.gps_siv & 0x1F) << 25) | ((data.gps_fix_type & 0x07) << 22) | ((data.flight_number & 0x01) << 21) | (lora.get_channel() & 0x0F) << 17;
  return packet;
}

board_data_t decode_packet(uint8_t* data, size_t len) {
  board_data_t packet;
  if(len != sizeof(telem_t)) {
    return packet; // Invalid packet size
  }
  telem_t* telem = (telem_t*)data;
  packet.gps_latitude = telem->gps_latitude;
  packet.gps_longitude = telem->gps_longitude;
  packet.gps_altitude = telem->gps_altitude;
  packet.gps_siv = (telem->gps_siv_fix_type >> 3) & 0x1F; // Extract SIV from upper bits
  packet.gps_fix_type = telem->gps_siv_fix_type & 0x07; // Extract fix type from lower bits
  return packet;
}

bool transmit_telemetry() {
  telem_t packet = make_packet(global_data);
  size_t packet_size = sizeof(packet);
  uint8_t* packet_data = (uint8_t*)&packet;

  return lora.transmit(packet_data, packet_size);
}

void setup() {
  #ifdef TEST_ENV
    // Run test setup
    YIPPEE_TEST_SETUP();
    return;
  #endif
  
  // Set up all pins
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(MEM_CS, OUTPUT);
  pinMode(MEM_HOLD, OUTPUT);
  pinMode(MEM_WRITE_PROTECT, OUTPUT);

  // Show signs of life!
  digitalWrite(LED_BLUE, HIGH);
  BUZZ_INIT();
  digitalWrite(LED_BLUE, LOW);


  // Init all interfaces
  Serial5.begin(9600);
  Serial4.begin(9600);
  Serial.begin(9600);
  SPI.begin();

  NEXT_GPS_FLASH = millis() + 500;
  NEXT_TELEM_TRANSMIT = millis();
  NEXT_GPS_DATA = millis();
  NEXT_LOGGER_COMMIT = millis();
  NEXT_BAROMETER_READ = millis();

  // Try to init GPS 
  bool gps_has_init = gps.begin(Serial5);
  
  gps.setDynamicModel(DYN_MODEL_AIRBORNE4g);

  if(!gps_has_init) { global_data.init_no_err = false; }
  #ifndef ALLOW_SETUP_FAILURES
    // Fail loudly.
    if(!gps_has_init) {
      Serial.println("Failed to init GPS!");
      digitalWrite(LED_GREEN, HIGH); // Flashing green LED = GPS fail
      BUZZ_FAIL_GPS();

      while(1) {
        digitalWrite(LED_GREEN, HIGH);
        delay(50);
        digitalWrite(LED_GREEN, LOW);
        delay(50);
      };
    }
  #endif



  // Flash init
  digitalWrite(MEM_HOLD, HIGH);
  digitalWrite(MEM_WRITE_PROTECT, HIGH);

  bool flash_init_stat = mem.begin();
  if(!flash_init_stat) { global_data.init_no_err = false; }
  #ifndef ALLOW_SETUP_FAILURES
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
  #endif

  // Baro init
  baro.begin(Wire);

  // Lora init
  bool lora_init_stat = lora.begin(&Serial4);
  #ifndef ALLOW_SETUP_FAILURES
    // Fail loudly.
    if(!lora_init_stat) {
      Serial.println("Failed to init LoRa!");
      digitalWrite(LED_BLUE, HIGH); // Both LEDs flashing is lora failure
      BUZZ_FAIL_LORA();

      while(1) {
        digitalWrite(LED_BLUE, HIGH);
        digitalWrite(LED_GREEN, LOW);
        delay(50);
        digitalWrite(LED_BLUE, LOW);
        digitalWrite(LED_GREEN, HIGH);
        delay(50);
      };
    }
  #endif

  // Show setup finished! (And play YIPPEE tone!)
  BUZZ_YIPPEE();
}

void get_gps_data() {
  global_data.gps_fix_ok = gps.getGnssFixOk(200);
  global_data.gps_siv = gps.getSIV(200);

  if(global_data.gps_siv > 100) {
    global_data.gps_siv = 0;
  }
  if(global_data.gps_fix_ok) {
    global_data.gps_fix_type = gps.getFixType(200);
    global_data.gps_latitude = gps.getLatitude(200);
    global_data.gps_longitude = gps.getLongitude(200);
    global_data.gps_altitude = gps.getAltitude(200);
  }
}

void DEBUG_print_gps_data() {
  Serial.println("== DEBUG: GPS ==");
  Serial.print("(FIX OK / SIV):  ");
  Serial.print(global_data.gps_fix_ok);
  Serial.print(" / ");
  Serial.println(global_data.gps_siv);

  if(global_data.gps_fix_ok) {
    Serial.println("GPS FIX OK!");
    Serial.print("FT: ");
    Serial.println(global_data.gps_fix_type);
    Serial.print("(LAT/LON/ALT): ");
    Serial.print(global_data.gps_latitude);
    Serial.print(" / ");
    Serial.print(global_data.gps_longitude);
    Serial.print(" / ");
    Serial.println(global_data.gps_altitude);
  } else {
    Serial.println("NO FIX");
  }
}

void loop() {
  #ifdef TEST_ENV
    // Run test loop and ignore other code.
    YIPPEE_TEST_LOOP();
    return;
  #endif
  
   // We will only attempt getting GPS data every 500ms
  if(millis() > NEXT_GPS_DATA) {
    NEXT_GPS_DATA = millis() + 500;
    get_gps_data();
    // DEBUG_print_gps_data();
  }
  
  if(global_data.gps_fix_ok) {
    if(LAST_GPS_OK_STATE == false) {
      LAST_GPS_OK_STATE = true;
      // Lil beeps to tell us gps is ok!
      BUZZ_GPS_LOCK_ACQUIRED();
      GPS_LIGHT_STATE = true;
      digitalWrite(LED_GREEN, HIGH);
    }

  } else {
    if(LAST_GPS_OK_STATE == true) {
      // We lost lock :(
      LAST_GPS_OK_STATE = false;
      GPS_LIGHT_STATE = false;
      digitalWrite(LED_GREEN, LOW);

      BUZZ_GPS_LOCK_LOST();
      
    }
    if(millis() > NEXT_GPS_FLASH) {
      if(global_data.gps_siv > 0) {
        Serial.println("Satellites are in view.. attempting to acquire lock");
        NEXT_GPS_FLASH = millis() + 350;
        GPS_LIGHT_STATE = !GPS_LIGHT_STATE;
        digitalWrite(LED_GREEN, GPS_LIGHT_STATE ? HIGH : LOW);
        BUZZ_GPS_SAT_IN_VIEW();
      } else {
        NEXT_GPS_FLASH = millis() + 750;
        GPS_LIGHT_STATE = !GPS_LIGHT_STATE;
        digitalWrite(LED_GREEN, GPS_LIGHT_STATE ? HIGH : LOW);
      }
    }
  }

  // Transmit telemetry data
  if(millis() > NEXT_TELEM_TRANSMIT && lora.has_init_succeeded()) {
    if(transmit_telemetry()) {
      TELE_LIGHT_STATE = !TELE_LIGHT_STATE;
      digitalWrite(LED_BLUE, TELE_LIGHT_STATE ? HIGH : LOW);
    } else {
      BUZZ_FAIL_TRANSMIT();
    }
    NEXT_TELEM_TRANSMIT = millis() + 500;
  }

  // Log data to flash memory
  if(millis() > NEXT_LOGGER_COMMIT) {
    LoggerStruct packet = make_logger_packet(global_data);
    mem.write(packet);
    NEXT_LOGGER_COMMIT = millis() + LOG_FREQUENCY;
  }

  if(millis() > NEXT_BAROMETER_READ) {
    global_data.baro_altitude = baro.readAltitude();
    global_data.baro_temp = baro.readTemp();
    global_data.baro_pressure = baro.readPressure();

    if(global_data.initial_baro_altitude == 0) {
      global_data.initial_baro_altitude = global_data.baro_altitude;
    } else {
      // Check if we are above the launch threshold
      int32_t delta_altitude = global_data.baro_altitude - global_data.initial_baro_altitude;

      if(delta_altitude < LAUNCH_THRESHOLD_ALT) {
        CONSECUTIVE_BAROMETER_READS = 0;
      } else {
        CONSECUTIVE_BAROMETER_READS++;
      }
  
      if(CONSECUTIVE_BAROMETER_READS >= CONSECUTIVE_LAUNCH_THRESHOLD && global_data.FSM_state == 0) {
        // We have launched! Set FSM state to 1 (Transition)
        global_data.FSM_state = 1;
        mem.change_state(global_data.FSM_state);
      }
    }
    

    NEXT_BAROMETER_READ = millis() + 50;
  }

  delay(1);
}
