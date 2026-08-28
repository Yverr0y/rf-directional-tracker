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

int rssi0[3] = {0, 0, 0};
int rssi1[3] = {0, 0, 0};

const int STEPS_PER_REV = 2048; //360 degree roation
Stepper stepper(STEPS_PER_REV, 13, 12, 14, 25); // (IN1, IN3, IN2, IN4)
int currentStepperAngle = 0; // degrees, 0-359 - TODO: needs to become per-target
// if you want both bearings physically indicated; see note at bottom
const int angleThreshold = 3; // change in angle must exceed to move stepper

struct RSSIReport {
  uint16_t nodeNum;
  uint16_t rssi0;
  uint16_t rssi1;
};

// single poll call gets both targets' data at once
bool pollNode(const byte* pollAddress, const byte* reportAddress, int expectedNode,
              int &outRssi0, int &outRssi1) {

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
          Serial.print(" RSSI0: ");
          Serial.print(report.rssi0);
          Serial.print(" RSSI1: ");
          Serial.println(report.rssi1);
          outRssi0 = report.rssi0;
          outRssi1 = report.rssi1;
          return true;
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
  return false;
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
  bool ok1 = pollNode(pollNode1, reprt1, 1, rssi0[0], rssi1[0]);
  
  // pause between polls
  delay(10);

  Serial.println("Polling NODE2...");
  bool ok2 = pollNode(pollNode2, reprt2, 2, rssi0[1], rssi1[1]);

  delay(10);

  Serial.println("Polling NODE3...");
  bool ok3 = pollNode(pollNode3, reprt3, 3, rssi0[2], rssi1[2]);

  if (!ok1 || !ok2 || !ok3) {
    if (!ok1) Serial.println("NODE1 not responding");
    if (!ok2) Serial.println("NODE2 not responding");
    if (!ok3) Serial.println("NODE3 not responding");
    return;
  }

  // Compute bearing to each target independently
  int targetAngle0 = calculateAngle(rssi0[0], rssi0[1], rssi0[2], currentStepperAngle);
  int targetAngle1 = calculateAngle(rssi1[0], rssi1[1], rssi1[2], currentStepperAngle);

  Serial.print("TX0 -> N1: "); Serial.print(rssi0[0]);
  Serial.print(" | N2: "); Serial.print(rssi0[1]);
  Serial.print(" | N3: "); Serial.print(rssi0[2]);
  Serial.print(" | Angle: "); Serial.println(targetAngle0);

  Serial.print("TX1 -> N1: "); Serial.print(rssi1[0]);
  Serial.print(" | N2: "); Serial.print(rssi1[1]);
  Serial.print(" | N3: "); Serial.print(rssi1[2]);
  Serial.print(" | Angle: "); Serial.println(targetAngle1);

  // Stepper Code removed
  radio.powerDown();
  delay(10);

  moveToAngle(targetAngle0);

  radio.powerUp();
  delay(10);
}
