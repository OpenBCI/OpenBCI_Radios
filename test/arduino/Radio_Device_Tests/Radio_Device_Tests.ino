#include <RFduinoGZLL.h>
#include "OpenBCI_Radios.h"
#include "PTW-Arduino-Assert.h"

int ledPin = 2;

void setup() {
    // Get's serial up and running
    pinMode(ledPin,OUTPUT);
    Serial.begin(115200);
    test.setSerial(Serial);
    test.failVerbosity = true;
}

void loop() {
    // Start tests by just sending a command
    if (Serial.available()) {
        Serial.read();
        go();
    }
}

void go() {
    // Start the test
    test.begin();
    digitalWrite(ledPin, HIGH);

    testProcessChar();
    testbufferStreamAddChar();
    testProcessRadioChar();
    testByteIdMakeStreamPacketType();

    digitalWrite(ledPin, LOW);
    test.end();
}

/********************************************/
/********************************************/
/*********    Process Char Tests    *********/
/********************************************/
/********************************************/

void testbufferStreamAddChar() {
    test.describe("bufferStreamAddChar");

    testbufferStreamAddChar_STREAM_STATE_INIT();
    testbufferStreamAddChar_STREAM_STATE_TAIL();
    testbufferStreamAddChar_STREAM_STATE_STORING();
    testbufferStreamAddChar_STREAM_STATE_READY();

}

void testbufferStreamAddChar_STREAM_STATE_INIT() {
    test.detail("STREAM_STATE_INIT");

    test.it("should recognize the start byte and change state to storing");
    radio.bufferStreamReset(radio.streamPacketBuffer);
    char newChar = '0';
    radio.bufferStreamAddChar(radio.streamPacketBuffer, newChar);
    test.assertEqualByte(radio.streamPacketBuffer->state,radio.STREAM_STATE_STORING, "should enter state storing", __LINE__);
    test.assertEqualChar(radio.streamPacketBuffer->data[0],newChar,"should have stored the new char", __LINE__);
    test.assertEqualByte(radio.streamPacketBuffer->bytesIn,1,"should have read one byte in",__LINE__);
}

void testbufferStreamAddChar_STREAM_STATE_TAIL() {
    test.detail("STREAM_STATE_TAIL");

    char newChar = 'A';

    test.it("should set the typeByte and set state to ready");
    newChar = (char)OPENBCI_STREAM_PACKET_TAIL;
    radio.bufferStreamReset(radio.streamPacketBuffer);
    radio.streamPacketBuffer->bytesIn = OPENBCI_MAX_PACKET_SIZE_BYTES;
    radio.streamPacketBuffer->state = radio.STREAM_STATE_TAIL;
    radio.bufferStreamAddChar(radio.streamPacketBuffer, newChar);
    test.assertEqualByte(radio.streamPacketBuffer->typeByte, newChar, "should set the type byte to the stop byte", __LINE__);
    test.assertEqualByte(radio.streamPacketBuffer->state, radio.STREAM_STATE_READY, "should be in the ready state", __LINE__);

    test.it("should set state to init if byte is not stop byte or head byte");
    newChar = (char)0x00;
    radio.bufferStreamReset(radio.streamPacketBuffer);
    radio.streamPacketBuffer->bytesIn = OPENBCI_MAX_PACKET_SIZE_BYTES;
    radio.streamPacketBuffer->state = radio.STREAM_STATE_TAIL;
    radio.bufferStreamAddChar(radio.streamPacketBuffer, newChar);
    test.assertEqualByte(radio.streamPacketBuffer->state, radio.STREAM_STATE_INIT, "should be in the ready state", __LINE__);
    test.assertEqualByte(radio.streamPacketBuffer->bytesIn, 0, "should set bytesIn back to 0", __LINE__);

    test.it("should set the state to storing if the byte is a head byte");
    newChar = (char)OPENBCI_STREAM_PACKET_HEAD;
    radio.bufferStreamReset(radio.streamPacketBuffer);
    radio.streamPacketBuffer->bytesIn = OPENBCI_MAX_PACKET_SIZE_BYTES;
    radio.streamPacketBuffer->state = radio.STREAM_STATE_TAIL;
    radio.bufferStreamAddChar(radio.streamPacketBuffer, newChar);
    test.assertEqualByte(radio.streamPacketBuffer->state,radio.STREAM_STATE_STORING, "should enter state storing", __LINE__);
    test.assertEqualChar(radio.streamPacketBuffer->data[0],newChar,"should have stored the new char", __LINE__);
    test.assertEqualByte(radio.streamPacketBuffer->bytesIn,1,"should have read one byte in",__LINE__);

}

