#include <RH_ASK.h>

// (Speed, Receive, Transmit (not used), PTT, PTT inverted)
RH_ASK rf_driver(2000, 12, 0, 0, false); 

void setup() {
    Serial.begin(115200);
    delay(1000);

    checking hardware is active
    if (!rf_driver.init()) {
        Serial.println("Receiver Init Failed!");
    }
    else {
        Serial.println("Init success!");
    }
}

void loop() {
    uint8_t buf[RH_ASK_MAX_MESSAGE_LEN]; // max array to ensure maximize length of messages (67 bytes)
    uint8_t buflen = sizeof(buf); // total memory capacity to be used

    if (rf_driver.recv(buf, &buflen)) { // checks if valid message is received and updates buflen
        buf[buflen] = '\0'; // Manually add a "Null Terminator" so microcontroller knows where the text ends
        Serial.print("Message Received: ");
        Serial.println((char*)buf);
    }
}

