#pragma once
#include <Arduino.h>

class HopeHM {
    HopeHM(HardwareSerial* serial);

    uint8_t get_err() { return errno_; };

    private:
    /**
     * @brief Writes a command with a single parameter
     * @return TRUE if successful, FALSE otherwise.
     */
    bool write_command(char* cmdtype, char* param);

    /**
     * @brief Writes a command with any number of parameters.
     * @return TRUE if successful, FALSE otherwise.
     */
    bool write_command(char* cmdtype, char** param_list, int n_params);
    int read_command(char* cmdtype, char** return_list);

    uint8_t errno_;
    HardwareSerial* serial_;
}
