// #include "SST26VF040A.h"

// SST26VF040A::SST26VF040A(uint8_t cs_pin, uint8_t hold_pin, uint8_t wp_pin) {
//     __cs_pin = cs_pin;
//     __hold_pin = hold_pin;
//     __wp_pin = wp_pin;


//     pinMode(cs_pin, HIGH);
//     digitalWrite(hold_pin, HIGH);
//     digitalWrite(wp_pin, HIGH);

//     /**
//      * since mem is the only thing on SPI, constantly hold its 
//      * value low. It never needs to be brought high
//     */ 

//     pinMode(cs_pin, LOW);
// }

// bool SST26VF040A::begin() {

// }

// void SST26VF040A::write_command(uint8_t command) {
//     SPI.transfer(command);
// }

// uint8_t SST26VF040A::read() {
//     SPI.transfer(0x00);
// }

#include "SST26VF040A.h"

// Command definitions
static constexpr uint8_t CMD_READ_ID         = 0x9F;
static constexpr uint8_t CMD_READ_STATUS     = 0x05;
static constexpr uint8_t CMD_WRITE_ENABLE    = 0x06;
static constexpr uint8_t CMD_PAGE_PROGRAM    = 0x02;
static constexpr uint8_t CMD_READ_DATA       = 0x03;
static constexpr uint8_t CMD_SECTOR_ERASE    = 0x20;
static constexpr uint8_t CMD_SECTOR_ERASE32  = 0x52;
static constexpr uint8_t CMD_SECTOR_ERASE64  = 0xD8;
static constexpr uint8_t CMD_CHIP_ERASE      = 0x60;
static constexpr uint8_t CMD_RESET_ENABLE    = 0x66;
static constexpr uint8_t CMD_RESET_MEMORY    = 0x99;

static constexpr uint8_t STATUS_BUSY_MASK    = 0x01;

// #define MEM_DEBUG

SST26VF040A::SST26VF040A(uint8_t cs, uint8_t hold, uint8_t wp)
    : cs_pin(cs), hold_pin(hold), wp_pin(wp) {}

bool SST26VF040A::begin() {
    
    digitalWrite(cs_pin, HIGH);
    delay(1);
    
    reset();

    

    uint8_t status = readStatus();
    if (status & 0x1C) {
        writeStatus(status & ~0x1C);
    }

    digitalWrite(cs_pin, LOW);
    SPI.transfer(CMD_READ_ID);

    uint8_t manufacturer_id = SPI.transfer(0x00);
    uint8_t memory_type = SPI.transfer(0x00);
    uint8_t capacity = SPI.transfer(0x00);

    digitalWrite(cs_pin, HIGH);

    return (manufacturer_id == 0xBF && memory_type == 0x26 && capacity == 0x14);
}


void SST26VF040A::read(uint32_t address, uint8_t* buffer, uint32_t length) {
    digitalWrite(cs_pin, LOW);

    SPI.transfer(CMD_READ_DATA);
    SPI.transfer((address >> 16) & 0xFF);
    SPI.transfer((address >> 8) & 0xFF);
    SPI.transfer(address & 0xFF);

    for (uint32_t i = 0; i < length; i++) {
        buffer[i] = SPI.transfer(0x00);
    }

    digitalWrite(cs_pin, HIGH);
}

// only allows writing to one page at a time, could be updated to do more
void SST26VF040A::write(uint32_t address, const uint8_t* data, uint32_t length) {
    writeEnable();

    digitalWrite(cs_pin, LOW);

    SPI.transfer(CMD_PAGE_PROGRAM);
    SPI.transfer((address >> 16) & 0xFF);
    SPI.transfer((address >> 8) & 0xFF);
    SPI.transfer(address & 0xFF);

    for (uint32_t i = 0; i < length; i++) {
        SPI.transfer(data[i]);
    }

    digitalWrite(cs_pin, HIGH);
    waitUntilReady();
}

