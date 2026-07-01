#include <RH_ASK.h>

// (Speed, Receive (not used), Transmit, PTT, PTT inverted)
RH_ASK rf_driver(2000, 0, 2, 0, false);

void setup() {
    Serial.begin(115200);
    delay(1000); 
    
    //ensures hardware is active
    if (!rf_driver.init()) {
        Serial.println("Init failed!");
    } 
    else {
        Serial.println("Init success!");
    }
}

void loop() {
    const char *msg = "test";
    rf_driver.send((uint8_t *)msg, strlen(msg)); // (data_pointer, data_length)

    rf_driver.waitPacketSent(); //wait for RF
    Serial.println("Sent");
    delay(1000);
}