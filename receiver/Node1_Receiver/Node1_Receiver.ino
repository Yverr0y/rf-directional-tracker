#include <SPI.h>
#include <RF24.h>

RF24 radio(9, 10);

const byte transmitterAddress[6] = "00001";
const byte pollAddress[6] =        "POLL1";
const byte reportAddress[6] =      "RPRT1";
const int myNodeNum = 1;

int packetCount = 0;
int smoothedStrength = 0;

const int smoothingWindow = 10;
int readings[10];
int readingIndex = 0;

const int COUNT_DURATION = 2000; // count for 2 second to increase packet count

struct RSSIReport {
  uint16_t nodeNum;
  uint16_t rssi;
};


void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!radio.begin()) {
    Serial.println("Radio not detected");
    while(1);
  }

  radio.setPALevel(RF24_PA_MIN);
  radio.setAutoAck(false);
  radio.setChannel(76);
  // radio.setDataRate(RF24_250KBPS);

  for (int i = 0; i < smoothingWindow; i++) {
    readings[i] = 0;
  }

  // Start counting transmitter packets
 // radio.openReadingPipe(1, transmitterAddress);
 // radio.startListening();

  Serial.print("NODE");
  Serial.print(myNodeNum);
  Serial.println(" ready");
}

void loop() {
  // PHASE 1 - Count transmitter packets for 1 second

  // When counting transmitter packets
  radio.setChannel(100);
  radio.openReadingPipe(1, transmitterAddress);
  radio.startListening();
  delay(20);

  Serial.println("Counting...");
  unsigned long countStart = millis();
  packetCount = 0;

  while (millis() - countStart < COUNT_DURATION) {
    if (radio.available()) {
      char text[32] = "";
      radio.read(&text, sizeof(text));
      packetCount++;
    }
  }

  // Calculate smoothed RSSI
  readings[readingIndex] = packetCount;
  readingIndex = (readingIndex + 1) % smoothingWindow;

  int sum = 0;
  for (int i = 0; i < smoothingWindow; i++) {
    sum += readings[i];
  }
  smoothedStrength = sum / smoothingWindow;

  Serial.print("Count done - RSSI: ");
  Serial.println(smoothedStrength);

  // PHASE 2 - Switch to poll address and wait indefinitely
  radio.stopListening();
  radio.openReadingPipe(1, pollAddress);
  radio.startListening();

  Serial.println("Waiting for brain poll...");

  // When waiting for poll
  radio.setChannel(76);
  radio.openReadingPipe(1, pollAddress);
  radio.startListening();
  delay(20);

  // Wait forever until brain polls
  while (true) {
    if (radio.available()) {
      uint8_t request;
      radio.read(&request, sizeof(request));

      if (request == (uint8_t)myNodeNum){

        Serial.println("Poll received - responding");

        // In node handlePoll - before responding
        delay(50); // 1ms settling time before transmitting response

        radio.stopListening();
        radio.openWritingPipe(reportAddress);

        RSSIReport report;
        report.nodeNum = myNodeNum;
        report.rssi = smoothedStrength;
        radio.write(&report, sizeof(report));

        Serial.print("Reported RSSI: ");
        Serial.println(smoothedStrength);

        // Break out and start counting again
        break;
      }
        // keep waiting for correct poll 
    }
  }
}