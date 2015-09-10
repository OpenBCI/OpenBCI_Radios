# OpenBCI_Radios
Libraries and firmware for OpenBCI Radio Modules

Current release of OpenBCI uses RFduino radio modules
http://www.rfduino.com/product/rfd22301-rfduino-ble-smt/index.html
The RFduino libraries contained here are custom builds for OpenBCI specifically. You must use these libraries to modify or update the Host/Device firmware.

## WE RECOMMEND USING ARDUINO IDE VERSION 1.5.8

After downloading the files in this repo, open the OpenBCI_RFduino_Library file and move the RFduino folder to the following location:

on Mac, right click the Arduino 1.5.x IDE and select Show Contents.
Then place the file in

/Applications/Arduino 154.app/Contents/Resources/Java/hardware/arduino


on Windows
Place the file in 

C\ProgramFiles (x86)\Arduino-1.5.x\hardware\arduino

Install the FTDI drivers for your OS here [FTDI Drivers](www.ftdichip.com/drivers/vcp.htm)

# OpenBCI_Donlge_PassThru
The PassThru code is designed to be used when you want to re-program the Device radio on the OpenBCI board. If you don't have a 3.3V FTDI cable, or friend, you can use the OpenBCI Dongle instead. 
There is a tutorial that describes the process here
http://docs.openbci.com/tutorials/03-Upload_Code_to_OpenBCI_Dongle
