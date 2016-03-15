/**
* Name: OpenBCI_Radio.h
* Date: 3/15/2016
* Purpose: This is the header file for the OpenBCI radios. Let us define two
*   over arching paradigms: Host and Device, where:
*     Host is connected to PC via USB VCP (FTDI).
*     Device is connectedd to uC (PIC32MX250F128B with UDB32-MX2-DIP).
*
* Author: Push The World LLC (AJ Keller)
*   Much credit must also go to Joel Murphy who with Conor Russomanno and Leif
*     Percifield created the original OpenBCI_32bit_Device.ino and
*     OpenBCI_32bit_Host.ino files in the Summer of 2014. Much of this code base
*     is inspired directly from their work.
*/


#ifndef __OpenBCI_Radio__
#define __OpenBCI_Radio__

#define PROD_MODE false

#if PROD_MODE
	#include <WProgram.h>
	#include <RFduinoGZLL.h>
#endif
#include "OpenBCI_Radio_Definitions.h"

class OpenBCI_Radio {
public:
  OpenBCI_Radio();
  boolean begin(uint8_t mode = OPENBCI_MODE_DEVICE,int8_t channelNumber);
private:
  // METHODS
  void configureDevice(void);
  void configureHost(void);
  void configurePassThru(void);
  void bufferInit(int *buffer);
  void initialize(uint8_t mode,int8_t channelNumber);

  // VARIABLES
  int8_t PGCpin = 5; // This is not used
  int *mainBuffer;
}
#endif // OPENBCI_RADIO_H
