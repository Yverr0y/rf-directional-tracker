# RF Directional Tracker
Autonomous RF signal direction tracker using dual 433MHz receivers with servo actuation to point towards an RF emitter.


## Motivation
CMD-detonated/RF-triggered IEDs use RF signals to function. UASs are becoming commonplace and are commonly controlled with RF. Locating where the transmission is coming from can help uncover where a triggerman or pilot may be located.

## Hardware
- 1x 433MHz transmitter module (FS1000A)
- 2x 433MHz receiver modules (MX-RM-5V)
- Arduino Uno
- Servo Motor

## Status
Active Development, currently establishing baseline RSSI reading and servo control

## How it works
Two receivers placed at a fixed offset compare incoming signal strength. Servo adjusts position to point towards the stronger signal source, tracking the RF in real time.