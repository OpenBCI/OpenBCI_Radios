#include <RFduinoGZLL.h>
#include "OpenBCI_Radios.h"
#include "PTW-Arduino-Assert.h"

void setup() {
    // Get's serial up and running
    Serial.begin(115200);
    test.setSerial(Serial);
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

    testProcessChar();
    testProcessRadioChar();
    testByteIdMakeStreamPacketType();

    test.end();
}

/********************************************/
/********************************************/
/*********    Process Char Tests    *********/
/********************************************/
/********************************************/
void testProcessChar() {
    testIsATailByteChar();
    testProcessCharSingleChar();
    testProcessCharStreamPacket();
    testProcessCharStreamPackets();
    testProcessCharNotStreamPacket();
    testProcessCharOverflow();
}

void testIsATailByteChar() {
    test.describe("isATailByteChar");

    test.assertEqualBoolean(radio.isATailByteChar((char)0xC0),true,"Stream packet type 0");
    test.assertEqualBoolean(radio.isATailByteChar((char)0xC1),true,"Stream packet type 1");
    test.assertEqualBoolean(radio.isATailByteChar((char)0xC8),true,"Stream packet type 8");
    test.assertEqualBoolean(radio.isATailByteChar((char)0xCA),true,"Stream packet type 10");
    test.assertEqualBoolean(radio.isATailByteChar((char)0xCF),true,"Stream packet type 15");
    test.assertEqualBoolean(radio.isATailByteChar((char)0xB0),false,"Not a stream packet type");

    // Remember to clean up after yourself
    testProcessChar_CleanUp();
}

void testProcessCharSingleChar() {
    test.describe("processCharForSingleChar");

    // Clear the buffers
    radio.bufferCleanSerial(OPENBCI_MAX_NUMBER_OF_BUFFERS);
    radio.bufferResetStreamPacketBuffer();

    // try to add a char
    char input = 'A';
    radio.processChar(input);

    // Verify serial buffer
    test.assertEqualChar(radio.bufferSerial.packetBuffer->data[radio.bufferSerial.packetBuffer->positionWrite - 1],input,"Char stored to serial buffer");
    test.assertEqualChar(radio.bufferSerial.numberOfPacketsToSend,1,"Serial buffer has 1 packet to send");
    test.assertEqualChar(radio.bufferSerial.numberOfPacketsSent,0,"Serial buffer not sent any packets");
    // Verify stream packet buffer
    test.assertEqualChar(radio.streamPacketBuffer.data[0],input,"Char stored to stream packet buffer");

    // Remember to clean up after yourself
    testProcessChar_CleanUp();

}

void testProcessCharStreamPacket() {
    test.describe("processCharForStreamPacket");

    // Clear the buffers
    radio.bufferCleanSerial(OPENBCI_MAX_NUMBER_OF_BUFFERS);
    radio.bufferResetStreamPacketBuffer();

    // Write a stream packet with end byte 0xA0
    writeAStreamPacketToProcessChar(0xC0);

    // Right away we want to see if enough time has passed, this should be false
    //  because we just processed a char, after this test is complete, we should
    //  be far passed 90uS
    boolean notEnoughTimePassed = micros() > (radio.lastTimeSerialRead + OPENBCI_TIMEOUT_PACKET_STREAM_uS);
    test.assertEqualBoolean(notEnoughTimePassed,false,"Type 0 waiting...");

    // Do we have a stream packet waiting to launch?
    test.assertEqualBoolean(radio.isAStreamPacketWaitingForLaunch(),true,"Type 0 loaded");

    // This should return true this time
    boolean enoughTimePassed = micros() > (radio.lastTimeSerialRead + OPENBCI_TIMEOUT_PACKET_STREAM_uS);
    test.assertEqualBoolean(enoughTimePassed,true,"Type 0 ready");

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // Try a stream packet with another stop byte /////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // Clear the buffers
    radio.bufferCleanSerial(OPENBCI_MAX_NUMBER_OF_BUFFERS);
    radio.bufferResetStreamPacketBuffer();

    // Write a stream packet with end byte not 0xA0
    writeAStreamPacketToProcessChar(0xC5);

    // Right away we want to see if enough time has passed, this should be false
    //  because we just processed a char, after this test is complete, we should
    //  be far passed 90uS
    notEnoughTimePassed = micros() > (radio.lastTimeSerialRead + OPENBCI_TIMEOUT_PACKET_STREAM_uS);
    test.assertEqualBoolean(notEnoughTimePassed,false,"Type 5 waiting");

    // Do we have a stream packet waiting to launch?
    test.assertEqualBoolean(radio.isAStreamPacketWaitingForLaunch(),true,"Type 5 loaded");

    // This should return true this time
    enoughTimePassed = micros() > (radio.lastTimeSerialRead + OPENBCI_TIMEOUT_PACKET_STREAM_uS);
    test.assertEqualBoolean(enoughTimePassed,true,"Type 5 ready");

    // Remember to clean up after yourself
    testProcessChar_CleanUp();

}