void testbufferStreamAddChar_STREAM_STATE_STORING() {
    test.detail("STREAM_STATE_STORING");

    char newChar = 'A';
    uint8_t initialBytesIn = 5;

    test.it("should store the new char to the buffer in position of bytesIn and increment byteIn by 1");
    initialBytesIn = 5;
    radio.bufferStreamReset(radio.streamPacketBuffer);
    radio.streamPacketBuffer->bytesIn = initialBytesIn;
    radio.streamPacketBuffer->state = radio.STREAM_STATE_TAIL;
    radio.bufferStreamAddChar(radio.streamPacketBuffer, newChar);
    test.assertEqualByte(radio.streamPacketBuffer->state,radio.STREAM_STATE_STORING, "should remain in state storing", __LINE__);
    test.assertEqualChar(radio.streamPacketBuffer->data[initialBytesIn],newChar,"should have stored the new char to inital bytesIn position", __LINE__);
    test.assertEqualByte(radio.streamPacketBuffer->bytesIn,initialBytesIn + 1,"should have incremented bytes in by 1",__LINE__);

    test.it("should change state to tail if 32 bytes have been read in");
    initialBytesIn = 31;
    radio.bufferStreamReset(radio.streamPacketBuffer);
    radio.streamPacketBuffer->bytesIn = initialBytesIn;
    radio.streamPacketBuffer->state = radio.STREAM_STATE_TAIL;
    radio.bufferStreamAddChar(radio.streamPacketBuffer, newChar);
    test.assertEqualByte(radio.streamPacketBuffer->state,radio.STREAM_STATE_TAIL, "should change state to tail", __LINE__);
    test.assertEqualChar(radio.streamPacketBuffer->data[initialBytesIn],newChar,"should have stored the new char to inital bytesIn position", __LINE__);
    test.assertEqualByte(radio.streamPacketBuffer->bytesIn,initialBytesIn + 1,"should have incremented bytes in by 32",__LINE__);

}

void testbufferStreamAddChar_STREAM_STATE_READY() {
    test.it("should change to init state and set bytesIn to 0");
    radio.bufferStreamReset(radio.streamPacketBuffer);
    radio.streamPacketBuffer->state = radio.STREAM_STATE_READY;
    radio.streamPacketBuffer->bytesIn = 32;
    radio.bufferStreamAddChar(radio.streamPacketBuffer,(char)0x00);
    test.assertEqualByte(radio.streamPacketBuffer->state, radio.STREAM_STATE_INIT, "should reset to init state",__LINE__);
}

void testProcessChar() {
    testIsATailByte();
    testProcessCharSingleChar();
    testProcessCharStreamPacket();
    testProcessCharStreamPackets();
    testProcessCharNotStreamPacket();
    testProcessCharOverflow();
}

