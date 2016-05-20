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
    // testProcessCharStreamPacket();
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

    test.assertEqual(radio.bufferSerial.packetBuffer->data[radio.bufferSerial.packetBuffer.positionWrite - 1],input,"Char stored to serial buffer");
    test.assertEqual(radio.streamPacketBuffer->data[0],input,"Char stoed to stream packet buffer");

}

void testProcessCharStreamPacket() {
    test.describe("processCharForStreamPacket");

    // Quickly write a bunch of bytes into the buffers
    radio.processChar(0x41); // make the first one a stream one so 0x41
    radio.processChar(0x00); radio.processChar(0x00); radio.processChar(0x00); // 4
    radio.processChar(0x00); radio.processChar(0x00); radio.processChar(0x00); // 7
    radio.processChar(0x00); radio.processChar(0x00); radio.processChar(0x00); // 10
    radio.processChar(0x00); radio.processChar(0x00); radio.processChar(0x00); // 13
    radio.processChar(0x00); radio.processChar(0x00); radio.processChar(0x00); // 16
    radio.processChar(0x00); radio.processChar(0x00); radio.processChar(0x00); // 19
    radio.processChar(0x00); radio.processChar(0x00); radio.processChar(0x00); // 22
    radio.processChar(0x00); radio.processChar(0x00); radio.processChar(0x00); // 25
    radio.processChar(0x00); radio.processChar(0x00); radio.processChar(0x00); // 28
    radio.processChar(0x00); radio.processChar(0x00); radio.processChar(0x00); // 31
    radio.processChar(0xA0); // This locks in the final stream packet

    test.assertEqual((boolean)radio.isAStreamPacketWaitingForLaunch(),true,"Sucessfully processed a stream packet");

    boolean notEnoughTimePassed = micros() > (radio.lastTimeSerialRead + OPENBCI_TIMEOUT_PACKET_STREAM_uS);
    test.assertEqual((boolean)notEnoughTimePassed,false,"Not enough time has passed");

    boolean enoughTimePassed = micros() > (radio.lastTimeSerialRead + OPENBCI_TIMEOUT_PACKET_STREAM_uS);
    test.assertEqual((boolean)enoughTimePassed,true,"Enough time has passed");



}
