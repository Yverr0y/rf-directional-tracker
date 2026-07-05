# RF Directional Tracker

Autonomous RF signal direction tracker using NRF24L01 transceivers with servo actuation to point towards an RF emitter.



## Motivation

CMD-detonated/RF-triggered IEDs use RF signals to function. UASs are becoming commonplace and are commonly controlled with RF. Locating where the transmission is coming from can help uncover where a triggerman or pilot may be located.


## Hardware

- 4x NRF24L01 Transceivers
- Arduino Uno, 3x Arduino Nanos (can using any pairing but requires 4 total)
- 360 degree Servo Motor


## Status

Active Development, currently establishing baseline RSSI reading and servo control.

Stage 1: 1 receiver and 1 transmitter with servo action changing based on proximity of transmit. At close range the servo doesn't spin, at a medium range it spins slowly, and at a far transmit range it spins faster.

Stage 2: 2 receivers and 1 transmitter. For the two receiving modules, each transceiver receives, then transmits the data to the third processor where the RSSI transmit data is used to determine approximate direction of incoming transmit signal via servo moving to point in transmit direction

Stage 3: 3 receivers to account for short coming of using only 2 receivers where there is 180 degree ambiguity in direction signal is transmitted from. (3rd receiver will be attached to Brain Nano doing the directional computation. While it is cleaner and simpler to have a separate Nano and receiver I've opted to place two NRF24L01 transceivers on the Brain Nano)

Unnecessary to proceed beyond three receivers to account for the "cone of confusion" ambiguity in 3D space as per the motivation, it wouldn't be expect to have to account for signals other than those mostly on the ground plane.


## How it works

Three receivers placed at a fixed offset compare incoming signal strength. Servo adjusts position to point towards the stronger signal source, tracking the RF in real time.