void testIsATailByte() {
    test.describe("isATailByte");

    test.assertBoolean(radio.isATailByte(0xC0),true,"Stream packet type 0",__LINE__);
    test.assertBoolean(radio.isATailByte(0xC1),true,"Stream packet type 1",__LINE__);
    test.assertBoolean(radio.isATailByte(0xC8),true,"Stream packet type 8",__LINE__);
    test.assertBoolean(radio.isATailByte(0xCA),true,"Stream packet type 10",__LINE__);
    test.assertBoolean(radio.isATailByte(0xCF),true,"Stream packet type 15",__LINE__);
    test.assertBoolean(radio.isATailByte(0xB0),false,"Not a stream packet type",__LINE__);

    // Remember to clean up after yourself
    testProcessChar_CleanUp();
}

void testProcessCharSingleChar() {
    test.describe("processCharForSingleChar");

    // Clear the buffers
    radio.bufferSerialReset(OPENBCI_MAX_NUMBER_OF_BUFFERS);
    radio.bufferStreamReset(radio.streamPacketBuffer);

    // try to add a char
    char input = 'A';
    // Store it to serial buffer
    radio.bufferSerialAddChar(input);
    // Get one char and process it
    radio.bufferStreamAddChar(radio.streamPacketBuffer,input);

    // Verify serial buffer
    test.assertEqualChar(radio.bufferSerial.packetBuffer->data[radio.bufferSerial.packetBuffer->positionWrite - 1],input,"Char stored to serial buffer",__LINE__);
    test.assertEqualChar(radio.bufferSerial.numberOfPacketsToSend,1,"Serial buffer has 1 packet to send",__LINE__);
    test.assertEqualChar(radio.bufferSerial.numberOfPacketsSent,0,"Serial buffer not sent any packets",__LINE__);
    // Verify stream packet buffer
    test.assertEqualChar(radio.streamPacketBuffer->data[0],input,"Char stored to stream packet buffer",__LINE__);

    // Remember to clean up after yourself
    testProcessChar_CleanUp();

}

void testProcessCharStreamPacket() {
    test.describe("processCharForStreamPacket");

    // Clear the buffers
    radio.bufferSerialReset(OPENBCI_MAX_NUMBER_OF_BUFFERS);
    radio.bufferStreamReset(radio.streamPacketBuffer);

    // Write a stream packet with end byte 0xA0
    writeAStreamPacketToProcessChar(0xC0);

    // Right away we want to see if enough time has passed, this should be false
    //  because we just processed a char, after this test is complete, we should
    //  be far passed 90uS
    boolean notEnoughTimePassed = micros() > (radio.lastTimeSerialRead + OPENBCI_TIMEOUT_PACKET_STREAM_uS);
    test.assertBoolean(notEnoughTimePassed,false,"Type 0 waiting...",__LINE__);

    // Do we have a stream packet waiting to launch?
    test.assertBoolean(radio.isAStreamPacketWaitingForLaunch(),true,"Type 0 loaded",__LINE__);

    // This should return true this time
    boolean enoughTimePassed = micros() > (radio.lastTimeSerialRead + OPENBCI_TIMEOUT_PACKET_STREAM_uS);
    test.assertBoolean(enoughTimePassed,true,"Type 0 ready",__LINE__);

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // Try a stream packet with another stop byte /////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // Clear the buffers
    radio.bufferSerialReset(OPENBCI_MAX_NUMBER_OF_BUFFERS);
    radio.bufferStreamReset(radio.streamPacketBuffer);

    // Write a stream packet with end byte not 0xA0
    writeAStreamPacketToProcessChar(0xC5);

    // Right away we want to see if enough time has passed, this should be false
    //  because we just processed a char, after this test is complete, we should
    //  be far passed 90uS
    notEnoughTimePassed = micros() > (radio.lastTimeSerialRead + OPENBCI_TIMEOUT_PACKET_STREAM_uS);
    test.assertBoolean(notEnoughTimePassed,false,"Type 5 waiting",__LINE__);

    // Do we have a stream packet waiting to launch?
    test.assertBoolean(radio.isAStreamPacketWaitingForLaunch(),true,"Type 5 loaded",__LINE__);

    // This should return true this time
    enoughTimePassed = micros() > (radio.lastTimeSerialRead + OPENBCI_TIMEOUT_PACKET_STREAM_uS);
    test.assertBoolean(enoughTimePassed,true,"Type 5 ready",__LINE__);

    // Remember to clean up after yourself
    testProcessChar_CleanUp();

}

