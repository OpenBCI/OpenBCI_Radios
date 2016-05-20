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

    test.end();
}

void testProcessChar() {
    testIsATailByteChar();
    testProcessCharSingleChar();
    testProcessCharStreamPacket();
    testProcessCharNotStreamPacket();
}

void testIsATailByteChar() {
    test.describe("isATailByteChar");

    test.assertEqual((boolean)radio.isATailByteChar((char)0xA0),true,"Stream packet type 0");
    test.assertEqual((boolean)radio.isATailByteChar((char)0xA1),true,"Stream packet type 1");
    test.assertEqual((boolean)radio.isATailByteChar((char)0xA8),true,"Stream packet type 8");
    test.assertEqual((boolean)radio.isATailByteChar((char)0xAA),true,"Stream packet type 10");
    test.assertEqual((boolean)radio.isATailByteChar((char)0xAF),true,"Stream packet type 15");
    test.assertEqual((boolean)radio.isATailByteChar((char)0xB0),false,"Not a stream packet type");
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
    // Verify stream packet buffer
    test.assertEqual(radio.streamPacketBuffer.data[0],input,"Char stoed to stream packet buffer");

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

}

void testProcessCharNotStreamPacket() {
    test.describe("processCharForNotStreamPacket");

    // Clear the buffers
    radio.bufferCleanSerial(OPENBCI_MAX_NUMBER_OF_BUFFERS);
    radio.bufferResetStreamPacketBuffer();

    // Write a stream packet
    writeAStreamPacketToProcessChar(0xA0);
    // Quick! Write another char
    radio.processChar(0xFF);

    test.assertEqual((boolean)radio.isAStreamPacketWaitingForLaunch(),true,"Not ready to launch stream packet");

    test.assertEqual(radio.streamPacketBuffer.bytesIn,1,"1 byte in");


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
