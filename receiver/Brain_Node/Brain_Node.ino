#include <SPI.h>
#include <RF24.h>
#include <Stepper.h>

RF24 radio(4, 5);

const byte pollNode1[6] = "POLL1";
const byte pollNode2[6] = "POLL2";
const byte pollNode3[6] = "POLL3";
const byte reprt1[6] = "RPRT1";
const byte reprt2[6] = "RPRT2";
const byte reprt3[6] = "RPRT3";

int rssi1 = 0;
int rssi2 = 0;
int rssi3 = 0;

const int STEPS_PER_REV = 2048; //360 degree roation
Stepper stepper(STEPS_PER_REV, 13, 12, 14, 25); // (IN1, IN3, IN2, IN4)
int currentStepperAngle = 0; // degrees, 0-359
const int angleThreshold = 3; // chnage in angle must exceed to move stepper

struct RSSIReport {
  uint16_t nodeNum;
  uint16_t rssi;
};

int pollNode(const byte* pollAddress, const byte* reportAddress, int expectedNode) {
  
  // Flush any noise before starting to remove garbage
  radio.flush_rx();

  // Listen for response first
  radio.openReadingPipe(1, reportAddress);
  radio.startListening();

  unsigned long pollStart = millis();

  // Keep sending poll until response received or 2 second timeout
  while (millis() - pollStart < 2000) {

    // Send poll
    radio.stopListening();
    radio.openWritingPipe(pollAddress);
    uint8_t request;
    request = (uint8_t)expectedNode; // 1, 2, 3

    radio.write(&request, sizeof(request));

    //Give radio time to switch modes
    delay(50);

    // Listen for response
    radio.openReadingPipe(1, reportAddress);
    radio.startListening();

    delay(50);

    // Wait briefly for response
    unsigned long waitStart = millis();
    while (millis() - waitStart < 50) {
      if (radio.available()) {
        RSSIReport report;
        radio.read(&report, sizeof(report));

        // Verify this packet came from the expected node
        // sometimes NRF24L01's receive packets address to similar addresses (one char off)
        if (report.nodeNum == expectedNode) {
        Serial.print("NODE");
        Serial.print(report.nodeNum);
        Serial.print(" RSSI: ");
        Serial.println(report.rssi);
        return report.rssi;
        }
        else{
          // Wrong node, discard and keep waiting
          Serial.print("Discarded packet from NODE");
          Serial.println(report.nodeNum);
          radio.flush_rx();
        }
      }
    }
    // No response yet - loop and send poll again
  }

  Serial.println("Timeout after 2 seconds");
  return -1;
}

// Calculate target angle (Node1 0 degrees, Node2, 120 degrees, Node3 240 degrees)
int calculateAngle(int r1, int r2, int r3) {
  
  // doesn't change position if signal strength is too low
  if (r1 < 100 && r2 < 100 && r3 < 100) {
    Serial.println("Signal too weak, holding position");
    return currentStepperAngle;
  }

  // doesn't change position if difference between RSSI is too small
  int maxRSSI = max(r1, max(r2, r3));
  int minRSSI = min(r1, min(r2, r3));
  int spread = maxRSSI - minRSSI;

  if (spread < 50) {
    Serial.println("Low differential - holding position");
    return currentStepperAngle;
  }

  // Square values
  long r1sq = (long)r1 * r1;
  long r2sq = (long)r2 * r2;
  long r3sq = (long)r3 * r3;
  long total = r1sq + r2sq + r3sq;

  if (total == 0) return currentStepperAngle;

  float x = 0;
  float y = 0;

  x += (float)r1sq * cos(0 * PI / 180.0);
  x += (float)r2sq * cos(120 * PI / 180.0);
  x += (float)r3sq * cos(240 * PI / 180.0);

  y += (float)r1sq * sin(0 * PI / 180.0);
  y += (float)r2sq * sin(120 * PI / 180.0);
  y += (float)r3sq * sin(240 * PI / 180.0);

  float angleRad = atan2(y, x);
  int angleDeg = (int)(angleRad * 180.0 / PI);
  if (angleDeg < 0) angleDeg += 360;

  return angleDeg;
}

// Move stepper to target angle
void moveToAngle(int targetAngle) {
  // Calculate shortest path to target
  int diff = targetAngle - currentStepperAngle;

  // Normalize to -180 to 180 for shortest path
  if (diff > 180) diff -= 360;
  if (diff < -180) diff += 360;

  // Only move if outside deadband
  if (abs(diff) <= angleThreshold) {
    Serial.print("Angle Unchanged: ");
    Serial.println(currentStepperAngle);
    return; // break
  }

  // Convert angle difference to steps
  int steps = (int)((float)diff / 360.0 * STEPS_PER_REV);

  Serial.print("Moving ");
  Serial.print(diff);
  Serial.print(" degrees (");
  Serial.print(steps);
  Serial.println(" steps)");

  stepper.setSpeed(10);
  stepper.step(steps); // positive = clockwise, negative = counterclockwise

  // Update tracked angle
  currentStepperAngle = targetAngle;
  if (currentStepperAngle < 0) currentStepperAngle += 360;
  if (currentStepperAngle >= 360) currentStepperAngle -= 360;

  Serial.print("Now at: ");
  Serial.println(currentStepperAngle);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  stepper.setSpeed(10);

  if (!radio.begin()) {
    Serial.println("Radio not detected");
    while(1);
  }

  radio.setPALevel(RF24_PA_MIN);
  radio.setAutoAck(false);
  radio.setChannel(76);
  // radio.setDataRate(RF24_250KBPS);

  Serial.println("Brain ready");
}

void loop() {
  // Wait for nodes to finish counting
  // 2200ms gives nodes full 2000ms count plus 100ms margin
  Serial.println("Waiting for nodes to count...");
  delay(2200);

  Serial.println("Polling NODE1...");
  rssi1 = pollNode(pollNode1, reprt1, 1);

  // pause between polls
  delay(10);

  Serial.println("Polling NODE2...");
  rssi2 = pollNode(pollNode2, reprt2, 2);

  delay(10);

  Serial.println("Polling NODE3...");
  rssi3 = pollNode(pollNode3, reprt3, 3);

  if (rssi1 == -1 || rssi2 == -1 || rssi3 == -1) {
    if (rssi1 == -1) Serial.println("NODE1 not responding");
    if (rssi2 == -1) Serial.println("NODE2 not responding");
    if (rssi3 == -1) Serial.println("NODE3 not responding");
    return;
  }

  // get target angle
  int targetAngle = calculateAngle(rssi1, rssi2, rssi3);

  Serial.print("NODE1: "); Serial.print(rssi1);
  Serial.print(" | NODE2: "); Serial.print(rssi2);
  Serial.print(" | NODE3: "); Serial.print(rssi3);
  Serial.print(" | Target angle: "); Serial.println(targetAngle);

  // powering transceiver down allows ESP32 to give power to turn the stepper
  radio.powerDown();
  delay(10); // let radio fully power down

  // Move stepper to target
  moveToAngle(targetAngle);

  // Power radio back up
  radio.powerUp();
  delay(10); // let radio fully power up before next poll cycle


  // After polling, both nodes start their nexs 1s count cycle, 2200ms delay keeps them in sync
}