// Send stream packets, one after the other
void testProcessCharStreamPackets() {
    test.describe("processCharForStreamPackets");

    // Clear the buffers
    radio.bufferSerialReset(OPENBCI_MAX_NUMBER_OF_BUFFERS);
    radio.bufferStreamReset(radio.streamPacketBuffer);

    int numberOfTrials = 9;
    char testMessage[] = "Sent 0";
    unsigned long t1, t2;

    for (int i = 0; i < numberOfTrials; i++) {
        // Write a stream packet with end byte 0xC0
        writeAStreamPacketToProcessChar(0xC0);
        // Stream packet should be waiting
        test.assertBoolean(radio.isAStreamPacketWaitingForLaunch(),true);
        // Save the current time;
        t1 = micros();
        // Wait
        while(micros() < (radio.lastTimeSerialRead + OPENBCI_TIMEOUT_PACKET_STREAM_uS)) {};
        // configure the test message
        testMessage[5] = (i + 1) + '0';
        // Send the stream packet
        test.assertBoolean(radio.bufferStreamSendToHost(radio.streamPacketBuffer),true,testMessage);
    }
}

// Test conditions that result in a stream packet not being launched
void testProcessCharNotStreamPacket() {
    test.describe("processCharForNotStreamPacket");

    // Clear the buffers
    radio.bufferSerialReset(OPENBCI_MAX_NUMBER_OF_BUFFERS);
    radio.bufferStreamReset(radio.streamPacketBuffer);

    // Write a stream packet
    writeAStreamPacketToProcessChar(0xC0);
    // Fake Serial read
    char newChar = (char)0xFF;
    // Save current time as the last serial read
    radio.lastTimeSerialRead = micros();
    // Quick! Write another char
    radio.bufferStreamAddChar(radio.streamPacketBuffer, newChar);

    test.assertBoolean(radio.isAStreamPacketWaitingForLaunch(),false,"Too many packets in",__LINE__);
    test.assertEqualInt(radio.streamPacketBuffer->bytesIn,0,"0 bytes in",__LINE__);

    // Clear the buffers
    radio.bufferSerialReset(OPENBCI_MAX_NUMBER_OF_BUFFERS);
    radio.bufferStreamReset(radio.streamPacketBuffer);

    // Write a stream packet with a bad end byte
    writeAStreamPacketToProcessChar(0xB5);
    test.assertBoolean(radio.isAStreamPacketWaitingForLaunch(),false,"Bad end byte",__LINE__);


    // Remember to clean up after yourself
    testProcessChar_CleanUp();
}

// Put the system in an overflow condition
void testProcessCharOverflow() {
    test.describe("testProcessCharOverflow");

    // Clear the buffers
    radio.bufferSerialReset(OPENBCI_MAX_NUMBER_OF_BUFFERS);
    radio.bufferStreamReset(radio.streamPacketBuffer);

    // Write the max number of bytes in buffers
    int maxBytes = OPENBCI_MAX_NUMBER_OF_BUFFERS * OPENBCI_MAX_DATA_BYTES_IN_PACKET;
    // Write max bytes but stop 1 before
    for (int i = 0; i < maxBytes; i++) {
        radio.bufferSerialAddChar(0x00);
    }

    // Verify that the emergency stop flag has NOT been deployed
    test.assertBoolean(radio.bufferSerial.overflowed,false,"Overflow emergency not hit",__LINE__);
    // Verify that there are 15 buffers filled
    test.assertEqualByte(radio.bufferSerial.numberOfPacketsToSend,OPENBCI_MAX_NUMBER_OF_BUFFERS,"15 buffers",__LINE__);
    // Verify the write position
    test.assertEqualByte((radio.bufferSerial.packetBuffer + OPENBCI_MAX_NUMBER_OF_BUFFERS - 1)->positionWrite,OPENBCI_MAX_PACKET_SIZE_BYTES,"32 bytes in buffer",__LINE__);

    // Write one more byte to overflow the buffer
    radio.bufferSerialAddChar(0x00);
    // Verify that the emergency stop flag has been deployed
    test.assertBoolean(radio.bufferSerial.overflowed,true,"Overflow emergency",__LINE__);

    // Remember to clean up after yourself
    testProcessChar_CleanUp();
}

