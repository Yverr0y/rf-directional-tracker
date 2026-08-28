#include <SPI.h>
#include <RF24.h>

RF24 radio(9, 10);

const byte tx0Address[6] = "00001";
const byte tx1Address[6] = "00002";
const byte pollAddress[6] =        "POLL1";
const byte reportAddress[6] =      "RPRT1";
const int myNodeNum = 1;

int packetCount0 = 0; // TX0 count this window
int packetCount1 = 0; // TX1 count this window
int smoothedStrength0 = 0;
int smoothedStrength1 = 0;

const int COUNT_DURATION = 2000; // count for 2 seconds to increase packet count

struct RSSIReport {
  uint16_t nodeNum;
  uint16_t rssi0;
  uint16_t rssi1;
};

// Kalman filter variables, now one set per target, since each
// transmitter can be at a different distance/movement independently
float kalmanEstimate[2] = {1000, 1000}; // mid range of expected RSSI values
float kalmanError[2] = {100, 100};
const float processNoise = 50; // large for responsiveness when transmitter moves away
const float measurementNoise = 5; // base level of noise when transmitter is close

float kalmanUpdate(float measurement, float &estimate, float &error) {
  error += processNoise;

  // normalize scale from 0 to 1 over 0 to ~2100 max packet count for RSSI
  float normal = (2100.0f - estimate) / 2100.0f;
  if (normal < 0.0f){
    normal = 0.0f;
  }

  // scales adaptive filter when transmitter moves away due to larger fluctuations
  float dynamicMeasurementNoise = measurementNoise + (normal * normal * normal * normal);
  float kalmanGain = error / (error + dynamicMeasurementNoise);
  estimate = estimate + kalmanGain * (measurement - estimate);
  error = (1 - kalmanGain) * error;

  return estimate;
}

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

  Serial.print("NODE");
  Serial.print(myNodeNum);
  Serial.println(" ready");
}

void loop() {
  // PHASE 1 - Count packets from BOTH transmitters for one window,
  // distinguishing source by which pipe the packet arrived on.

  radio.setChannel(100);
  radio.openReadingPipe(1, tx0Address);
  radio.openReadingPipe(2, tx1Address);
  radio.startListening();
  delay(20);

  Serial.println("Counting...");
  unsigned long countStart = millis();
  packetCount0 = 0;
  packetCount1 = 0;

  uint8_t pipeNum;
  while (millis() - countStart < COUNT_DURATION) {
    if (radio.available(&pipeNum)) {
      char text[32] = "";
      radio.read(&text, sizeof(text));
      if (pipeNum == 1) {
        packetCount0++;
      } else if (pipeNum == 2) {
        packetCount1++;
      }
    }
  }

  smoothedStrength0 = kalmanUpdate(packetCount0, kalmanEstimate[0], kalmanError[0]);
  smoothedStrength1 = kalmanUpdate(packetCount1, kalmanEstimate[1], kalmanError[1]);

  Serial.print("Count done - TX0 RSSI: ");
  Serial.print(smoothedStrength0);
  Serial.print(" | TX1 RSSI: ");
  Serial.println(smoothedStrength1);

  // PHASE 2 - Switch to poll address and wait indefinitely
  radio.stopListening();
  radio.openReadingPipe(1, pollAddress);
  radio.startListening();

  Serial.println("Waiting for brain poll...");

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

        delay(50); // settling time before transmitting response

        radio.stopListening();
        radio.openWritingPipe(reportAddress);

        RSSIReport report;
        report.nodeNum = myNodeNum;
        report.rssi0 = smoothedStrength0;
        report.rssi1 = smoothedStrength1;
        radio.write(&report, sizeof(report));

        Serial.print("Reported RSSI0: ");
        Serial.print(smoothedStrength0);
        Serial.print(" RSSI1: ");
        Serial.println(smoothedStrength1);

        // Break out and start counting again
        break;
      }
      // keep waiting for correct poll
    }
  }
}
