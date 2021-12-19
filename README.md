# MHI-AC-Trace
Trace of SPI signals for Mitsubishi Air Conditioner

Publish the data from the SPI signals MOSI and MISO via MQTT and output to the debug console.
Use this program if you intend to evaluate / debug the SPI data of your air conditioner. It is not needed for the user of [MHI-AC-Ctrl](https://github.com/absalom-muc/MHI-AC-Ctrl).

For understanding of the monitored data [SPI.md](SPI.md) is a good starting point.

# Attention:
:warning: You have to open the indoor unit to have access to the SPI. Opening of the indoor unit should be done by 
a qualified professional because faulty handling may cause leakage of water, electric shock or fire! :warning: 

# Prerequisites:
You need a [MHI-AC-Ctrl](https://github.com/absalom-muc/MHI-AC-Ctrl) PCB for using MHI-AC-Trace. For MHI-AC-Trace MOSI and MISO (and SCK of course) are inputs, both data streams are monitored.

# Installing:

## Hardware:
Identical to MHI-AC-Ctrl PCB, no adaption of the MHI-AC-Ctrl PCB is required. If you want to connect more than one MHI-AC-Ctrl PCBs it might be easier using an older PCB version because the latest PCB version has only one SPI connector. Check [MHI-AC-Ctrl Readme](https://github.com/absalom-muc/MHI-AC-Ctrl#prerequisites) for a list of supported devices.

## Software:
The program uses the following libraries
 - [MQTT client library](https://github.com/knolleary/pubsubclient) :warning: please don't use v2.8.0! (because of this [issue](https://github.com/knolleary/pubsubclient/issues/747)). Better use v2.7.0:warning:
 - [ArduinoOTA](https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA)
 
Please check the GitHub pages to see how to install them (usually via tools -> libraries).
Create a sub-directory "MHI-AC-Trace" and copy the files from the src directory in your MHI-AC-Trace sub-directory. Finally enter WiFi and MQTT credentials in [MHI-AC-Trace.h](src/MHI-AC-Trace.h). Please also consider the option 'MQTT_CHAR'.

# Disclaimer
This SW is experimental and you use it on your own risk.

# Output
The data is output on the serial debug console and on MQTT. There is an offset of one MISO frame implemented. That means you see in one line
- MISO data from t
- MOSI data from t+1  

With this offset you see in a line together the requesting MISO and the answering MOSI frame. This simplifies the analysis.

## Serial Console output
The output on the serial debug (baud rate 115200, switch off the time stamp) has the following human readable format:
![grafik](https://user-images.githubusercontent.com/23119513/146646269-7d4b1d84-a936-4010-a62e-8363e1d14d9a.png)
All numbers in hexadecimal format.

## MQTT output
The data is output via topic "raw" in ascii char format. Overall 43 chars per line. Order is identical to serial console output:
- Frame counter number (high byte)
- Frame counter number (low byte)
- MOSI SB0-SB2
- MOSI DB0-DB14
- MOSI CBH, CBL
- MISO SB0-SB2
- MISO DB0-DB14
- MISO CBH, CBL
- Repetition No

Frame counter number is a consecutive number. In order to reduce workload a frame is published only once also if it was received multiple times. The "Repetition No" represents the number of repetitions of the **previous** line.

MQTT topic "Error1" is updated with the received MOSI signature if it isn't the expected signature.
MQTT topic "Error2" is updated with the received MOSI checksum if it isn't the expected checksum.
No check of MISO signature / checksum.

# License
This project is licensed under the MIT License - see LICENSE.md for details

# Acknowledgments
The coding of the [SPI protocol](SPI.md) of the AC is a nightmare. Without [rjdekker's MHI2MQTT](https://github.com/rjdekker/MHI2MQTT) I had no chance to understand the protocol! Unfortunately rjdekker is no longer active on GitHub. Also thank you very much to the authors and contributors of [MQTT client](https://github.com/knolleary/pubsubclient) and [ArduinoOTA](https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA) libraries.
