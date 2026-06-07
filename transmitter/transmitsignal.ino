#include <RH_ASK.h>

// Use GPIO 27 for Transmit, GPIO 14 for Receive (unused)
// The 'false' at the end disables PTT which can cause crashes
RH_ASK rf_driver(2000, 14, 27, 0, false); 

void setup() {
    Serial.begin(115200);
    delay(1000); // Give the serial monitor time to catch up
    
    Serial.println("Starting RF...");
    if (!rf_driver.init()) {
        Serial.println("Init failed!");
    } else {
        Serial.println("Init success!");
    }
}

void loop() {
    const char *msg = "test";
    rf_driver.send((uint8_t *)msg, strlen(msg));
    rf_driver.waitPacketSent();
    Serial.println("Sent");
    delay(10000);
}