void SST26VF040A::eraseSector(uint32_t address) {
    writeEnable();
    digitalWrite(cs_pin, LOW);
    
    SPI.transfer(CMD_SECTOR_ERASE);

    SPI.transfer((address >> 16) & 0xFF);
    SPI.transfer((address >> 8) & 0xFF);
    SPI.transfer(address & 0xFF);

    #ifdef MEM_DEBUG
    Serial.print("Erasing sector4 at address: 0x");
    Serial.println(address, HEX);

    Serial.print("Memory region affected: 0x");
    Serial.print(address, HEX);
    Serial.print(" to 0x");
    Serial.println(address + 0x1000, HEX);
    #endif
    

    digitalWrite(cs_pin, HIGH);
    waitUntilReady();
}

void SST26VF040A::eraseSector32(uint32_t address) {
    writeEnable();
    digitalWrite(cs_pin, LOW);
    
    SPI.transfer(CMD_SECTOR_ERASE32);

    SPI.transfer((address >> 16) & 0xFF);
    SPI.transfer((address >> 8) & 0xFF);
    SPI.transfer(address & 0xFF);

    #ifdef MEM_DEBUG
    Serial.print("Erasing sector32 at address: 0x");
    Serial.println(address, HEX);

    Serial.print("Memory region affected: 0x");
    Serial.print(address, HEX);
    Serial.print(" to 0x");
    Serial.println(address + 0x8000, HEX);
    #endif

    digitalWrite(cs_pin, HIGH);
    waitUntilReady();
}

void SST26VF040A::eraseSector64(uint32_t address) {
    writeEnable();
    digitalWrite(cs_pin, LOW);
    
    SPI.transfer(CMD_SECTOR_ERASE64);

    SPI.transfer((address >> 16) & 0xFF);
    SPI.transfer((address >> 8) & 0xFF);
    SPI.transfer(address & 0xFF);

    #ifdef MEM_DEBUG
    Serial.print("Erasing sector64 at address: 0x");
    Serial.println(address, HEX);

    Serial.print("Memory region affected: 0x");
    Serial.print(address, HEX);
    Serial.print(" to 0x");
    Serial.println(address + 0x10000, HEX);
    #endif


    digitalWrite(cs_pin, HIGH);
    waitUntilReady();
}

void SST26VF040A::chipErase() {
    writeEnable();
    digitalWrite(cs_pin, LOW);
    SPI.transfer(CMD_CHIP_ERASE);
    digitalWrite(cs_pin, HIGH);
    waitUntilReady();
}

void SST26VF040A::writeEnable() {
    digitalWrite(cs_pin, LOW);
    delay(10);

    SPI.transfer(CMD_WRITE_ENABLE);

    delay(10);
    digitalWrite(cs_pin, HIGH);
}

void SST26VF040A::waitUntilReady() {
    uint8_t status;

    do {
        digitalWrite(cs_pin, LOW);

        SPI.transfer(CMD_READ_STATUS);
        status = SPI.transfer(0x00);

        digitalWrite(cs_pin, HIGH);
    } while (status & STATUS_BUSY_MASK);
}

void SST26VF040A::reset() {
    digitalWrite(cs_pin, LOW);
    SPI.transfer(CMD_RESET_ENABLE);
    digitalWrite(cs_pin, HIGH);

    delay(1);

    digitalWrite(cs_pin, LOW);
    SPI.transfer(CMD_RESET_MEMORY);
    digitalWrite(cs_pin, HIGH);

    delay(1);
}

uint8_t SST26VF040A::readStatus() {
    digitalWrite(cs_pin, LOW);
    SPI.transfer(0x05); 
    uint8_t status = SPI.transfer(0x00);
    digitalWrite(cs_pin, HIGH);
    return status;
}

void SST26VF040A::writeStatus(uint8_t value) {
    writeEnable();
    digitalWrite(cs_pin, LOW);
    SPI.transfer(0x01); 
    SPI.transfer(value);
    digitalWrite(cs_pin, HIGH);
    waitUntilReady();
}
