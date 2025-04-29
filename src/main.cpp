#include <Arduino.h>
#include <test_util.h>
#include "pins.h"
#include <Wire.h>
#include <MPL3115A2.h>
#include <__flash2_driver.h>

MPL3115A2 baro;

void setup() {
  #ifdef TEST_ENV
    // Run test setup
    YIPPEE_TEST_SETUP();
    return;
  #endif
  
  // Run main setup
  Serial.println("1");
  Wire.begin();
  Serial.println("2");
  
  // baro.begin(Wire, MPL311_ADDR);

  flash2_init();

  Serial.println("INITTED");
}

void loop() {
  #ifdef TEST_ENV
    // Run test loop and ignore other code.
    YIPPEE_TEST_LOOP();
    return;
  #endif

  float alt = baro.readAltitude();
  float pres = baro.readPressure();
  float temp = baro.readTempF();

  Serial.printf("Alt: %f, Pres: %f, Temp: %f", alt, pres, temp);
}
