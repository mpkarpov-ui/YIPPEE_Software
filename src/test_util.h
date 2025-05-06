#pragma once
// This file helps run any test utility functions to test components. This lets us avoid having different repositories, and we can instead call these test functions.

// Define configuration
// #define TEST_MCU
// #define TEST_BAROMETER
// #define TEST_GPS
// #define TEST_LED
// #define TEST_MEM
// #define TEST_MEM_2
// #define TEST_LOGGER
#define TEST_LORA
// #define DISABLE_LOOP

// Define functions
void YIPPEE_TEST_SETUP();
void YIPPEE_TEST_LOOP();