void testProcessChar_CleanUp() {
    // Clear the buffers
    radio.bufferSerialReset(OPENBCI_MAX_NUMBER_OF_BUFFERS);
    radio.bufferStreamReset(radio.streamPacketBuffer);
}

/********************************************/
/********************************************/
/******    Process Radio Char Tests    ******/
/********************************************/
/********************************************/
void testProcessRadioChar() {
    testPacketToSend();
}

// This is used to determine if there is in fact a packet waiting to be sent
void testPacketToSend() {
    test.describe("testPacketToSend");
    // Clear the buffers
    radio.bufferSerialReset(OPENBCI_MAX_NUMBER_OF_BUFFERS);
    radio.bufferStreamReset(radio.streamPacketBuffer);
    // Set the buffers up to think there is a packet to be sent
    //  by triggering a serial read
    char input = 'A';
    // Set last serial read to now
    radio.lastTimeSerialRead = micros();
    // Process that char!
    radio.bufferSerialAddChar(input);
    // Less than 3ms has passed, veryify we can't send a packet
    test.assertBoolean(radio.packetToSend(),false,"Can't send packet yet",__LINE__);
    // Wait for 3 ms
    delayMicroseconds(3000);
    // Re ask if there is something to send
    test.assertBoolean(radio.packetToSend(),true,"Enough time passed",__LINE__);

}

void testByteIdMakeStreamPacketType() {
    test.describe("byteIdMakeStreamPacketType");

    test.assertEqualChar(radio.byteIdMakeStreamPacketType(0xC5),5,"Can get type 5",__LINE__);

    testProcessChar_CleanUp();
}

void writeAStreamPacketToProcessChar(char endByte) {
    // Quickly write a bunch of bytes into the buffers
    radio.bufferStreamAddChar(radio.streamPacketBuffer, 0x41); // make the first one a stream one so 0x41
    radio.bufferStreamAddChar(radio.streamPacketBuffer, 0x00); // Sample number what have you

    for (byte i = 0; i < 8; i++) {
        bufferStreamAdd3Byte(i);
    }
    // 5 bytes - channel 1
    // 8 bytes - channel 2
    // 11 bytes - channel 3
    // 14 bytes - channel 4
    // 17 bytes - channel 5
    // 20 bytes - channel 6
    // 23 bytes - channel 7
    // 26 bytes - channel 8

    for (byte i = 0; i < 3; i++) {
        bufferStreamAdd2Byte(i);
    }
    // 28 bytes - Aux 1
    // 30 bytes - Aux 2
    // 32 bytes - Aux 3

    radio.bufferStreamAddChar(radio.streamPacketBuffer, endByte); // This locks in the final stream packet
    radio.lastTimeSerialRead = micros();
}

void bufferStreamAdd3Byte(byte n) {
    radio.bufferStreamAddChar(radio.streamPacketBuffer, (char)0x00);
    radio.bufferStreamAddChar(radio.streamPacketBuffer, (char)0x00);
    radio.bufferStreamAddChar(radio.streamPacketBuffer, (char)n);
}
void bufferStreamAdd2Byte(byte n) {
    radio.bufferStreamAddChar(radio.streamPacketBuffer, (char)0x00);
    radio.bufferStreamAddChar(radio.streamPacketBuffer, (char)n);
}
