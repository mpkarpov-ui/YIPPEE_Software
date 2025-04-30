#pragma once
#include <Arduino.h>

enum HOPEHM_TX_POWER {
    TX_POWER_20DB = 0,
    TX_POWER_17DB = 1,
    TX_POWER_15DB = 2,
    TX_POWER_10DB = 3,
    TX_POWER_8DB = 4,
    TX_POWER_5DB = 5,
    TX_POWER_2DB = 6
};

class HopeHM {
    public:
    
    HopeHM();
    uint8_t get_err() { return errno_; };
    bool set_tx_power(HOPEHM_TX_POWER pwr);

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
};
