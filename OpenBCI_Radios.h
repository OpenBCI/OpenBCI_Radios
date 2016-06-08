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


#ifndef __OpenBCI_Radios__
#define __OpenBCI_Radios__

#include <Arduino.h>
#include <RFduinoGZLL.h>

// needed for enum and callback support
// #include "libRFduinoGZLL.h"
#include "OpenBCI_Radios_Definitions.h"

class OpenBCI_Radios_Class {

public:
    // STRUCTS
    typedef struct {
      char  data[OPENBCI_MAX_PACKET_SIZE_BYTES];
      int   positionRead;
      int   positionWrite;
    } PacketBuffer;

    typedef struct {
        boolean         overflowed;
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

    OpenBCI_Radios_Class();
    boolean     begin(uint8_t mode);
    boolean     begin(uint8_t mode, uint32_t channelNumber);
    boolean     beginDebug(uint8_t mode, uint32_t channelNumber);
    char        byteIdGetCheckSum(char byteId);
    boolean     byteIdGetIsStream(char byteId);
    int         byteIdGetPacketNumber(char byteId);
    byte        byteIdGetStreamPacketType(char byteId);
    void        bufferAddStreamPacket(char *data, int length);
    void        bufferCleanChar(char *buffer, int bufferLength);
    void        bufferCleanCompleteBuffer(Buffer *buffer, int numberOfPacketsToClean);
    void        bufferCleanCompletePacketBuffer(PacketBuffer *packetBuffer, int numberOfPackets);
    void        bufferCleanPacketBuffer(PacketBuffer *packetBuffer,int numberOfPackets);
    void        bufferCleanBuffer(Buffer *buffer, int numberOfPacketsToClean);
    void        bufferCleanRadio(void);
    void        bufferCleanSerial(int numberOfPacketsToClean);
    void        bufferCleanStreamPackets(int numberOfPacketsToClean);
    void        bufferResetStreamPacketBuffer(void);
    // void        bufferSerialFetch(void);
    char        byteIdMake(boolean isStreamPacket, int packetNumber, char *data, int length);
    byte        byteIdMakeStreamPacketType(void);
    char        checkSumMake(char *data, int length);
    boolean     checkSumsAreEqual(char *data, int len);
    void        configure(uint8_t mode,uint32_t channelNumber);
    void        configureDevice(void);
    void        configureHost(void);
    void        configurePassThru(void);
    boolean     didPCSendDataToHost(void);
    boolean     didPicSendDeviceSerialData(void);
    boolean     flashNonVolatileMemory(void);
    uint32_t    getChannelNumber(void);
    // void        getSerialDataFromPCAndPutItInHostsSerialBuffer(void);
    uint32_t    getPollTime(void);
    boolean     hasItBeenTooLongSinceHostHeardFromDevice(void);
    boolean     hasStreamPacket(void);
    boolean     isAStreamPacketWaitingForLaunch(void);
    boolean     isATailByteChar(char newChar);
    void        ledFeedBackForPassThru(void);
    boolean     needToSetChannelNumber(void);
    boolean     needToSetPollTime(void);
    byte        outputGetStopByteFromByteId(char byteId);
    void        pollHost(void);
    boolean     pollNow(void);
    boolean     packetToSend(void);
    void        pollRefresh(void);
    void        pushRadioBuffer(void);
    char        processChar(char newChar);
    // void        processCharForStreamPacket(char newChar);
    boolean     processDeviceRadioCharData(char *data, int len);
    byte        processOutboundBuffer(PacketBuffer *currentPacketBuffer);
    byte        processOutboundBufferCharDouble(char *buffer);
    byte        processOutboundBufferCharSingle(char aChar);
    boolean     processRadioCharDevice(char newChar);
    boolean     processRadioCharHost(device_t device, char newChar);
    boolean     processHostRadioCharData(device_t device, char *data, int len);
    void        resetPic32(void);
    boolean     revertToDefaultPollTime(void);
    void        revertToPreviousChannelNumber(void);
    void        sendPacketToDevice(device_t device);
    void        sendPacketToHost(void);
    void        sendPollMessageToHost(void);
    void        sendRadioMessageToHost(byte msg);
    void        sendStreamPackets(void);
    boolean     sendStreamPacketToTheHost(void);
    void        sendTheDevicesFirstPacketToTheHost(void);
    void        setByteIdForPacketBuffer(int packetNumber);
    boolean     setChannelNumber(uint32_t channelNumber);
    boolean     setPollTime(uint32_t pollTime);
    boolean     storeCharToSerialBuffer(char newChar);
    boolean     thereIsDataInSerialBuffer(void);
    // boolean     theLastTimeNewSerialDataWasAvailableWasLongEnough(void);
    void        writeBufferToSerial(char *buffer,int length);
    void        writeTheHostsRadioBufferToThePC(void);
    void        writeTheHostsStreamPacketBufferToThePC(void);
    void        writeStreamPacket(char *data);

    // VARIABLES
    Buffer  bufferSerial;
    Buffer  bufferStreamPackets;

    StreamPacketBuffer streamPacketBuffer;

    boolean debugMode;
    boolean isDevice;
    boolean isHost;
    boolean gotAllRadioPackets;
    boolean isWaitingForNewChannelNumber;
    boolean isWaitingForNewChannelNumberConfirmation;
    boolean isWaitingForNewPollTime;
    boolean verbosePrintouts;

    char    bufferRadio[OPENBCI_BUFFER_LENGTH];
    char    singleCharMsg[1];

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
    unsigned long lastTimeSerialRead;
    unsigned long timeWeGot0xFXFromPic;
    unsigned long timeOfLastPoll;
    unsigned long debugT1;
    unsigned long debugT2;
    unsigned long debugT3;

    uint32_t radioChannel;
    uint32_t previousRadioChannel;
    uint32_t pollTime;
};

// Very important, major key to success
extern OpenBCI_Radios_Class radio;

#endif // OPENBCI_RADIO_H
