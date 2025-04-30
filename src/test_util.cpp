#include <Arduino.h>
#include <SPI.h>
#include <test_util.h>
#include <Wire.h>
#include <pins.h>

#ifdef TEST_BAROMETER
    #include <MPL3115A2.h>
    MPL3115A2 pressure_sensor_test;

#endif

#ifdef TEST_MEM
    #include "SST26VF040A.h"
    SST26VF040A flash(MEM_CS, MEM_HOLD, MEM_WRITE_PROTECT);
#endif

#ifdef TEST_LORA
    #include <HopeHM.h>
    HopeHM lora;
#endif

#ifdef TEST_GPS
    #include <SparkFun_u-blox_GNSS_v3.h>
    SFE_UBLOX_GNSS_SERIAL ublox;

    void sendUbloxConfig(Stream &gpsSerial) {
        // UBX-CFG-PRT to set UART to UBX only, 38400 baud
        uint8_t setUartToUblox[] = {
            0xB5, 0x62,  // UBX header
            0x06, 0x00,  // CFG-PRT class and ID
            20, 0,       // Payload length = 20 bytes
            0x01,        // PortID = UART1
            0x00,        // Reserved
            0x00, 0x00,  // txReady (disabled)
            0xD0, 0x08, 0x00, 0x00,  // mode (8N1 no parity, 1 stop bit)
            0x00, 0x00, 0x00, 0x00,  // baudRate placeholder (not changed)
            0x01, 0x00,  // inProtoMask = UBX only
            0x01, 0x00,  // outProtoMask = UBX only
            0x00, 0x00,  // flags
            0x00, 0x00   // reserved
        };
        // Send the message
        gpsSerial.write(setUartToUblox, sizeof(setUartToUblox));
        gpsSerial.flush(); // Make sure it gets sent immediately
        delay(200); // Give GPS time to switch
    }

    void gpsHardwareReset() {
        pinMode(GPS_RESET, OUTPUT);
        digitalWrite(GPS_RESET, LOW);   // Hold reset LOW
        delay(200);                     // Hold low long enough
        digitalWrite(GPS_RESET, HIGH);   // Release reset
        delay(500);                     // Give GPS time to boot
    }

    bool detectAndFixGPS(Stream &gpsSerial) {
        unsigned long startTime = millis();
        bool nmeaDetected = false;
        bool ubxDetected = false;
    
        while (millis() - startTime < 2000) { // Watch for 2 seconds
            if (gpsSerial.available()) {
                uint8_t c = gpsSerial.read();
    
                if (c == '$') {
                    // NMEA sentence start
                    nmeaDetected = true;
                    break;
                } else if (c == 0xB5) {
                    // Possible UBX packet
                    if (gpsSerial.read() == 0x62) {
                        ubxDetected = true;
                        break;
                    }
                }
            }
        }
    
        if (ubxDetected) {
            Serial.println("[GPS] UBX protocol detected.");
            return true; // Good
        }
    
        if (nmeaDetected) {
            Serial.println("[GPS] NMEA protocol detected. Sending UBX config...");
            sendUbloxConfig(gpsSerial);
            delay(300); // Give it time to apply
            return true; // Try again
        }
    
        Serial.println("[GPS] No valid GPS data detected.");
        return false;
    }
    
#endif




