#include <Arduino.h>
#ifdef TEST_ENV
  #include <test_util.h>
#endif
#include "pins.h"
#include <Wire.h>
#include <MPL3115A2.h>
#include <SparkFun_u-blox_GNSS_v3.h>
#include <SPIMemory.h>

// Check the config below to allow for booting with failures
// #define ALLOW_SETUP_FAILURES


MPL3115A2 baro;
SFE_UBLOX_GNSS_SERIAL gps;
SPIFlash mem(MEM_CS);

struct board_data_t {
  // GPS
  bool gps_fix_ok;
  uint8_t gps_fix_type;
  uint8_t gps_siv;
  int32_t gps_latitude;
  int32_t gps_longitude;
  int32_t gps_altitude;

};

board_data_t global_data;
uint32_t NEXT_GPS_FLASH;
bool GPS_LIGHT_STATE = false;
bool TELE_LIGHT_STATE = false;
bool LAST_GPS_OK_STATE = false;

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
  tone(BUZZER, 2500, 60);
  delay(100);
  digitalWrite(LED_BLUE, LOW);


  // Init all interfaces
  Serial5.begin(9600);
  Serial4.begin(9600);
  Serial.begin(9600);
  SPI.begin();

  NEXT_GPS_FLASH = millis() + 500;

  // Try to init GPS 
  bool gps_has_init = gps.begin(Serial5);
  
  gps.setDynamicModel(DYN_MODEL_AIRBORNE4g);

  #ifndef ALLOW_SETUP_FAILURES
    // Fail loudly.
    if(!gps_has_init) {
      Serial.println("Failed to init GPS!");
      digitalWrite(LED_GREEN, HIGH); // Flashing green LED = GPS fail
      tone(BUZZER, 3000, 100);
      delay(200);
      tone(BUZZER, 3000, 100);
      delay(200);
      tone(BUZZER, 3000, 100);
      delay(200);

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
  #ifndef ALLOW_SETUP_FAILURES
    // Fail loudly.
    if(!flash_init_stat) {
      Serial.println("Failed to init Flash!");
      digitalWrite(LED_BLUE, HIGH); // Flashing blue is flash failure
      tone(BUZZER, 3000, 100);
      delay(200);
      tone(BUZZER, 3000, 100);
      delay(200);

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
  

  // Show setup finished! (And play YIPPEE tone!)
  // tone(BUZZER, 5200, 130);
  // delay(120);
  tone(BUZZER, 2600, 100);
  delay(100);
}

void get_gps_data() {
  global_data.gps_fix_ok = gps.getGnssFixOk();
  global_data.gps_siv = gps.getSIV(400);
  if(global_data.gps_fix_ok) {
    global_data.gps_fix_type = gps.getFixType();
    global_data.gps_latitude = gps.getLatitude();
    global_data.gps_longitude = gps.getLongitude();
    global_data.gps_altitude = gps.getAltitude();
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
  
  get_gps_data();  
  if(global_data.gps_fix_ok) {
    if(LAST_GPS_OK_STATE == false) {
      LAST_GPS_OK_STATE = true;
      // Lil beeps to tell us gps is ok!
      tone(BUZZER, 4000, 80);
      delay(100);
      tone(BUZZER, 3000, 80);
      delay(80);
      GPS_LIGHT_STATE = true;
      digitalWrite(LED_GREEN, HIGH);
    } else {
      LAST_GPS_OK_STATE = false;
      GPS_LIGHT_STATE = false;
      digitalWrite(LED_GREEN, LOW);
    }

  } else {
    if(millis() > NEXT_GPS_FLASH) {
      NEXT_GPS_FLASH = millis() + 500;
      GPS_LIGHT_STATE = !GPS_LIGHT_STATE;
      digitalWrite(LED_GREEN, GPS_LIGHT_STATE ? HIGH : LOW);
    }
  }

  DEBUG_print_gps_data();

  delay(10);
}
