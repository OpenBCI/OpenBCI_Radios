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

#include <Arduino.h>
#include <RFduinoGZLL.h>

// needed for enum and callback support
// #include "libRFduinoGZLL.h"
#include "OpenBCI_Radio_Definitions.h"

class OpenBCI_Radio_Class {

public:
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

    OpenBCI_Radio_Class();
    boolean begin(uint8_t mode,int8_t channelNumber);
    int     byteIdGetPacketNumber(char byteId);
    char    byteIdGetCheckSum(char byteId);
    boolean byteIdGetIsStream(char byteId);
    char    checkSumMake(char *data, int length);
    void    pollHost(void);
    void    pollRefresh(void);
    // boolean readRadio(void);
    // boolean readSerial(void);
    // void    writeRadio(void);
    // void    writeSerial(void);
    void    writeStreamPacket(char *data);
    void    bufferCleanChar(char *buffer, int bufferLength);
    void    bufferCleanPacketBuffer(PacketBuffer *packetBuffer,int numberOfPackets);
    void    bufferCleanBuffer(Buffer *buffer);
    void    bufferCleanRadio(void);
    void    bufferCleanSerial(void);
    void    bufferSerialFetch(void);
    char    byteIdMake(boolean isStreamPacket, int packetNumber, char *data, int length);
    boolean checkSumsAreEqual(char *data, int len);
    void    configure(uint8_t mode,int8_t channelNumber);
    void    configureDevice(void);
    void    configureHost(void);
    void    configurePassThru(void);
    boolean pollNow(void);


    void    writeBufferToSerial(char *buffer,int length);

    boolean didPCSendDataToHost(void);
    boolean didPicSendDeviceSerialData(void);
    boolean thereIsDataInSerialBuffer(void);
    boolean theLastTimeNewSerialDataWasAvailableWasLongEnough(void);
    boolean hasItBeenTooLongSinceHostHeardFromDevice(void);
    void    getSerialDataFromPCAndPutItInHostsSerialBuffer(void);
    void    getSerialDataFromPicAndPutItInTheDevicesSerialBuffer(void);
    void    sendTheDevicesFirstPacketToTheHost(void);
    void    writeTheDevicesRadioBufferToThePic(void);
    void    writeTheHostsRadioBufferToThePC(void);

    // VARIABLES
    Buffer  bufferSerial;
    boolean isTheDevicesRadioBufferFilledWithAllThePacketsFromTheHost;
    boolean isTheHostsRadioBufferFilledWithAllThePacketsFromTheDevice;
    char    bufferRadio[OPENBCI_BUFFER_LENGTH];
    int     bufferPacketsReceived;
    int     bufferPacketsToReceive;
    int     bufferPositionReadRadio;
    int     bufferPositionWriteRadio;
    int     previousPacketNumber;
    PacketBuffer *currentPacketBufferSerial;
    uint8_t radioMode;
    boolean isHost;
    boolean isDevice;
    unsigned long lastTimeNewSerialDataWasAvailable;
    unsigned long lastTimeHostHeardFromDevice;
    char *loremIpsum;
    boolean verbosePrintouts;

    // METHODS
    // void _RFduinoGZLL_onReceive(device_t device, int rssi, char *data, int len);


    // VARIABLES
    unsigned long timeOfLastPoll;

    int8_t radioChannel;
};

// Very important, major key to success
extern OpenBCI_Radio_Class OpenBCI_Radio;

#endif // OPENBCI_RADIO_H