// Send stream packets, one after the other
void testProcessCharStreamPackets() {
    test.describe("processCharForStreamPackets");

    // Clear the buffers
    radio.bufferCleanSerial(OPENBCI_MAX_NUMBER_OF_BUFFERS);
    radio.bufferResetStreamPacketBuffer();

    int numberOfTrials = 9;
    char testMessage[] = "Sent 0";
    unsigned long t1, t2;

    for (int i = 0; i < numberOfTrials; i++) {
        // Write a stream packet with end byte 0xC0
        writeAStreamPacketToProcessChar(0xC0);
        // Stream packet should be waiting
        test.assertEqualBoolean(radio.isAStreamPacketWaitingForLaunch(),true);
        // Save the current time;
        t1 = micros();
        // Wait
        while(micros() < (radio.lastTimeSerialRead + OPENBCI_TIMEOUT_PACKET_STREAM_uS)) {};
        // configure the test message
        testMessage[5] = (i + 1) + '0';
        // Send the stream packet
        test.assertEqualBoolean(radio.sendStreamPacketToTheHost(),true,testMessage);
    }
}

// Test conditions that result in a stream packet not being launched
void testProcessCharNotStreamPacket() {
    test.describe("processCharForNotStreamPacket");

    // Clear the buffers
    radio.bufferCleanSerial(OPENBCI_MAX_NUMBER_OF_BUFFERS);
    radio.bufferResetStreamPacketBuffer();

    // Write a stream packet
    writeAStreamPacketToProcessChar(0xC0);
    // Fake Serial read
    char newChar = (char)0xFF;
    // Save current time as the last serial read
    radio.lastTimeSerialRead = micros();
    // Quick! Write another char
    radio.processChar(newChar);

    test.assertEqualBoolean(radio.isAStreamPacketWaitingForLaunch(),false,"Too many packets in");
    test.assertEqualInt(radio.streamPacketBuffer.bytesIn,0,"0 bytes in");

    // Clear the buffers
    radio.bufferCleanSerial(OPENBCI_MAX_NUMBER_OF_BUFFERS);
    radio.bufferResetStreamPacketBuffer();

    // Write a stream packet with a bad end byte
    writeAStreamPacketToProcessChar(0xB5);
    test.assertEqualBoolean(radio.isAStreamPacketWaitingForLaunch(),false,"Bad end byte");


    // Remember to clean up after yourself
    testProcessChar_CleanUp();
}

// Put the system in an overflow condition
void testProcessCharOverflow() {
    test.describe("testProcessCharOverflow");

    // Clear the buffers
    radio.bufferCleanSerial(OPENBCI_MAX_NUMBER_OF_BUFFERS);
    radio.bufferResetStreamPacketBuffer();

    // Write the max number of bytes in buffers
    int maxBytes = OPENBCI_MAX_NUMBER_OF_BUFFERS * OPENBCI_MAX_DATA_BYTES_IN_PACKET;
    // Write max bytes but stop 1 before
    for (int i = 0; i < maxBytes; i++) {
        radio.processChar(0x00);
    }

    // Verify that the emergency stop flag has NOT been deployed
    test.assertEqualBoolean(radio.bufferSerial.overflowed,false,"Overflow emergency not hit");
    // Verify that there are 15 buffers filled
    test.assertEqualInt(radio.bufferSerial.numberOfPacketsToSend,OPENBCI_MAX_NUMBER_OF_BUFFERS,"15 buffers");
    // Verify the write position
    test.assertEqualInt((radio.bufferSerial.packetBuffer + OPENBCI_MAX_NUMBER_OF_BUFFERS - 1)->positionWrite,OPENBCI_MAX_PACKET_SIZE_BYTES,"32 bytes in buffer");

    // Write one more byte to overflow the buffer
    radio.processChar(0x00);
    // Verify that the emergency stop flag has been deployed
    test.assertEqualBoolean(radio.bufferSerial.overflowed,true,"Overflow emergency");

    // Remember to clean up after yourself
    testProcessChar_CleanUp();
}

