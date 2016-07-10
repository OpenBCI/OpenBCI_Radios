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

// SHARED
    OpenBCI_Radios_Class();
    boolean     begin(uint8_t);
    boolean     begin(uint8_t, uint32_t);
    boolean     beginDebug(uint8_t, uint32_t);
    boolean     byteIdGetIsStream(char);
    int         byteIdGetPacketNumber(char);
    byte        byteIdGetStreamPacketType(char);
    void        bufferAddStreamPacket(volatile char *, int);
    void        bufferAddTimeSyncSentAck(void);
    void        bufferCleanChar(volatile char *, int);
    void        bufferCleanCompleteBuffer(volatile Buffer *, int);
    void        bufferCleanCompletePacketBuffer(volatile PacketBuffer *, int );
    void        bufferCleanPacketBuffer(volatile PacketBuffer *,int);
    void        bufferCleanBuffer(volatile Buffer *, int);
    void        bufferCleanSerial(int);
    void        bufferCleanStreamPackets(int);
    boolean     bufferRadioAddData(volatile char *, int, boolean);
    void        bufferRadioClean(void);
    void        bufferRadioFlush(void);
    void        bufferRadioReset(void);
    void        bufferResetStreamPacketBuffer(void);
    char        byteIdMake(boolean, int, volatile char *, int);
    byte        byteIdMakeStreamPacketType(void);
    boolean     commsFailureTimeout(void);
    void        configure(uint8_t,uint32_t);
    void        configureDevice(void);
    void        configureHost(void);
    void        configurePassThru(void);
    boolean     didPCSendDataToHost(void);
    boolean     didPicSendDeviceSerialData(void);
    boolean     flashNonVolatileMemory(void);
    uint32_t    getChannelNumber(void);
    uint32_t    getPollTime(void);
    boolean     hasStreamPacket(void);
    boolean     hostPacketToSend(void);
    boolean     isAStreamPacketWaitingForLaunch(void);
    boolean     isATailByteChar(char);
    void        ledFeedBackForPassThru(void);
    boolean     needToSetChannelNumber(void);
    boolean     needToSetPollTime(void);
    byte        outputGetStopByteFromByteId(char);
    void        pollHost(void);
    boolean     pollNow(void);
    boolean     packetToSend(void);
    boolean     packetsInSerialBuffer(void);
    void        pollRefresh(void);
    void        pushRadioBuffer(void);
    void        printBaudRateChangeTo(int);
    void        printChannelNumber(char);
    void        printChannelNumberVerify(void);
    void        printCommsTimeout(void);
    void        printEOT(void);
    void        printFailure(void);
    void        printMessageToDriver(uint8_t);
    void        printPollTime(char);
    void        printSuccess(void);
    void        printValidatedCommsTimeout(void);
    char        processChar(char);
    void        processCommsFailure(void);
    void        processCommsFailureSinglePacket(void);
    boolean     processDeviceRadioCharData(volatile char *, int);
    boolean     processHostRadioCharData(device_t, volatile char *, int);
    byte        processOutboundBuffer(volatile PacketBuffer *);
    byte        processOutboundBufferCharDouble(volatile char *);
    byte        processOutboundBufferCharSingle(char);
    byte        processOutboundBufferCharTriple(volatile char *);
    boolean     processRadioCharDevice(char);
    boolean     processRadioCharHost(device_t, char);
    void        resetPic32(void);
    boolean     revertToDefaultPollTime(void);
    void        revertToPreviousChannelNumber(void);
    void        sendPacketToDevice(volatile device_t);
    void        sendPacketToHost(void);
    void        sendPollMessageToHost(void);
    void        sendRadioMessageToHost(byte);
    void        sendStreamPackets(void);
    boolean     sendStreamPacketToTheHost(void);
    void        sendTheDevicesFirstPacketToTheHost(void);
    boolean     serialWriteTimeOut(void);
    void        setByteIdForPacketBuffer(int);
    boolean     setChannelNumber(uint32_t);
    boolean     setPollTime(uint32_t);
    boolean     storeCharToSerialBuffer(char);
    boolean     thereIsDataInSerialBuffer(void);
    void        writeBufferToSerial(char *,int);
    void        writeTheHostsRadioBufferToThePC(void);

    //////////////////////
    // SHARED VARIABLES //
    //////////////////////
    // CUSTOMS
    BufferRadio bufferRadio;
    Buffer bufferSerial;
    PacketBuffer *currentPacketBufferSerial;
    // BOOLEANS
    boolean debugMode;
    volatile boolean gotAllRadioPackets;
    // CHARS
    char singleCharMsg[1];
    char singlePayLoad[1];

    StreamPacketBuffer streamPacketBuffer;
    volatile boolean isWaitingForNewChannelNumber;
    volatile boolean isWaitingForNewPollTime;
    volatile unsigned long timeOfLastPoll;

    boolean channelNumberSaveAttempted;
    volatile boolean isWaitingForNewChannelNumberConfirmation;
    volatile boolean isWaitingForNewPollTimeConfirmation;
    volatile boolean sendSerialAck;
    volatile boolean processingSendToDevice;
    volatile boolean printMessageToDriverFlag;
    volatile boolean systemUp;
    char ringBuffer[OPENBCI_BUFFER_LENGTH];
    volatile boolean packetInTXRadioBuffer;
    int ringBufferRead;
    volatile int ringBufferWrite;
    volatile int ringBufferNumBytes;


    STREAM_STATE curStreamState;

    uint8_t radioMode;
    volatile uint8_t msgToPrint;
    volatile uint8_t ackCounter;

    unsigned long lastTimeHostHeardFromDevice;
    volatile unsigned long lastTimeSerialRead;


    uint32_t radioChannel;
    uint32_t previousRadioChannel;
    uint32_t pollTime;
};

// Very important, major key to success #christmas
extern OpenBCI_Radios_Class radio;

#endif // OPENBCI_RADIO_H
