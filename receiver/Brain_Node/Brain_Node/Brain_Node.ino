#include <SPI.h>
#include <RF24.h>
#include <Servo.h>

RF24 radio(9, 10);

// Servo variables
Servo servo;
const int SERVO_PIN = 6;
const int SERVO_STOP = 90;
const int SERVO_LEFT_SLOW = 80;
const int SERVO_LEFT_FAST = 70;
const int SERVO_RIGHT_SLOW = 100;
const int SERVO_RIGHT_FAST = 110;

// addresses
// const byte node1Address[6] = "NODE1";
// const byte node2Address[6] = "NODE2";
const byte brainAddress1[6] = "BRAIN1"; // receive Node1
const byte brainAddress2[6] = "BRAIN2"; // receive Node2

// RSSI variables
// RSSI storage
int rssi1 = 0;
int rssi2 = 0;
unsigned long lastReport1 = 0;
unsigned long lastReport2 = 0;
const int reportTimeout = 2000; // if no report in 2 secs consider node disconnected

// Differential threshold in signal for servo to respond
const int DiffThreshold = 10;

// RSSI report struct matching that of nodes 1 and 2
struct RSSIReport {
  char nodeID[6];
  int rssi;
};

void setup() {
  Serial.begin(115200);
  delay(1000);

  radio.begin();
  radio.openReadingPipe(1, brainAddress1);
  radio.openReadingPipe(2, brainAddress2);
  radio.setAutoAck(false);
  radio.setPALevel(RF24_PA_MIN);
  // radio.setDataRate(RF24_250KBPS);

  servo.attach(SERVO_PIN);
  servo.write(SERVO_STOP);

  radio.startListening();
  Serial.println("Brain Node Ready");
}

void loop() {
  // Check for incoming reports
  if (radio.available()) {
    uint8_t pipe;
    
    if (radio.available(&pipe)) {
      RSSIReport report;
      radio.read(&report, sizeof(report));

      // Store based on which pipe it came in on
      if (pipe == 1) {
        rssi1 = report.rssi;
        lastReport1 = millis();
        Serial.print("NODE1 RSSI: ");
        Serial.println(rssi1);
      }
      else if (pipe == 2) {
        rssi2 = report.rssi;
        lastReport2 = millis();
        Serial.print("NODE2 RSSI: ");
        Serial.println(rssi2);
      }
    }
  }

  // Check node timeouts
  bool node1alive = (millis() - lastReport1 < reportTimeout);
  bool node2alive = (millis() - lastReport2 < reportTimeout);

  // Only make servo decisions if both nodes are reporting
  if (node1alive && node2alive) {
    int differential = rssi1 - rssi2;

    Serial.print("Differential (NODE1 - NODE2): ");
    Serial.println(differential);

    if (abs(differential) < DiffThreshold) {
      // Signals too close to call - ambiguity zone
      // Slow sweep to find better angle
      servo.write(SERVO_RIGHT_SLOW);
      Serial.println("Status: AMBIGUOUS - slow sweep");
    }
    else if (differential > 0) {
      // NODE1 stronger
      if (differential > 50) {
        servo.write(SERVO_LEFT_FAST);
        Serial.println("Status: STRONG LEFT");
      } else {
        servo.write(SERVO_LEFT_SLOW);
        Serial.println("Status: WEAK LEFT");
      }
    }
    else {
          // NODE2 stronger
          if (abs(differential) > 50) {
            servo.write(SERVO_RIGHT_FAST);
            Serial.println("Status: STRONG RIGHT");
          } else {
            servo.write(SERVO_RIGHT_SLOW);
            Serial.println("Status: WEAK RIGHT");
          }
        }
      }
      else if (!node1alive && !node2alive) {
        // Both nodes dead - stop servo
        servo.write(SERVO_STOP);
        Serial.println("Status: NO SIGNAL");
      }
      else {
        // Only one node reporting - limited info
        servo.write(SERVO_STOP);
        Serial.println("Status: PARTIAL SIGNAL");
      }

  delay(100); // update servo 10 times per second

}