void testProcessChar_CleanUp() {
    // Clear the buffers
    radio.bufferCleanSerial(OPENBCI_MAX_NUMBER_OF_BUFFERS);
    radio.bufferResetStreamPacketBuffer();
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
    radio.bufferCleanSerial(OPENBCI_MAX_NUMBER_OF_BUFFERS);
    radio.bufferResetStreamPacketBuffer();
    // Set the buffers up to think there is a packet to be sent
    //  by triggering a serial read
    char input = 'A';
    // Set last serial read to now
    radio.lastTimeSerialRead = micros();
    // Process that char!
    radio.processChar(input);
    // Serial.print("Pos write: "); Serial.println(radio.bufferSerial.packetBuffer->positionWrite);
    // test.assertEqualChar(radio.bufferSerial.packetBuffer->data[radio.bufferSerial.packetBuffer->positionWrite - 1],input,"Char stored to serial buffer");
    // test.assertEqualInt(radio.bufferSerial.numberOfPacketsToSend,1,"Serial buffer has 1 packet to send");
    // test.assertEqualInt(radio.bufferSerial.numberOfPacketsSent,0,"Serial buffer not sent any packets");

    // Less than 3ms has passed, veryify we can't send a packet
    test.assertEqualBoolean(radio.packetToSend(),false,"Can't send packet yet");
    // Serial.print("Packets sent: "); Serial.println(radio.bufferSerial.numberOfPacketsSent);
    // Serial.print("Packets to send: "); Serial.println(radio.bufferSerial.numberOfPacketsToSend);
    // Wait for 3 ms
    delayMicroseconds(3000);
    // Serial.print("Packets sent: "); Serial.println(radio.bufferSerial.numberOfPacketsSent);
    // Serial.print("Packets to send: "); Serial.println(radio.bufferSerial.numberOfPacketsToSend);
    // Re ask if there is something to send
    test.assertEqualBoolean(radio.packetToSend(),true,"Enough time passed");

}

void testByteIdMakeStreamPacketType() {
    test.describe("byteIdMakeStreamPacketType");

    radio.streamPacketBuffer.typeByte = 0xC5;
    test.assertEqualChar(radio.byteIdMakeStreamPacketType(),5,"Can get type 5");

    testProcessChar_CleanUp();
}

void writeAStreamPacketToProcessChar(char endByte) {
    // Quickly write a bunch of bytes into the buffers
    radio.processChar(0x41); // make the first one a stream one so 0x41
    radio.processChar(0x00); // Sample number what have you
    radio.processChar(0x00); radio.processChar(0x00); radio.processChar(0x01); // 5 bytes - channel 1
    radio.processChar(0x00); radio.processChar(0x00); radio.processChar(0x02); // 8 bytes - channel 2
    radio.processChar(0x00); radio.processChar(0x00); radio.processChar(0x03); // 11 bytes - channel 3
    radio.processChar(0x00); radio.processChar(0x00); radio.processChar(0x04); // 14 bytes - channel 4
    radio.processChar(0x00); radio.processChar(0x00); radio.processChar(0x05); // 17 bytes - channel 5
    radio.processChar(0x00); radio.processChar(0x00); radio.processChar(0x06); // 20 bytes - channel 6
    radio.processChar(0x00); radio.processChar(0x00); radio.processChar(0x07); // 23 bytes - channel 7
    radio.processChar(0x00); radio.processChar(0x00); radio.processChar(0x08); // 26 bytes - channel 8
    radio.processChar(0x00); radio.processChar(0x01); // 28 bytes - Aux 1
    radio.processChar(0x00); radio.processChar(0x02); // 30 bytes - Aux 2
    radio.processChar(0x00); radio.processChar(0x03); // 32 bytes - Aux 3
    radio.processChar(endByte); // This locks in the final stream packet
    radio.lastTimeSerialRead = micros();
}
