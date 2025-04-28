#include <Arduino.h>
#include <test_util.h>



void setup() {
  #ifdef TEST_ENV
    // Run test setup
    YIPPEE_TEST_SETUP();
    return;
  #endif
  
  // Run main setup
}

void loop() {
  #ifdef TEST_ENV
    // Run test loop and ignore other code.
    YIPPEE_TEST_LOOP();
    return;
  #endif


}
