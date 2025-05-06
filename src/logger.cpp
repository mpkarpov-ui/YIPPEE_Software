#include "logger.h"

Logger::Logger(uint8_t cs_pin, uint8_t hold_pin, uint8_t wp_pin) : cs(cs_pin), 
hold(hold_pin), wp(wp_pin), flash(cs_pin, hold_pin, wp_pin), 
packets_written(0), last_written_section(0), buf_idx(0), state(0) {}

bool Logger::begin() {
    if (!flash.begin()) {
        return false;
    }

    if(!read_settings(&settings_)) {
        return false;
    }

    last_written_section = settings_.last_written_section;

    if (last_written_section == 1) {
        data_addr_min = LOGGER_DATA_LIMITS::LOGGER_DATA_MIN_F0;
        data_addr_max = LOGGER_DATA_LIMITS::LOGGER_DATA_MAX_F0;
        oldest_written_section = 0;
    } else {
        data_addr_min = LOGGER_DATA_LIMITS::LOGGER_DATA_MIN_F1;
        data_addr_max = LOGGER_DATA_LIMITS::LOGGER_DATA_MAX_F1;
        oldest_written_section = 1;
    }

    settings_.last_written_section = (settings_.last_written_section + 1) % 2;
    commit_settings(&settings_);

    addr = data_addr_min;

    return true;
}

// void Logger::read_flight_timestamps(uint32_t* t0, uint32_t* t1) {
    
// }

bool Logger::begin_readonly() {
    if (!flash.begin()) {
        return false;
    }

    data_addr_min = 0x0;
    data_addr_max = 0x80000;

    return true;
}

void Logger::clear_first_sector(bool fast_erase = false) {
    // Erase the first sector, but rewrite the first page of settings back to the sector to save settings
    // If fast_erase is true, it will delete the 64kb sector instead of the 4kb sector.

    
    // Read the first page
    uint8_t buffer[256];
    flash.read(0x0, buffer, 256);

    // Erase the first sector
    if(fast_erase) {
        flash.eraseSector64(0x0);
    } else {
        flash.eraseSector(0x0);
    }

    // Write the first page of settings back
    flash.write(0x0, buffer, 256);
}

void Logger::reset_flight(int flight_no) {
    // Due to memory layout we implement different logic for deleting flight 0 and flight 1 data.
    // The layout is 1 page for settings, then pages 1-1023 are for flight 0, and pages 1024-2047 are for flight 1.
    // So we need to erase the first sector, but rewrite the first page of settings back to the sector for flight 0.


    size_t ADDR_MIN = 0x000000;
    size_t ADDR_MAX = LOGGER_DATA_LIMITS::LOGGER_DATA_MAX_F0;

    if (flight_no == 1) {
        ADDR_MIN = LOGGER_DATA_LIMITS::LOGGER_DATA_MIN_F1;
        ADDR_MAX = LOGGER_DATA_LIMITS::LOGGER_DATA_MAX_F1;
    }

    // In effect this means that flight 0 has 1 less page, which isn't a problem since 1 page of data is 4 seconds.
    size_t ADDR_OFFSET = 0x000000;
    if(flight_no == 0) {
        clear_first_sector(true); // Clear the first 64kb, but keep the first page of settings. TRUE fast erase.
        ADDR_OFFSET = 0x10000; // 64kb
    }

    // Flight 1 can just be erased by a sequence of 64kb erases.6
    for (size_t i = ADDR_MIN + ADDR_OFFSET; i < ADDR_MAX; i += 0x10000) {
        flash.eraseSector64(i);
    }
}

void Logger::write_packet(LoggerStruct data) {
    LoggerStruct cur = data;

    flash.write(addr, (uint8_t *) (&cur), sizeof(LoggerStruct));

    addr += sizeof(LoggerStruct);
    packets_written += 1;
}

void Logger::commit_settings(FlashSettings* data) {
    // To rewrite settings we need to erase the first sector, but save the data except for the first page.
    // The first page stores settings but the rest of the pages in the sector store flight data which should be preserved.
    // So we need to read the first sector, save the data, erase the sector, and then write the data back.

    // Read the first sector
    uint8_t buffer[4096];
    flash.read(0x0, buffer, 4096);

    // Erase the first sector
    flash.eraseSector(0x0);

    // Write the first page of settings
    flash.write(0x0, (uint8_t *) data, sizeof(FlashSettings));

    // Write the rest of the data back
    flash.write(0x0 + FLASH_PAGE_SIZE, buffer + FLASH_PAGE_SIZE, 4096 - FLASH_PAGE_SIZE);
}

void Logger::write(LoggerStruct data) {
    if(!allow_write_to_flash) {
        return;
    }
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
    if(s == 2) {
        allow_write_to_flash = false;

        return;
    }
    if (s != 1 || state != 0) {
        return;
    }

    Serial.print("Deleting flight data from ");
    Serial.println(oldest_written_section);

    if(oldest_written_section == 0) {
        settings_.flight0_start_time = millis();
    } else {
        settings_.flight1_start_time = millis();
    }

    commit_settings(&settings_);

    reset_flight(oldest_written_section);

    for (int i = buf_idx; i < buf_idx + 8; i++) {
        write_packet(packet_array[i % 8]);
    }

    state = s;

    buf_idx = 0;
}

bool is_valid_logger_struct(uint8_t* data) {
    // Returns true if the data is invalid (all bytes are 0xFF)
    for (int i = 0; i < sizeof(LoggerStruct); i++) {
        if (data[i] != 0xFF) {
            return true;
        }
    }
    return false;
}

bool Logger::read_settings(FlashSettings* data) {
    flash.read(0x0, (uint8_t *) data, sizeof(FlashSettings));
    return true;   
}

bool Logger::read(LoggerStruct* data, uint32_t ad) {
    if (ad > 0x80000) {
        return false;
    }

    flash.read(ad, (uint8_t *) data, sizeof(LoggerStruct));
    return is_valid_logger_struct((uint8_t*)data);   
}

bool Logger::read_raw(uint8_t* data, uint32_t addr, uint32_t len) {
    if (addr > 0x80000) {
        return false;
    }

    flash.read(addr, data, len);
    return true;
}

bool Logger::read_flightlocked(LoggerStruct* data, uint32_t ad, int flight_num) {
    size_t MIN_ADDR = LOGGER_DATA_LIMITS::LOGGER_DATA_MIN_F0;
    size_t MAX_ADDR = LOGGER_DATA_LIMITS::LOGGER_DATA_MAX_F0;

    if(flight_num == 1) {
        MIN_ADDR = LOGGER_DATA_LIMITS::LOGGER_DATA_MIN_F1;
        MAX_ADDR = LOGGER_DATA_LIMITS::LOGGER_DATA_MAX_F1;
    }

    if (ad >= MAX_ADDR || ad < MIN_ADDR) {
        return false;
    }

    flash.read(ad, (uint8_t *) data, sizeof(LoggerStruct));
    return is_valid_logger_struct((uint8_t*)data);   
}