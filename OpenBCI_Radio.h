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
        int             numberOfPacketsToSend;
        int             numberOfPacketsSent;
        PacketBuffer    packetBuffer[OPENBCI_MAX_NUMBER_OF_BUFFERS];
    } Buffer;

    typedef struct {
        boolean     readyForLaunch;
        boolean     gotHead;
        char        data[OPENBCI_MAX_PACKET_SIZE_BYTES];
        char        typeByte;
        int         bytesIn;
    } StreamPacketBuffer;

    OpenBCI_Radio_Class();
    boolean begin(uint8_t mode);
    boolean begin(uint8_t mode,int8_t channelNumber);
    char    byteIdGetCheckSum(char byteId);
    boolean byteIdGetIsStream(char byteId);
    int     byteIdGetPacketNumber(char byteId);
    byte    byteIdGetStreamPacketType(char byteId);
    char    checkSumMake(char *data, int length);
    void    bufferAddStreamPacket(char *data, int length);
    void    bufferCleanChar(char *buffer, int bufferLength);
    void    bufferCleanCompleteBuffer(Buffer *buffer, int numberOfPacketsToClean);
    void    bufferCleanCompletePacketBuffer(PacketBuffer *packetBuffer, int numberOfPackets);
    void    bufferCleanPacketBuffer(PacketBuffer *packetBuffer,int numberOfPackets);
    void    bufferCleanBuffer(Buffer *buffer, int numberOfPacketsToClean);
    void    bufferCleanRadio(void);
    void    bufferCleanSerial(int numberOfPacketsToClean);
    void    bufferCleanStreamPackets(int numberOfPacketsToClean);
    void    bufferResetStreamPacketBuffer(void);
    void    bufferSerialFetch(void);
    char    byteIdMake(boolean isStreamPacket, int packetNumber, char *data, int length);
    byte    byteIdMakeStreamPacketType(void);
    boolean checkSumsAreEqual(char *data, int len);
    void    configure(uint8_t mode,int8_t channelNumber);
    void    configureDevice(void);
    void    configureHost(void);
    void    configurePassThru(void);
    boolean didPCSendDataToHost(void);
    boolean didPicSendDeviceSerialData(void);
    boolean doesTheHostHaveAStreamPacketToSendToPC(void);
    void    getSerialDataFromPCAndPutItInHostsSerialBuffer(void);
    void    getSerialDataFromPicAndPutItInTheDevicesSerialBuffer(void);
    boolean hasEnoughTimePassedToLaunchStreamPacket(void);
    boolean hasItBeenTooLongSinceHostHeardFromDevice(void);
    boolean isAStreamPacketWaitingForLaunch(void);
    void    ledFeedBackForPassThru(void);
    byte    outputGetStopByteFromByteId(char byteId);
    void    pollHost(void);
    boolean pollNow(void);
    void    pollRefresh(void);
    void    processCharForStreamPacket(char newChar);
    void    sendTheDevicesFirstPacketToTheHost(void);
    void    sendStreamPacketToTheHost(void);
    boolean thereIsDataInSerialBuffer(void);
    boolean theLastTimeNewSerialDataWasAvailableWasLongEnough(void);
    void    writeBufferToSerial(char *buffer,int length);
    void    writeTheDevicesRadioBufferToThePic(void);
    void    writeTheHostsRadioBufferToThePC(void);
    void    writeTheHostsStreamPacketBufferToThePC(void);
    void    writeStreamPacket(char *data);

    // VARIABLES
    Buffer  bufferSerial;
    Buffer  bufferStreamPackets;

    StreamPacketBuffer streamPacketBuffer;

    boolean debugMode;
    boolean isDevice;
    boolean isHost;
    boolean isTheDevicesRadioBufferFilledWithAllThePacketsFromTheHost;
    boolean isTheHostsRadioBufferFilledWithAllThePacketsFromTheDevice;
    boolean verbosePrintouts;

    char    bufferRadio[OPENBCI_BUFFER_LENGTH];

    int     bufferPacketsReceived;
    int     bufferPacketsToReceive;
    int     bufferPositionReadRadio;
    int     bufferPositionWriteRadio;
    int     previousPacketNumber;

    PacketBuffer *currentPacketBufferSerial;
    PacketBuffer *currentPacketBufferStreamPacket;

    uint8_t radioMode;

    unsigned long lastTimeNewSerialDataWasAvailable;
    unsigned long lastTimeHostHeardFromDevice;
    unsigned long timeWeGot0xFXFromPic;
    unsigned long timeOfLastPoll;

    int8_t radioChannel;
};

// Very important, major key to success
extern OpenBCI_Radio_Class OpenBCI_Radio;

#endif // OPENBCI_RADIO_H
