/**
* Name: OpenBCI_Radio_Definitions.h
* Date: 3/15/2016
* Purpose: This is the header file for the OpenBCI radios definitions.
*
* Author: Push The World LLC (AJ Keller)
*   Much credit must also go to Joel Murphy who with Conor Russomanno and Leif
*     Percifield created the original OpenBCI_32bit_Device.ino and
*     OpenBCI_32bit_Host.ino files in the Summer of 2014. Much of this code base
*     is inspired directly from their work.
*/

#ifndef __OpenBCI_Radio_Definitions__
#define __OpenBCI_Radio_Definitions__

// These are helpful maximums to reference nad use in the code
#define OPENBCI_MAX_PACKET_SIZE_BYTES 32
#define OPENBCI_MAX_NUMBER_OF_BUFFERS 20

// Max buffer lengths
#define OPENBCI_BUFFER_LENGTH 512

// These are the three different possible configuration modes for this library
#define OPENBCI_MODE_DEVICE 0
#define OPENBCI_MODE_HOST 1
#define OPENBCI_MODE_PASS_THRU 2

// Pins used by the Device
#define OPENBCI_PIN_DEVICE_PCG 5
// Pins used by the Host
#define OPENBCI_PIN_HOST_LED 2
#define OPENBCI_PIN_HOST_RESET 6

// roles for the RFduinoGZLL
#define RFDUINOGZLL_ROLE_HOST HOST
#define RFDUINOGZLL_ROLE_DEVICE DEVICE0

#endif
