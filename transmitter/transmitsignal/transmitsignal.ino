#include <SPI.h>
#include <RF24.h>

// CE pin 9, CSN pin 10
RF24 radio(9, 10);

const byte address[6] = "00001";

void setup() {
    Serial.begin(115200);
    delay(1000); 

    radio.begin();
    radio.setAutoAck(false); // ignores protocal to receive acknowledgement of message sent to receiver
    radio.openWritingPipe(address);
    radio.setPALevel(RF24_PA_MIN); // min power for close range testing
    // radio.setDataRate(RF24_250KBPS); // slower rate, can be more reliable for NRF24L01 PA+LNA
    radio.stopListening(); // set as transmitter

    Serial.println("Transmitter ready");
}

void loop() {
    const char text[] = "ping";
    bool success = radio.write(&text, sizeof(text));

    if(success){
        Serial.println("Sent:ping");
    }
    else{
        Serial.println("Send failed");
    }

    delay(100); // 10 pings per second
}