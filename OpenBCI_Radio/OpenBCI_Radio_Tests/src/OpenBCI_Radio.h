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

#include <WProgram.h>
#include <RFduinoGZLL.h>
#include "OpenBCI_Radio_Definitions.h"

class OpenBCI_Radio {
public:
  OpenBCI_Radio();
  boolean begin(uint8_t mode = OPENBCI_MODE_DEVICE,int8_t channelNumber);
  boolean readRadio(void);
  boolean readSerial(void);
  void writeRadio(void);
  void writeSerial(void);
private:
  // STRUCTS
  typedef struct {
    char  data[OPENBCI_MAX_PACKET_SIZE_BYTES];
    int   positionRead;
    int   positionWrite;
  } PacketBuffer;

  typedef struct {
    int           numberOfPacketsToSend;
    int           numberOfPacketsSent;
    PacketBuffer  packetBuffer[OPENBCI_MAX_NUMBER_OF_BUFFERS];
  } Buffer;

  // METHODS
  void bufferCleanChar(char *buffer, int bufferLength);
  void bufferCleanPacketBuffer(PacketBuffer *packetBuffer,int numberOfPackets);
  void bufferCleanBuffer(Buffer *buffer);
  void bufferCleanRadio(void);
  void bufferCleanSerial(void);
  void configure(uint8_t mode,int8_t channelNumber);
  void configureDevice(void);
  void configureHost(void);
  void configurePassThru(void);
  boolean readSerialDevice(void);
  boolean readSerialHost(void);
  void writeSerialDevice(void);
  void writeSerialHost(void);

  // VARIABLES
  char bufferRadio[OPENBCI_BUFFER_LENGTH];
  int bufferPacketsReceived;
  int bufferPacketsToReceive;
  int bufferPositionReadRadio;
  int bufferPositionWriteRadio;

  Buffer bufferSerial;
  PacketBuffer *currentPacketBufferSerial;


  unsigned long timeOfLastPoll;
  unsigned long timeOfLastSerialRead;

  uint8_t radioMode;


}
#endif // OPENBCI_RADIO_H
