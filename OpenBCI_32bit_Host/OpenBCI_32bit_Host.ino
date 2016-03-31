/*
This sketch is OpenBCI_32bit specific and sets up a HOST module
You need to add cusom files to your application software (sorry)
If you want to modify this firmware, go to RFduino and download their latest release
Install RFduino libraries as instructed from their website
go here (there) to download the adjusted library components,
and follow the instalation instructions to use the custom tools for OpenBC

Since the Device must initiate communication, the
device "polls" the Host evey 50mS when not sending packets.
Host is connected to PC via USB VCP (FTDI).
Device is connectedd to uC (PIC32MX250F128B with UDB32-MX2-DIP).
The code switches between 'normal' mode and 'streamingData' mode.
Normal mode expects a call-response protocol between the Host PC and Device uC.
Normal mode supports avrdude and will allow over-air upload to Device-connected uC.

StreamingData mode expects a continuous Serial stream of data from the uC.
In streamingData mode, Host insterts a pre-fix and post-fix to the data for PC coordination.

Single byte serial messages sent from PC are modified by the Host to include a '+' before and after
This is to avoid an error experienced when the uC gets a 'ghost' command byte during streamData mode

Made by Joel Murphy with Leif Percifield and Conor Russomanno, Summer 2014
Free to use and share. This code presented for use as-is. wysiwyg.

* ASCII commands are received on the serial port to configure and control
* Serial protocol uses '+' immediately before and after the command character
* We call this the 'burger' protocol. the '+' are the buns. Example:
* To begin streaming data, send '+b+'
* This software is provided as-is with no promise of workability
* Use at your own risk, wysiwyg.
*/
#include <RFduinoGZLL.h>
#include "OpenBCI_Radio.h"

void setup() {
  // put your setup code here, to run once:
  OpenBCI_Radio.begin(OPENBCI_MODE_HOST,20);
}

void loop() {
  // put your main code here, to run repeatedly:
    
  if (OpenBCI_Radio.didPCSendDataToHost()) {
    OpenBCI_Radio.getSerialDataFromPCAndPutItInHostsSerialBuffer();
  }
  
  if (OpenBCI_Radio.isTheHostsRadioBufferFilledWithAllThePacketsFromTheDevice()) {
    OpenBCI_Radio.writeTheHostsRadioBufferToThePC();
  }
}
