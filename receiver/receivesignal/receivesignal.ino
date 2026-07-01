// #include <RH_ASK.h>
// #include <SPI.h>

// (Speed, Receive, Transmit (not used), PTT, PTT inverted)
// RH_ASK rf_driver(2000, 12, 0, 0, false); 

const int rssiPin = A0; // analog pin
int rssiValue = 0;
int smoothedRssi = 0;
const int window = 20;
int measurements[20];
int measurementIndex = 0;

//unsigned long lastCountTime = 0;
//const int countInterval = 1000; // packet count per second

void setup() {
    Serial.begin(115200);
    delay(1000);

    for (int i = 0; i < window; i++){
      measurements[i] = 0;
    }
}

void loop() {
    rssiValue = analogRead(rssiPin); // takes reading

    // Takes average reading over time period to smooth out receiver measurements
    measurements[measurementIndex] = rssiValue;
    measurementIndex = (measurementIndex + 1) % window;

    int sum = 0;
    for (int i = 0; i < window; i++){
      sum += measurements[i];
    }
    smoothedRssi = sum / window;

    Serial.print("RSSI: ");
    Serial.println(smoothedRssi);

    delay (50);

}

