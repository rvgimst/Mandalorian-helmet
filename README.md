# Mandalorian-helmet
Project to create a 3D printed mano helmet and add sounds and lights to it using Arduino with LEDs

Used ideas and files from:
https://www.myminifactory.com/object/3d-print-mandalorian-helmet-v2-106137

https://all3dp.com/2/3d-printed-mandalorian-helmet-3d-models/

Note on hardware I used:
========================
I used an Arduino Nano Every which worked fine on the breadboard, powered through its micro-USB port and the 5V pin connected to the breadboard's power strip (Vin not used).
When soldered on a protoboard in the device, the Arduino is powered through the Vin pin (5V pin not used).
When trying to connect the micro-USB port to the PCs USB port for uploading the code to the Arduino, it didn't work. Turns out that the Arduino powers the Vin port (which powers the MP player and all other components) and that it fails the PC to even recognize the Arduino. When ALSO powered through the power input of the device (Mando helmet), uploading worked!

