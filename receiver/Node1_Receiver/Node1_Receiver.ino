#include <SPI.h>
#include <RF24.h>

RF24 radio(9, 10);

const byte myAddress[6] = "NODE1";
const byte brainAddress[6] = "BRAIN"; // send to 
const byte transmitAddress[6] = "00001"; // listen to receive

// packet counting variables
int packetCount = 0;
unsigned long listenBegin = 0;
// const int windowSize = 500; // count pings per 500ms
const int listenWindow = 450; // listening for signals for 450ms
const int transmitWindow = 50; // transmit to BRAIN over 50ms window

// smoothing variables
// int rssiValue = 0;
int smoothedStrength = 0;
const int window = 5;
int measurements[5];
int measurementIndex = 0;

// Timing Offset to distinguish NODE1 and NODE2
const int startOffset = 0;
// bool offsetDone = false;
bool reported = false;

void setup() {
  Serial.begin(115200);
    delay(1000 + startOffset); // stagger startup (can delete for node1)

   for (int i = 0; i < window; i++){
      measurements[i] = 0; 
   }

   radio.begin();
   radio.openReadingPipe(1, transmitAddress);
   radio.setAutoAck(false);
   radio.setPALevel(RF24_PA_MIN);
   // radio.setDataRate(RF24_250KBPS);
   radio.startListening(); // set as receiver

   listenBegin = millis();
   Serial.println("Receiver Node Ready");

}

void loop() {
  // Handle startup offset for NODE2 to avoid collision
  // if (!offsetDone) {
  //  delay(startOffset);
  //  offsetDone = true;
  //  listenBegin = millis();
  //}

  if (millis() - listenBegin < listenWindow) {
    if (radio.available()) {
      char text[32] = "";
      radio.read(&text, sizeof(text));
      packetCount++;
    }
  }

  else if (millis() - listenBegin < listenWindow + transmitWindow) {

    // Only transmit once per cycle
    if (!reported) {
      // Takes average reading over time period to smooth out receiver measurements
      measurements[measurementIndex] = packetCount;
      measurementIndex = (measurementIndex + 1) % window;

      int sum = 0;
      for (int i = 0; i < window; i++){
        sum += measurements[i];
      }
      smoothedStrength = sum / window;

      // Switch to transmit mode
      radio.stopListening();
      radio.openWritingPipe(brainAddress);

      // Send node ID and RSSI value as a struct
      struct RSSIReport {
        char nodeID[6];
        int rssi;
      };

      RSSIReport report;
      strcpy(report.nodeID, myAddress);
      report.rssi = smoothedStrength;

      bool success = radio.write(&report, sizeof(report));

      if (success) {
       // Serial.print(myAddress);
        Serial.print("Reported RSSI: ");
        Serial.println(smoothedStrength);
      } 
      else {
        Serial.println("Report failed");
      }

      // Switch back to listen mode
      radio.openReadingPipe(1, transmitAddress);
      radio.startListening();

      radio.flush_rx(); // clear any garbage in receive accumulated during transmit phase

      reported = true;
      packetCount = 0;
    }
  }

  // Reset for next window
  else {
    listenBegin = millis();
    reported = false;
  }

}
