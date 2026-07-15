# RF Directional Tracker

Passive-sensing, autonomous tracking RF signal direction finding system using three nodes with NRF24L01 transceivers and a weighted algorithm to point towards an RF emitter across 360 degrees.


Motivated through operational EOD/military training/experience with RF use found everywhere. CMD-detonated/RF-triggered IEDs use RF signals to function, UGVs/UAVs are commonplace and are commonly controlled with RF, not to mention all radio systems. The design of this project is directly applicable to RF signal emitter detection.


## Hardware

- 5x NRF24L01 Transceivers
- Arduino Uno, 3x Arduino Nanos, 1x ESP-32 DevKit (what I had on hand, can essentially using any pairing but requires 4 minimum and note ESP-32 is dual core so it can handle two simultaneous processes, but not recommended)
- Stepper Motor


## Architecture

- Transmitter: Arduino Uno w/ NRF24L01
- 3x Receiver Nodes: Arduino Nanos each w/ NRF24L01
- Brain Node: ESP32 w/ NRF24L01 and 28BYJ-48 stepper motor

- Transmission operate on 433MHz
- Transmitter set on ch100
- Receiver nodes receive on ch100 and transmit to Brain Node on ch76 via token-activated polling


## Status

Stage 1: One receiver and one transmitter setup. RSSI proxy via packet counting method from continuous transmissions. The closer the transmitter the higher the packet count, the great the assumed RSSI, and vice versa.

Stage 2: Two receivers, one transmitter, and one brain to calculate direction over 180 degrees based on RSSI values. For the two receiving modules, each transceiver receives, then transmits the data to the third processor where the RSSI transmit data is used to determine approximate direction of incoming transmit signal. 

(Notes: Known issue using only one transceiver per node is the transmission window of the node to the brain creates a gap where the Node isn't listening for the transmitter, reducing RSSI values per cycle; solution: use two transceivers per node. Additionally, With two receivers, system capability is reduced to 180 degrees due to ambiguity of signal direction; three receivers needed to unlock 360 degree tracking).

Stage 3: Three receivers set up in an equilateral triangle for 360 degree tracking.  

Unnecessary to proceed beyond three receivers to account for the "cone of confusion" ambiguity in 3D space as per the motivation, it wouldn't be expect to have to account for signals other than those mostly on the ground plane.


## Technical Decisions
- Separate channel use (ch100 and ch76) to eliminate RF interference
- Using token-activated polling from the Brain to eliminate cross-node responses to the polls
- Third node not attached to brain, but a separate entity; while ESP32 is capable of handling brain functions and receiver functions, a third node reduces inefficiencies
-Squared RSSI values and adaptive Kalman filter to have better direction response due to fluctuations and uncertainties of RF.


## Known Limitations
- One NRF24L01 transceiver per node requires channel switching. This leads to a gap where the nodes are not continuously listening for the transmitter
- Random fluctuations in RF signals reduce the accuracy of tracking; choice of hardware limits tracking ability, especially when RSSI proxy values are low
- Using packet count to approximate RSSI based on distance

## Future Improvements
- Two NRF24L01 transceivers per node to eliminate channel switching
- Magnetometer or rotary encoder for precise servo/stepper position
- Additional receiver nodes for more robust data to reduce inefficiences

