#pragma once
#include <Arduino.h>

enum HOPEHM_STAT {
    ERR_INCORRECT_FMT = 0,
    ERR_INCORRECT_PARAM = 1,
    ERR_CMD_FAILED = 2,
    ERR_UNKNOWN = 3,
    ERR_TX_TOO_LONG = 4,
    ERR_TX_NO_DATA = 5,
    OK = 99
};

enum HOPEHM_TX_POWER {
    TX_POWER_20DB = 0,
    TX_POWER_17DB = 1,
    TX_POWER_15DB = 2,
    TX_POWER_10DB = 3,
    TX_POWER_8DB = 4,
    TX_POWER_5DB = 5,
    TX_POWER_2DB = 6
};

enum HOPEHM_CHANNEL {
    CHANNEL_0 = '0',
    CHANNEL_1 = '1',
    CHANNEL_2 = '2',
    CHANNEL_3 = '3',
    CHANNEL_4 = '4',
    CHANNEL_5 = '5',
    CHANNEL_6 = '6',
    CHANNEL_7 = '7',
    CHANNEL_8 = '8',
    CHANNEL_9 = '9',
    CHANNEL_10 = 'A',
    CHANNEL_11 = 'B',
    CHANNEL_12 = 'C',
    CHANNEL_13 = 'D',
    CHANNEL_14 = 'E',
    CHANNEL_15 = 'F',
};

enum HOPEHM_BANDWIDTH {
    BW_62_5KHZ = '6',
    BW_125KHZ = '7',
    BW_250KHZ = '8',
    BW_500KHZ = '9',
};

enum HOPEHM_SPREADINGFACTOR {
    SF_7 = '7',
    SF_8 = '8',
    SF_9 = '9',
    SF_10 = 'A',
    SF_11 = 'B',
    SF_12 = 'C'
};

enum HOPEHM_CODINGRATE4 {
    CR_4_5 = '0',
    CR_4_6 = '1',
    CR_4_7 = '2',
    CR_4_8 = '3'
};

class HopeHM {
    public:

    HopeHM(uint8_t reset_pin, uint8_t sleep_pin, uint8_t config_pin) {
        errno_ = 0;
        reset_pin_ = reset_pin;
        sleep_pin_ = sleep_pin;
        config_pin_ = config_pin;
    };
    uint8_t get_err() { return errno_; };

    void begin_config();
    void end_config();

    bool set_tx_power(HOPEHM_TX_POWER pwr);
    bool set_channel(HOPEHM_CHANNEL channel);
    bool set_enable_crc(bool enable);
    bool set_lora_bandwidth(HOPEHM_BANDWIDTH bw);
    bool set_lora_spreading_factor(HOPEHM_SPREADINGFACTOR sf);
    bool set_lora_codingrate4(HOPEHM_CODINGRATE4 cr);

    // Sending and recieving commands
    bool transmit(uint8_t* data, uint8_t len);

    bool begin(HardwareSerial* serial) {
        serial_ = serial;
        // Set default non-transmission settings
        digitalWrite(sleep_pin_, LOW);
        digitalWrite(reset_pin_, HIGH);
        digitalWrite(config_pin_, HIGH);
        delay(10);

        bool init_res = true;
        begin_config();

        init_res &= write_command("MODE", "0");
        init_res &= write_command("BAND", "0");

        end_config();
        return init_res;
    }

    private:
    
    /**
     * @brief Helper function to write a command with a single character parameter.
     * @return TRUE if successful, FALSE otherwise.
     */
    bool helper_write_char(char* cmdtype, char param) {
        char* param_str = new char[2];
        sprintf(param_str, "%c", param);
        bool res = write_command(cmdtype, param_str);
        delete param_str;
        return res;
    }

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

    HardwareSerial* serial_;
    uint8_t errno_;
    uint8_t reset_pin_;
    uint8_t sleep_pin_;
    uint8_t config_pin_;
};
