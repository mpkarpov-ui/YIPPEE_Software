#include "HopeHM.h"



bool HopeHM::write_command(char* cmdtype, char** param_list, int n_params) {
    #ifdef LORA_DEBUG
    Serial.print("[LORA:DEBUG:write_command] ");
    
    Serial.print("AT+");
    Serial.print(cmdtype);
    Serial.print("=");
    for (int i = 0; i < n_params; i++) {
        Serial.print(param_list[i]);
        if (i != n_params - 1) {
            Serial.print(",");
        }
    }
    Serial.println("<CR>");
    #endif

    while(serial_->available()) {
        // Flush input buffer before the write
        serial_->read();
    }

    serial_->print("AT+");
    serial_->print(cmdtype);
    serial_->print("=");
    for (int i = 0; i < n_params; i++) {
        serial_->print(param_list[i]);
        if (i != n_params - 1) {
            serial_->print(",");
        }
    }
    serial_->print("\r\n");
    serial_->flush();

    delay(20);

    #ifdef LORA_DEBUG
    Serial.println("[LORA:DEBUG:write_command] Waiting for a response...");
    #endif


    // Read the response from the module
    // Successfuly command reply: OK<CR>
    // Command fail reply:ERROR:n<CR>
    char response[100];
    unsigned long start = millis();
    int index = 0;
    while (millis() - start < 1000) {
        char c = serial_->read();
        if (c == '\r' || c == '\n') {
            break;
        }
        
        response[index++] = c;
    }
    response[index] = '\0';

    #ifdef LORA_DEBUG
    Serial.print("[LORA:DEBUG:write_command] Response: ");
    Serial.println(response);
    #endif

    // Check if the response contains "OK" or "ERROR"
    if (strstr(response, "OK") != NULL) {
        return true; // Command successful
    } else if (strstr(response, "ERROR") != NULL) {
        errno_ = atoi(strstr(response, ":") + 1); // Extract error number
        return false; // Command failed
    }

    // If the response is neither "OK" nor "ERROR", treat it as an error
    errno_ = HOPEHM_STAT::ERR_UNKNOWN; // Unknown error
    return false; // Command failed
}

bool HopeHM::write_command(char* cmdtype, char* param) {
    #ifdef LORA_DEBUG
    Serial.print("[LORA:DEBUG] Sending command: ");
    Serial.print(cmdtype);
    Serial.print(" with param: '");
    Serial.print(param);
    Serial.println("' ...");
    #endif
    char** param_list = new char*[1];
    param_list[0] = param;
    bool res = write_command(cmdtype, param_list, 1);
    #ifdef LORA_DEBUG
    Serial.print("[LORA:DEBUG] Command result: ");
    Serial.println(res ? "OK" : "ERROR");
    #endif
    delete[] param_list;
    return res;
}

bool HopeHM::set_tx_power(HOPEHM_TX_POWER pwr) {
    return helper_write_char("POWER", pwr);
}

bool HopeHM::set_channel(HOPEHM_CHANNEL channel) {
    channel_ = channel;
    return helper_write_char("CS", channel);
}

bool HopeHM::set_enable_crc(bool enable) {
    return helper_write_char("LRCRC", enable ? '1' : '0');
}

bool HopeHM::set_lora_bandwidth(HOPEHM_BANDWIDTH bw) {
    return helper_write_char("LRSBW", bw);
}

bool HopeHM::set_lora_spreading_factor(HOPEHM_SPREADINGFACTOR sf) {
    return helper_write_char("LRSF", sf);
}

bool HopeHM::set_lora_codingrate4(HOPEHM_CODINGRATE4 cr) {
    return helper_write_char("LRCR", cr);
}

void HopeHM::begin_config() {
    digitalWrite(config_pin_, LOW);
    delay(10);
}

void HopeHM::end_config() {
    digitalWrite(config_pin_, HIGH);
    delay(10);
}

bool HopeHM::transmit(uint8_t* data, uint8_t len) {
    if (len > 255) {
        errno_ = HOPEHM_STAT::ERR_TX_TOO_LONG; // Data too long
        return false;
    }

    if (len == 0) {
        errno_ = HOPEHM_STAT::ERR_TX_NO_DATA; // No data to send
        return false;
    }

    end_config(); // End config mode before transmission

    size_t written = serial_->write(data, len);

    if (written != len) {
        errno_ = HOPEHM_STAT::ERR_TX_WRITE_FAIL; // Write failed
        return false;
    }

    // Serial.println();
    serial_->flush(); // Wait for transmission to complete
    return true;

}