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
    // ENUMS
    typedef enum STREAM_STATE {
        STREAM_STATE_INIT,
        STREAM_STATE_STORING,
        STREAM_STATE_READY,
        STREAM_STATE_TAIL
    };
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
        char        typeByte;
        char        data[OPENBCI_MAX_PACKET_SIZE_BYTES];
        int         bytesIn;
    } StreamPacketBuffer;

    typedef struct {
        char    data[OPENBCI_BUFFER_LENGTH];
        int     positionWrite;
        int     previousPacketNumber;
    } BufferRadio;

    OpenBCI_Radios_Class();
    boolean     begin(uint8_t mode);
    boolean     begin(uint8_t mode, uint32_t channelNumber);
    boolean     beginDebug(uint8_t mode, uint32_t channelNumber);
    char        byteIdGetCheckSum(char byteId);
    boolean     byteIdGetIsStream(char byteId);
    int         byteIdGetPacketNumber(char byteId);
    byte        byteIdGetStreamPacketType(char byteId);
    void        bufferAddStreamPacket(volatile char *data, int length);
    void        bufferCleanChar(volatile char *buffer, int bufferLength);
    void        bufferCleanCompleteBuffer(volatile Buffer *buffer, int numberOfPacketsToClean);
    void        bufferCleanCompletePacketBuffer(volatile PacketBuffer *packetBuffer, int numberOfPackets);
    void        bufferCleanPacketBuffer(volatile PacketBuffer *packetBuffer,int numberOfPackets);
    void        bufferCleanBuffer(volatile Buffer *buffer, int numberOfPacketsToClean);
    void        bufferCleanSerial(int numberOfPacketsToClean);
    void        bufferCleanStreamPackets(int numberOfPacketsToClean);
    boolean     bufferRadioAddData(volatile char *data, int len, boolean clearBuffer);
    void        bufferRadioClean(void);
    void        bufferRadioFlush(void);
    void        bufferRadioReset(void);
    void        bufferResetStreamPacketBuffer(void);
    // void        bufferSerialFetch(void);
    char        byteIdMake(boolean isStreamPacket, int packetNumber, volatile char *data, int length);
    byte        byteIdMakeStreamPacketType(void);
    char        checkSumMake(volatile char *data, int length);
    boolean     checkSumsAreEqual(volatile char *data, int len);
    boolean     commsFailureTimeout(void);
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
    boolean     hasStreamPacket(void);
    boolean     hostPacketToSend(void);
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
    void        printChannelNumber(char);
    void        printCommsTimeout(void);
    void        printEOT(void);
    void        printFailure(void);
    void        printPollTime(char);
    void        printSuccess(void);
    void        printValidatedCommsTimeout(void);
    char        processChar(char newChar);
    void        processCommsFailure(void);
    void        processCommsFailureSinglePacket(void);
    boolean     processDeviceRadioCharData(volatile char *data, int len);
    boolean     processHostRadioCharData(device_t device, volatile char *data, int len);
    byte        processOutboundBuffer(volatile PacketBuffer *currentPacketBuffer);
    byte        processOutboundBufferCharDouble(volatile char *buffer);
    byte        processOutboundBufferCharSingle(char aChar);
    boolean     processRadioCharDevice(char newChar);
    boolean     processRadioCharHost(device_t device, char newChar);
    void        resetPic32(void);
    boolean     revertToDefaultPollTime(void);
    void        revertToPreviousChannelNumber(void);
    void        sendPacketToDevice(volatile device_t device);
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
    void        writeBufferToSerial(char *buffer,int length);
    void        writeTheHostsRadioBufferToThePC(void);
    void        writeTheHostsStreamPacketBufferToThePC(void);
    void        writeStreamPacket(volatile char *data);

    // VARIABLES
    volatile StreamPacketBuffer streamPacketBuffer;
    volatile BufferRadio bufferRadio;
    volatile Buffer bufferSerial;
    volatile Buffer bufferStreamPackets;

    boolean debugMode;
    boolean isDevice;
    boolean isHost;
    volatile boolean gotAllRadioPackets;
    volatile boolean isWaitingForNewChannelNumber;
    volatile boolean isWaitingForNewChannelNumberConfirmation;
    volatile boolean isWaitingForNewPollTime;
    volatile boolean isWaitingForNewPollTimeConfirmation;
    boolean verbosePrintouts;

    char    singleCharMsg[1];

    volatile boolean packetInTXRadioBuffer;
    volatile device_t deviceRadio;

    volatile int lastPacketSent;

    volatile PacketBuffer *currentPacketBufferSerial;
    volatile PacketBuffer *currentPacketBufferStreamPacket;

    STREAM_STATE curStreamState;

    uint8_t radioMode;

    unsigned long lastTimeNewSerialDataWasAvailable;
    unsigned long lastTimeHostHeardFromDevice;
    volatile unsigned long lastTimeSerialRead;
    unsigned long timeWeGot0xFXFromPic;
    volatile unsigned long timeOfLastPoll;

    uint32_t radioChannel;
    uint32_t previousRadioChannel;
    uint32_t pollTime;
};

// Very important, major key to success
extern OpenBCI_Radios_Class radio;

#endif // OPENBCI_RADIO_H
