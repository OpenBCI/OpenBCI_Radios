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
    testProcessCharNotStreamPacket();
    testProcessCharOverflow();
}

void testIsATailByteChar() {
    test.describe("isATailByteChar");

    test.assertEqual((boolean)radio.isATailByteChar((char)0xA0),true,"Stream packet type 0");
    test.assertEqual((boolean)radio.isATailByteChar((char)0xA1),true,"Stream packet type 1");
    test.assertEqual((boolean)radio.isATailByteChar((char)0xA8),true,"Stream packet type 8");
    test.assertEqual((boolean)radio.isATailByteChar((char)0xAA),true,"Stream packet type 10");
    test.assertEqual((boolean)radio.isATailByteChar((char)0xAF),true,"Stream packet type 15");
    test.assertEqual((boolean)radio.isATailByteChar((char)0xB0),false,"Not a stream packet type");

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
    test.assertEqual(radio.bufferSerial.packetBuffer->data[radio.bufferSerial.packetBuffer->positionWrite - 1],input,"Char stored to serial buffer");
    test.assertEqual(radio.bufferSerial.numberOfPacketsToSend,1,"Serial buffer has 1 packets");
    Serial.print("Packets to send: "); Serial.println(radio.bufferSerial.numberOfPacketsToSend);
    // Verify stream packet buffer
    test.assertEqual(radio.streamPacketBuffer.data[0],input,"Char stored to stream packet buffer");

    // Remember to clean up after yourself
    testProcessChar_CleanUp();

}

void testProcessCharStreamPacket() {
    test.describe("processCharForStreamPacket");

    // Clear the buffers
    radio.bufferCleanSerial(OPENBCI_MAX_NUMBER_OF_BUFFERS);
    radio.bufferResetStreamPacketBuffer();

    // Write a stream packet with end byte 0xA0
    writeAStreamPacketToProcessChar(0xA0);

    // Right away we want to see if enough time has passed, this should be false
    //  because we just processed a char, after this test is complete, we should
    //  be far passed 90uS
    boolean notEnoughTimePassed = micros() > (radio.lastTimeSerialRead + OPENBCI_TIMEOUT_PACKET_STREAM_uS);
    test.assertEqual((boolean)notEnoughTimePassed,false,"Type 0 waiting...");

    // Do we have a stream packet waiting to launch?
    test.assertEqual((boolean)radio.isAStreamPacketWaitingForLaunch(),true,"Type 0 loaded");

    // This should return true this time
    boolean enoughTimePassed = micros() > (radio.lastTimeSerialRead + OPENBCI_TIMEOUT_PACKET_STREAM_uS);
    test.assertEqual((boolean)enoughTimePassed,true,"Type 0 ready");

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // Try a stream packet with another stop byte /////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // Clear the buffers
    radio.bufferCleanSerial(OPENBCI_MAX_NUMBER_OF_BUFFERS);
    radio.bufferResetStreamPacketBuffer();

    // Write a stream packet with end byte not 0xA0
    writeAStreamPacketToProcessChar(0xA5);

    // Right away we want to see if enough time has passed, this should be false
    //  because we just processed a char, after this test is complete, we should
    //  be far passed 90uS
    notEnoughTimePassed = micros() > (radio.lastTimeSerialRead + OPENBCI_TIMEOUT_PACKET_STREAM_uS);
    test.assertEqual((boolean)notEnoughTimePassed,false,"Type 5 waiting");

    // Do we have a stream packet waiting to launch?
    test.assertEqual((boolean)radio.isAStreamPacketWaitingForLaunch(),true,"Type 5 loaded");

    // This should return true this time
    enoughTimePassed = micros() > (radio.lastTimeSerialRead + OPENBCI_TIMEOUT_PACKET_STREAM_uS);
    test.assertEqual((boolean)enoughTimePassed,true,"Type 5 ready");

    // Remember to clean up after yourself
    testProcessChar_CleanUp();

}

// Test conditions that result in a stream packet not being launched
void testProcessCharNotStreamPacket() {
    test.describe("processCharForNotStreamPacket");

    // Clear the buffers
    radio.bufferCleanSerial(OPENBCI_MAX_NUMBER_OF_BUFFERS);
    radio.bufferResetStreamPacketBuffer();

    // Write a stream packet
    writeAStreamPacketToProcessChar(0xA0);
    // Quick! Write another char
    radio.processChar(0xFF);

    test.assertEqual((boolean)radio.isAStreamPacketWaitingForLaunch(),false,"Too many packets in");
    test.assertEqual(radio.streamPacketBuffer.bytesIn,1,"1 byte in");

    // Clear the buffers
    radio.bufferCleanSerial(OPENBCI_MAX_NUMBER_OF_BUFFERS);
    radio.bufferResetStreamPacketBuffer();

    // Write a stream packet with a bad end byte
    writeAStreamPacketToProcessChar(0xB5);
    test.assertEqual((boolean)radio.isAStreamPacketWaitingForLaunch(),false,"Bad end byte");

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
    test.assertEqual((boolean)radio.bufferSerial.overflowed,false,"Overflow emergency not hit");
    // Verify that there are 15 buffers filled
    test.assertEqual(radio.bufferSerial.numberOfPacketsToSend,OPENBCI_MAX_NUMBER_OF_BUFFERS,"15 buffers");
    // Verify the write position
    test.assertEqual((radio.bufferSerial.packetBuffer + OPENBCI_MAX_NUMBER_OF_BUFFERS - 1)->positionWrite,OPENBCI_MAX_PACKET_SIZE_BYTES,"32 bytes in buffer");

    // Write one more byte to overflow the buffer
    radio.processChar(0x00);
    // Verify that the emergency stop flag has been deployed
    test.assertEqual((boolean)radio.bufferSerial.overflowed,true,"Overflow emergency");

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
    // Set the buffers up to think there is a packet to be sent
    //  by triggering a serial read
    radio.processChar(0x41);
    // Less than 3ms has passed, veryify we can't send a packet
    test.assertEqual(radio.packetToSend(),false,"Can't send packet yet");
    // Wait for 3 ms
    delay(3);
    // Re ask if there is something to send
    test.assertEqual(radio.packetToSend(),true,"Can send packet");

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
}
