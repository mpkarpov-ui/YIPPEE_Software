#include "HopeHM.h"


HopeHM::HopeHM() {
    errno_ = 0;
}


bool HopeHM::write_command(char* cmdtype, char** param_list, int n_params) {
    Serial.print("AT+");
    Serial.print(cmdtype);
    Serial.print("=");
    for (int i = 0; i < n_params; i++) {
        Serial.print(param_list[i]);
        if (i != n_params - 1) {
            Serial.print(",");
        }
    }
    Serial.print("<CR>\r\n");
    Serial.flush();

    // Read the response from the module
    // Successfuly command reply: OK<CR>
    // Command fail reply:ERROR:n<CR>
    char response[100];

    int index = 0;
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\r' || c == '\n') {
            break;
        }
        response[index++] = c;
    }
    response[index] = '\0';

    // Check if the response contains "OK" or "ERROR"
    if (strstr(response, "OK") != NULL) {
        return true; // Command successful
    } else if (strstr(response, "ERROR") != NULL) {
        errno_ = atoi(strstr(response, ":") + 1); // Extract error number
        return false; // Command failed
    }

    // If the response is neither "OK" nor "ERROR", treat it as an error
    errno_ = 3; // Unknown error
    return false; // Command failed
}

bool HopeHM::write_command(char* cmdtype, char* param) {
    char** param_list = new char*[1];
    param_list[0] = param;
    bool res = write_command(cmdtype, param_list, 1);
    delete param_list;
    return res;
}

bool HopeHM::set_tx_power(HOPEHM_TX_POWER pwr) {
    char* param = new char[2];
    sprintf(param, "%d", pwr);
    bool res = write_command("POWER", param);
    delete param;
    return res;
}
