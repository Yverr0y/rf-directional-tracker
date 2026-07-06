#include <SPI.h>
#include <RF24.h>
#include <Servo.h>

RF24 radio(9, 10);
Servo trackingServo;

const byte pollNode1[6] = "POLL1";
const byte pollNode2[6] = "POLL2";
const byte reprt1[6] =    "RPRT1";
const byte reprt2[6] =    "RPRT2";

int rssi1 = 0;
int rssi2 = 0;

const int SERVO_PIN = 6;
int currentAngle = 90;
const int angleThreshold = 3; // servo only moves 3+ degrees at a time

struct RSSIReport {
  int nodeNum;
  int rssi;
};

int pollNode(const byte* pollAddress, const byte* reportAddress, int expectedNode) {
  
  // Flush any stale packets before starting to remove garbage
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
    const char request[] = "POLL";
    radio.write(&request, sizeof(request));

    //Give radio time to switch modes
    delay(50);

    // Listen for response
    radio.openReadingPipe(1, reportAddress);
    radio.startListening();

    // Give radio time to settle into listen mode
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

void updateServo(int r1, int r2) {
  if ((r1 + r2) > 0) {

    // Square values to amplify differential
    long r1sq = (long)r1 * r1;
    long r2sq = (long)r2 * r2;
    
    // Calculate proportion: 0.0 = all NODE1, 1.0 = all NODE2
    float proportion = (float)r2sq / (float)(r1sq + r2sq);
    
    // Map to full servo range
    int targetAngle = (int)(proportion * 180);
    targetAngle = constrain(targetAngle, 0, 180);

    // moves servo if only by 3+ degrees to reduce movements from small RSSI fluctuations
    if (abs(targetAngle - currentAngle) > angleThreshold) {
      trackingServo.write(targetAngle*0.667); // reduce 270 degree servo to 180 degree travel
      currentAngle = targetAngle;
      Serial.print("NODE1: "); Serial.print(r1);
      Serial.print(" | NODE2: "); Serial.print(r2);
      Serial.print(" | Proportion: "); Serial.print(proportion);
      Serial.print(" | Angle: "); Serial.println(targetAngle);

    }
    else {
      Serial.print("NODE1: "); Serial.print(r1);
      Serial.print(" | NODE2: "); Serial.print(r2);
      Serial.print(" | Proportion unchanged: "); Serial.print(proportion);
      Serial.print(" | Angle unchanged: "); Serial.println(currentAngle);
  
    }
  }
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

  trackingServo.attach(SERVO_PIN);
  trackingServo.write(90);

  Serial.println("Brain ready");
}

void loop() {
  // Wait for nodes to finish counting
  // 2200ms gives nodes full 2000ms count plus 100ms margin
  Serial.println("Waiting for nodes to count...");
  delay(2200);

  // Poll both nodes - they are guaranteed to be waiting
  Serial.println("Polling NODE1...");
  rssi1 = pollNode(pollNode1, reprt1, 1);

  // Brief pause between polls
  delay(10);

  Serial.println("Polling NODE2...");
  rssi2 = pollNode(pollNode2, reprt2, 2);

  // Update servo
  if (rssi1 != -1 && rssi2 != -1) {
    updateServo(rssi1, rssi2);
  }
  else if (rssi1 == -1 && rssi2 == -1) {
    Serial.println("NO SIGNAL");
  }
  else if (rssi1 == -1) {
    Serial.println("NODE1 not responding");
  }
  else {
    Serial.println("NODE2 not responding");
  }

  // After polling both nodes they immediately start
  // their next 1 second count cycle
  // Brain's next delay(2200) keeps them in sync
}