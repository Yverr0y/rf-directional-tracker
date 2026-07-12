# RF Directional Tracker

Autonomous RF signal direction tracker using NRF24L01 transceivers with stepper motor actuation to point towards an RF emitter.



## Motivation

CMD-detonated/RF-triggered IEDs use RF signals to function. UASs are becoming commonplace and are commonly controlled with RF. Locating where the transmission is coming from can help uncover where a triggerman or pilot may be located.


## Hardware

- 5x NRF24L01 Transceivers
- Arduino Uno, 3x Arduino Nanos, 1x ESP-32 DevKit (what I had on hand, can essentially using any pairing but requires 4 minimum and note ESP-32 is dual core so it can handle two simultaneous processes, but not recommended)
- Stepper Motor


## Status

Active Development, 2 receivers send reliable packets to Brain Node and manipulates servo.

Stage 1: 1 receiver and 1 transmitter with servo action changing based on proximity of transmit. At close range the servo doesn't spin, at a medium range it spins slowly, and at a far transmit range it spins faster.

Stage 2: 2 receivers and 1 transmitter. For the two receiving modules, each transceiver receives, then transmits the data to the third processor where the RSSI transmit data is used to determine approximate direction of incoming transmit signal via servo moving to point in transmit direction. (Notes: Known issue using only one transceiver per Node is the transmission window of the Node to the Brain creates a gap where the Node isn't listening for the transmitter reducing RSSI values per cycle; solution use two transceivers per Node. Additionally, With two receivers system capability is reduced to 180 degree plane so servo is reduced to 180 degree rotation; three receivers needed to unlock 360 degree tracking).

Stage 3: 3 receivers to account for short coming of using only 2 receivers where there is 180 degree ambiguity in direction signal is transmitted from. (Switched brain to ESP-32 Devkit and attempted dual core to handle receiving and transmitting simultaneously and more reliably; another solution to the listening gap discussed in stage 2. 3rd receiver was be attached to Brain ESP-32 doing the directional computation. Shared SPI bus led to cores competing and packets being lost. Considered dual SPI bus for zero core contention, but stepper motor (replaced servo) requires the SPI bus. Ultimately, three identical separate receiver nodes (3x Nanos) is the clearer, more optimal solution given hardware constraints)

Unnecessary to proceed beyond three receivers to account for the "cone of confusion" ambiguity in 3D space as per the motivation, it wouldn't be expect to have to account for signals other than those mostly on the ground plane.


## How it works

Three receivers placed at a fixed offset compare incoming signal strength. Stepper motor adjusts position to point towards the direction of the strongest signal source, tracking the RF in real time.

