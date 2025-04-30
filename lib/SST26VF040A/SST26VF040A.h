// // Minimal driver for SST26VF040A, written by Michael Karpov, Surag Nuthulapaty

// #pragma once

// #include <Arduino.h>
// #include <SPI.h>

// class SST26VF040A {
//     public:
//     SST26VF040A() = delete;
//     SST26VF040A(uint8_t cs_pin, uint8_t hold_pin, uint8_t wp_pin);

//     bool begin();

//     void write_command(uint8_t command);
//     uint8_t read();

//     private:
//         uint8_t __cs_pin;
//         uint8_t __hold_pin;
//         uint8_t __wp_pin;

// };

#ifndef SST26VF040A_H
#define SST26VF040A_H

#include <Arduino.h>
#include <SPI.h>

class SST26VF040A {
public:
    static constexpr uint32_t PAGE_SIZE = 256;
    static constexpr uint32_t SECTOR_SIZE = 4096;

    SST26VF040A(uint8_t cs, uint8_t hold, uint8_t wp);

    bool begin();

    void read(uint32_t address, uint8_t* buffer, uint32_t length);
    void write(uint32_t address, const uint8_t* data, uint32_t length);

    void eraseSector(uint32_t address);
    void chipErase();

private:
    uint8_t cs_pin, hold_pin, wp_pin;

    void writeEnable();
    void waitUntilReady();
    void reset();
};

#endif // SST26VF040A_H