void YIPPEE_TEST_SETUP() {
    Serial.begin(9600);

    // Wait until serial..
    while (!Serial) {}

    Serial.println("======= YIPPEE TEST ENVIRONMENT =======");
    Serial.println("BEGIN SETUP:");

    // Set up all interfaces
    Serial.print("Begin I2c ... ");
    Wire.begin(); Serial.println("OK!");

    Serial.print("Begin SPI ... ");
    SPI.begin(); Serial.println("OK!");


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

        Serial.print("[TEST_GPS] Resetting GPS hardware... ");
        gpsHardwareReset();
        Serial.println("Done!");

        Serial.print("[TEST_GPS] Begin SERIAL 5 (GPS Serial) ... "); 
        Serial5.begin(38400);
        Serial.println("OK!");

        Serial.print("[TEST_GPS] Detecting GPS protocol... ");
        if (!detectAndFixGPS(Serial5)) {
            Serial.println("FAIL\nFAILED TO DETECT GPS!");
            while(1) {};
        }
        Serial.println("OK!");

        Serial.print("[TEST_GPS] Calling ublox::begin() ... ");
        if(!ublox.begin(Serial5)) {
            Serial.println("\nFAILED TO INITIALIZE GPS!");
            while(1) {};
        }
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

    #ifdef TEST_LORA
        Serial.println("[TEST_LORA] Initializes and test LORA.");
        Serial.print("[TEST_LORA] Setting all pin modes ... ");
        Serial4.begin(9600);

        pinMode(LORA_RESET, OUTPUT);
        pinMode(LORA_SLEEP, OUTPUT);
        pinMode(LORA_CONFIG, OUTPUT);
        Serial.println("OK!");

        
        lora.set_tx_power(HOPEHM_TX_POWER::TX_POWER_20DB);


    #endif    


    #ifdef TEST_MEM
        // Serial.println("[TEST_MEM] Initializes r/w tests memory. THIS TEST ONLY RUNS AT STARTUP!");
        // Serial.print("[TEST_MEM] Setting all pin modes ... ");
        // pinMode(MEM_CS, OUTPUT);
        // pinMode(MEM_HOLD, OUTPUT);
        // pinMode(MEM_WRITE_PROTECT, OUTPUT);
        // Serial.println("OK!");

        // digitalWrite(MEM_CS, LOW);
        // digitalWrite(MEM_HOLD, HIGH);
        // digitalWrite(MEM_WRITE_PROTECT, HIGH);

        // Serial.print("[TEST_MEM] Attemping flash init... ");
        // if(!flash.begin()) {
        //     Serial.println("FAIL\nFAILED TO INITIALIZE FLASH!");
        //     while(1) {};
        // }
        // Serial.println("OK!");
        // Serial.println("[TEST_MEM] Simple flash test: ");
        // flash.writeByte(0x0000, 0x69);
        // delay(1000);

        // uint8_t data = flash.readByte(0x0000); // Read byte at address 0x0000
        // Serial.print("Data at 0x0000: ");
        // Serial.println(data, HEX);

        SPI.begin();
        pinMode(MEM_CS, OUTPUT);
        pinMode(MEM_HOLD, OUTPUT);
        pinMode(MEM_WRITE_PROTECT, OUTPUT);
        digitalWrite(MEM_CS, HIGH);
        digitalWrite(MEM_HOLD, HIGH);
        digitalWrite(MEM_WRITE_PROTECT, HIGH);
    
        delay(100);
    
        // // Send JEDEC ID Read command (0x9F)
        // digitalWrite(MEM_CS, LOW);
        // SPI.transfer(0x9F);  // Command
        // uint8_t manufacturer = SPI.transfer(0x00);
        // uint8_t memoryType   = SPI.transfer(0x00);
        // uint8_t capacity     = SPI.transfer(0x00);
        // digitalWrite(MEM_CS, HIGH);
    
        // Serial.println("Test flash communication..");
        // Serial.print("Manufacturer ID: 0x");
        // Serial.println(manufacturer, HEX);
        // Serial.print("Device Type: 0x");
        // Serial.println(memoryType, HEX);
        // Serial.print("Device ID: 0x");
        // Serial.println(capacity, HEX);

        if (flash.begin()) {
            Serial.println("Flash initialized successfully!");
        } else {
            Serial.println("Flash initialization failed!");
        }

        const char* message = "Hello World!";
        flash.eraseSector(0x000000);
        flash.write(0x000000, (const uint8_t*)message, strlen(message));

        uint8_t buffer[20];

        flash.read(0x000000, buffer, strlen(message));
        buffer[strlen(message)] = '\0';

        Serial.print("Read back: ");

        Serial.println((char*) buffer);
        


    #endif


    Serial.println("\n\nSETUP DONE.");
    #ifdef DISABLE_LOOP
    Serial.println("==== ALL DONE! ====");
    #else
    Serial.println("Delaying for 2s to allow for output saving.");
    delay(2000);
    #endif
}

void YIPPEE_TEST_LOOP() {
    #ifdef DISABLE_LOOP
        delay(10);
        return;
    #endif
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

    Serial.println("");
    delay(10);
}