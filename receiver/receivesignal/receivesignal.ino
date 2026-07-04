#include <SPI.h>
#include <RF24.h>
#include <Servo.h>

// CE pin 9, CSN pin 10
RF24 radio(9, 10);

Servo servo;

const byte address[6] = "00001";

// ping counting variables
int pingCount = 0;
unsigned long windowBegin = 0;
const int windowSize = 500; // count pings per 500ms

// smoothing variables
// int rssiValue = 0;
int smoothedStrength = 0;
const int window = 3;
int measurements[3];
int measurementIndex = 0;

// Signal thresholds (adjust when needed)
const int STRONG_SIGNAL = 45;
const int MEDIUM_SIGNAL = 40;

void setup() {
    Serial.begin(115200);
    delay(1000);

   for (int i = 0; i < window; i++){
      measurements[i] = 0; 
   }

   radio.begin();
   radio.openReadingPipe(1, address);
   radio.setAutoAck(false);
   radio.setPALevel(RF24_PA_MIN);
   // radio.setDataRate(RF24_250KBPS);
   radio.startListening(); // set as receiver

   servo.attach(6);
   servo.write(90); // servo stops moving

   windowBegin = millis();
   Serial.println("Receiver Ready");
}

void loop() {
  if (radio.available()) {
    char text[32] = "";
    radio.read(&text, sizeof(text));
    pingCount++;
  }

  if (millis() - windowBegin >= windowSize){

    // Takes average reading over time period to smooth out receiver measurements
    measurements[measurementIndex] = pingCount;
    measurementIndex = (measurementIndex + 1) % window;

    int sum = 0;
    for (int i = 0; i < window; i++){
      sum += measurements[i];
    }
    smoothedStrength = sum / window;

    // Servo response based on signal
    if (smoothedStrength >= STRONG_SIGNAL) {
      // Strong signal - stop, transmitter is close
      servo.write(90);
      Serial.print("STRONG - Servo: STOP | ");
      
    } else if (smoothedStrength >= MEDIUM_SIGNAL) {
      // Medium signal - slow rotation
      servo.write(100); // slight rotation
      Serial.print("MEDIUM - Servo: SLOW | ");
      
    } else {
      // Weak signal - faster rotation, searching
      servo.write(110); // faster rotation
      Serial.print("WEAK - Servo: FAST | ");
    }

    Serial.print("Pings: ");
    Serial.print(pingCount);
    Serial.print(" | Smoothed Strength:");
    Serial.println(smoothedStrength);

    //Reset for next window
    pingCount = 0;
    windowBegin = millis();
    // delay(50);
  
  }

}

