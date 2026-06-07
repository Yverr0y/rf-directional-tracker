#include <RH_ASK.h>

// Speed (2000), RX Pin (14), TX Pin (unused)
RH_ASK rf_driver(2000, 14, 27, 0, false); 

void setup() {
    Serial.begin(115200);
    if (!rf_driver.init()) {
        Serial.println("Receiver Init Failed!");
    }
}

void loop() {
    uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
    uint8_t buflen = sizeof(buf);

    if (rf_driver.recv(buf, &buflen)) { // Non-blocking check
        buf[buflen] = '\0'; // Manually add a "Null Terminator" so the ESP32 knows where the text ends
        Serial.print("Message Received: ");
        Serial.println((char*)buf);
    }
}

