#include "logger.h"

Logger::Logger(uint8_t cs_pin, uint8_t hold_pin, uint8_t wp_pin) : cs(cs_pin), 
hold(hold_pin), wp(wp_pin), flash(cs_pin, hold_pin, wp_pin), 
packets_written(0), last_written_section(0), buf_idx(0), state(0) {}

bool Logger::begin() {
    if (!flash.begin()) {
        return false;
    }

    int last_section = 0;

    flash.read(0x0, (uint8_t *) (&last_section), sizeof(last_section));

    last_written_section = last_section;

    if (last_written_section == 1) {
        data_addr_min = 0x200;
        data_addr_max = 0x40100;
    } else {
        data_addr_min = 0x40100;
        data_addr_max = 0x80000;
    }

    Serial.printf("read %d\n", last_written_section);

    flash.eraseSector(0x000000);

    last_section = (last_section + 1) % 2;
    flash.write(0x0, (uint8_t *) (&last_section), sizeof(last_section));

    addr = data_addr_min;

    return true;
}

bool Logger::begin_readonly() {
    if (!flash.begin()) {
        return false;
    }

    data_addr_min = 0x0;
    data_addr_max = 0x80000;

    return true;
}

void Logger::reset_flight() {
    for (size_t i = data_addr_min; i < data_addr_max; i += 4096) {
        flash.eraseSector(i);
    }
}

void Logger::write_packet(LoggerStruct data) {
    LoggerStruct cur = data;

    flash.write(addr, (uint8_t *) (&cur), sizeof(LoggerStruct));

    addr += sizeof(LoggerStruct);
    packets_written += 1;
}

void Logger::write(LoggerStruct data) {
    // before launch
    if (state == 0) {
        packet_array[buf_idx % 8] = data;
        buf_idx += 1;
        return;
    }

    if (addr >= data_addr_max) {
        return;
    }

    packet_array[buf_idx] = data;

    buf_idx += 1;

    if (buf_idx >= 8) {
        for (int i = 0; i < 8; i++) {
            write_packet(packet_array[i]);
        }

        buf_idx = 0;
    }

    return;   
}

void Logger::change_state(int s) {
    if (s != 1 || state != 0) {
        return;
    }

    reset_flight();

    for (int i = buf_idx; i < buf_idx + 8; i++) {
        write_packet(packet_array[buf_idx % 8]);
    }

    state = s;

    buf_idx = 0;
}

void Logger::read(LoggerStruct* data, uint32_t ad) {
    if (ad > 0x80000) {
        return;
    }

    flash.read(ad, (uint8_t *) data, sizeof(LoggerStruct));

    return;   
}