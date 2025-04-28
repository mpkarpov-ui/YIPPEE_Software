#include <Arduino.h>
#include <test_util.h>
#include <Wire.h>
#include <pins.h>

#ifdef TEST_BAROMETER
    #include <MPL3115A2.h>
    MPL3115A2 pressure_sensor_test;

#endif

#ifdef TEST_GPS
    #include <SparkFun_u-blox_GNSS_v3.h>
    SFE_UBLOX_GNSS_SERIAL ublox;
#endif


void YIPPEE_TEST_SETUP() {
    Serial.begin(9600);

    // Wait until serial..
    while (!Serial) {}

    Serial.println("======= YIPPEE TEST ENVIRONMENT =======");
    Serial.println("BEGIN SETUP:");

    // Set up all interfaces
    Serial.print("Begin I2c ... ");
    Wire.begin();

    // Set up all tests
    Serial.println("BEGIN TEST SETUP:");
    #ifdef TEST_MCU
        Serial.println("[TEST_MCU] Tests whether the MCU is responding over serial properly.");
        Serial.println("No special setup required");
    #endif

    #ifdef TEST_BAROMETER
        Serial.println("[TEST_BAROMETER] Initializes and reads from barometer.");
        Serial.print("[TEST_BAROMETER] Calling MPL3115A2::begin() ... ");
        pressure_sensor_test.begin(); Serial.println("OK!");

        Serial.print("[TEST_BAROMETER] Calling MPL3115A2::setModeAltimeter() ... ");
        pressure_sensor_test.setModeAltimeter(); Serial.println("OK!");

        Serial.print("[TEST_BAROMETER] Setting MPL3115A2 settings ... ");
        pressure_sensor_test.setOversampleRate(7);
        pressure_sensor_test.enableEventFlags();
        Serial.println("OK!");
    #endif

    #ifdef TEST_GPS
        Serial.println("[TEST_GPS] Initializes and reads from GPS.");
        Serial.print("[TEST_GPS] Calling ublox::begin() ... ");
        if(!ublox.begin()) {
            Serial.println("\nFAILED TO INITIALIZE GPS!");
            while(1) {};
        }
        Serial.println("OK!");

        Serial.print("[TEST_BAROMETER] Calling MPL3115A2::setModeAltimeter() ... ");
        pressure_sensor_test.setModeAltimeter(); Serial.println("OK!");

        Serial.print("[TEST_BAROMETER] Setting MPL3115A2 settings ... ");
        pressure_sensor_test.setOversampleRate(7);
        pressure_sensor_test.enableEventFlags();
        Serial.println("OK!");
    #endif

    #ifdef TEST_LED
        Serial.println("[TEST_LED] Initializes and flashes LEDs.");
        Serial.print("[TEST_LED] Setting all pin modes ... ");
        pinMode(LED_BUILTIN, OUTPUT);
        pinMode(LED_BLUE, OUTPUT);
        pinMode(LED_GREEN, OUTPUT);
        Serial.println("OK!");

    #endif

    Serial.println("\n\nSETUP DONE.");
    Serial.println("Delaying for 2s to allow for output saving.");
    delay(2000);
}

void YIPPEE_TEST_LOOP() {
    Serial.println("BEGIN LOOP:");

    #ifdef TEST_MCU
        Serial.println("[TEST_MCU] Hello, World!");
    #endif

    #ifdef TEST_BAROMETER
        float alt = pressure_sensor_test.readAltitude();
        float pres = pressure_sensor_test.readPressure();
        float temp = pressure_sensor_test.readTemp();
        Serial.print("[TEST_BAROMETER] ");
        Serial.print(alt, 2);
        Serial.print("m  (P ");
        Serial.print(pres, 2);
        Serial.print("Pa / ");
        Serial.print(temp, 2);
        Serial.println("C)");

    #endif

    #ifdef TEST_LED
        // Flash LEDs sequentially
        Serial.println("[TEST_LED] Flashing all 3 LEDs, sequentially");
        digitalWrite(LED_BUILTIN, HIGH);
        delay(20);
        digitalWrite(LED_BUILTIN, LOW);

        digitalWrite(LED_BLUE, HIGH);
        delay(20);
        digitalWrite(LED_BLUE, LOW);

        digitalWrite(LED_GREEN, HIGH);
        delay(20);
        digitalWrite(LED_GREEN, LOW);

        Serial.println("[TEST_LED] Done!");
    #endif

    Serial.println("\n");
    delay(10);
}