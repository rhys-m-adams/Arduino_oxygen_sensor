# Arduino_oxygen_sensor

I made a circuit board to detect gas oxygen and display it on the internet. This is not a liquid oxygen sensor. This could be useful for anaerobic chamber monitoring.

Parts:
```
1 conductive paint/solder "910-CP1-5S CP1-5S Conductive Paint LR"
1 2462 OpAmp
2 10 uF electrolytic capacitors
2 100 nF capacitors
1 10 ohm resistor
2 4.7k ohm resistors
3 10k ohm resistors
1 MKR wifi 1010 arduino chip
1 560 ohm resistor
1 LCD display with potentiometer resistor "1528-1502-ND LCD MOD 32DIG 16X2 TRANSMISV WHT"
1 EC410 oxygen sensor
1 outlet to USB MICRO B adapter. I used the two parts to accomplish this:
 490-SWI3-5-N-I38 SWI3-5-N-I38
 RoHS1 895-CUB-100-BK CUB-100-BK USB A To USB MICRO B

1 circuit board
```
You can buy the circuit board here:
https://oshpark.com/shared_projects/SpwukUbW

Solder all the parts in, except for the O2 sensor. For the O2 sensor I used conductive paint since the instructions say that heat can damage it.
![Front](front.JPG "Front")
![Back](back.JPG "back")
Please excuse the wild wire lengths - I didn't have access to a proper wire cutter when I made this board so I used an ill-suited plier with cutting edges that was too thick for electronics.

In the Arduino IDE, I had to install the board manager for MKR Wifi 1010 by installing "Arduino SAMD boards". Make sure to change it for your network and password.

The O2 sensor is not accurate for normal air. The sensitivity is determined by the RGain resistor, so these images are just for illustrative purposes. However, this sensor can detect when it's in a functioning anaerobic chamber.

##### How does it generally work?
The EC410 needs to maintain a potential difference of 600 mV. I implemented this by smoothing the Arduino's Pulse Width Modulation (PWM, 4.7k ohm and 10 uF capacitors) and applying this smoothed signal to the + pins of the OP amp. I have set the virtual ground to be 2.95 V and the 600 mV higer + pin to be 3.55 V. The issue is that arduino's analog output is actually a square wave. The average of this wave might be 2.95 V, but at any moment it is either 0 or 5 V. So I used a capacitor to store a bit of the charge, and a resister to protect the arduino from back voltage. The average voltage should be smoothed and acts an anchor point for the opamp, which can also handle larger currents. 

The EC410 sensor can change a bit, and the potentials are adaptively compensated by the - pins of the OP amp. I implemented this as the schematic that I found at figure 4 of

https://www.sgxsensortech.com/content/uploads/2014/08/A1A-EC_SENSORS_AN2-Design-of-Electronics-for-EC-Sensors-V4.pdf



Once the chip is plugged in and the program ./21.01.04_wifi_oxygen_sensor/21.01.04_wifi_oxygen_sensor.ino has been uploaded to the arduino chip, it will display an ip address. The program needs wifi credentials to connect to a network, the default is set to "guest" network with no password. You can change this on lines 11 and 12 of the program:
```
char ssid[] = "guest";        // your network SSID (name)
char pass[] = "";    // your network password (use for WPA, or use as key for WEP)
```
If you visit the IP address on the LCD display you will see the chip's measurement history. I wrote the table so that it should be pretty easily queried by python's pandas.read_html() method.
![Website](website.png "website")
