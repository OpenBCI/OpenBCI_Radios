#include <RFduinoGZLL.h>
#include "OpenBCI_Radio.h"
#include "PTW-Arduino-Assert.h"

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    test.setSerial(Serial);
}

void loop() {
    // put your main code here, to run repeatedly:
    if (Serial.available()) {
        Serial.read();
        go();
    }
}

void go() {
    // Start the test
    test.begin();

    testCheckSum();
    testByteIdMake();
    testByteIdGetPacketNumber();
    testByteIdGetStreamPacketType();
    testByteIdGetCheckSum();
    testOutput();
    testBufferCleanChar();
    testBufferCleanPacketBuffer();
    testDidPicSendDeviceAStreamPacket();

    test.end();
}

void testCheckSum() {
    test.describe("checkSum");

    char temp1[3];
    temp1[1] = 'a';
    temp1[2] = 'j';

    char temp2[3];
    temp2[1] = 'a';
    temp2[2] = 'k';


    char checkSum1 = OpenBCI_Radio.checkSumMake(temp1 + 1,2);
    char checkSum2 = OpenBCI_Radio.checkSumMake(temp2 + 1,2);

    test.assertEqual(checkSum1,0x05,"Get's correct check sum for given char *");
    test.assertGreaterThan(checkSum1,checkSum2,"Data with one byte of 1 less than other has smaller check sum");
    test.assertEqual(checkSum1 - checkSum2,1,"Check sums of two different packets changed by 1 bit shall be one 1 less");

    // Get byteId for this packet
    char byteId = OpenBCI_Radio.byteIdMake(false, 0, temp1 + 1, 2);
    // Set byteId for this packet
    temp1[0] = byteId;
    test.assertEqual(OpenBCI_Radio.checkSumsAreEqual(temp1,3),true,"Check sums verification works in good condition");

    // Mess it up on purpose
    temp1[0] = byteId & 0x00;
    test.assertEqual(OpenBCI_Radio.checkSumsAreEqual(temp1,3),false,"Check sums verification works in bad condition");

}

void testByteIdMake() {

    test.describe("byteIdMake");
    // Streaming
    char byteId = OpenBCI_Radio.byteIdMake(true,0,NULL,0);
    test.assertGreaterThan(byteId,0x7f,"Streaming byteId has 1 in MSB");

    // Not streaming
    byteId = OpenBCI_Radio.byteIdMake(false,0,NULL,0);
    test.assertLessThan(byteId,0x80,"Non streaming byteId has 0 in MSB");

    byteId = OpenBCI_Radio.byteIdMake(false,1,NULL,0);
    test.assertEqual(byteId,0x08,"Can set packet number of 1 in byteId");

    byteId = OpenBCI_Radio.byteIdMake(false,9,NULL,0);
    test.assertEqual(byteId,0x48,"Can set packet number of 9 in byteId");
}

void testByteIdGetPacketNumber() {
    test.describe("byteIdGetPacketNumber");

    int expectedPacketNumber = 10; // Expected packet number
    char byteId = 0x50;
    int actualPacketNumber = OpenBCI_Radio.byteIdGetPacketNumber(byteId);
    test.assertEqual(actualPacketNumber,expectedPacketNumber,"Extracts packet number from byteId");
}

void testByteIdGetStreamPacketType() {
    test.describe("byteIdGetStreamPacketType");

    byte expectedStreamPacketType = 0x00;
    char byteId = 0x80;
    byte actualStreamPacketType = OpenBCI_Radio.byteIdGetStreamPacketType(byteId);

    test.assertEqual(expectedStreamPacketType,actualStreamPacketType,"Stream Packet");

    expectedStreamPacketType = 0x01;
    byteId = 0x88;
    actualStreamPacketType = OpenBCI_Radio.byteIdGetStreamPacketType(byteId);

    test.assertEqual(expectedStreamPacketType,actualStreamPacketType,"Time Sync Packet");

}

void testByteIdGetCheckSum() {
    test.describe("byteIdGetCheckSum");

    char checkSum = OpenBCI_Radio.byteIdGetCheckSum(0x1B);

    test.assertEqual(checkSum,0x03,"Extracts check sum from byte id");
}

void testOutput() {
    test.describe("outputGetStopByteFromByteId");

    // Stream
    byte expectedStopByte = 0b11000000;
    char byteId = 0b10000000; // normal stream packet
    byte actualStopByte = OpenBCI_Radio.outputGetStopByteFromByteId(byteId);
    test.assertEqual(expectedStopByte,actualStopByte,"Gets a Stream Packet");

    // Time sync packet
    expectedStopByte = 0b11000111;
    byteId = 0b10111111; // 0b1(0111)111

    actualStopByte = OpenBCI_Radio.outputGetStopByteFromByteId(byteId);
    test.assertEqual(expectedStopByte,actualStopByte,"Time sync packet");

}

void testBufferCleanChar() {
    test.describe("bufferCleanChar");

    char buffer[] = "AJ";
    char testMessage[] = "buf at index 0 cleared to 0x00";

    OpenBCI_Radio.bufferCleanChar(buffer, sizeof(buffer));

    for (int i = 0; i < sizeof(buffer); i++) {
        testMessage[13] = (char)i + '0';
        test.assertEqual(buffer[i],0x00, testMessage);
    }
}

void testBufferCleanPacketBuffer() {
    test.describe("bufferCleanPacketBuffer");

    int numberOfPackets = 10;

    for (int i = 0; i < numberOfPackets; i++) {
        (OpenBCI_Radio.bufferSerial.packetBuffer + i)->positionRead = 5 + i;
        (OpenBCI_Radio.bufferSerial.packetBuffer + i)->positionWrite = 6 + i;
    }

    OpenBCI_Radio.bufferCleanPacketBuffer(OpenBCI_Radio.bufferSerial.packetBuffer,numberOfPackets);

    char testMessage1[] = "buf at index 0 positionRead reset";
    char testMessage2[] = "buf at index 0 positionWrite reset";
    for (int j = 0; j < numberOfPackets; j++) {
        testMessage1[13] = (char)j + '0';
        test.assertEqual((OpenBCI_Radio.bufferSerial.packetBuffer + j)->positionRead, 0x00, testMessage1);
        testMessage2[13] = (char)j + '0';
        test.assertEqual((OpenBCI_Radio.bufferSerial.packetBuffer + j)->positionWrite, 0x01, testMessage2);
    }
}

void testDidPicSendDeviceAStreamPacket() {
    test.describe("didPicSendDeviceAStreamPacket");
    
    OpenBCI_Radio.verbosePrintouts = true;

    // Store some data
    for (int i = 0; i < OPENBCI_MAX_DATA_BYTES_IN_PACKET; i++) {
        OpenBCI_Radio.bufferSerial.packetBuffer->data[i] = i + '0';
    }
    (OpenBCI_Radio.bufferSerial.packetBuffer + 1)->data[1] = 'A';
    (OpenBCI_Radio.bufferSerial.packetBuffer + 1)->data[2] = 'J';
    (OpenBCI_Radio.bufferSerial.packetBuffer + 1)->data[3] = 0xF0;
    
    OpenBCI_Radio.bufferSerial.numberOfPacketsToSend = 2;
    (OpenBCI_Radio.bufferSerial.packetBuffer + 1)->positionWrite = 4;

    test.assertEqual(OpenBCI_Radio.didPicSendDeviceAStreamPacket(), true, "Detects stream packet");
    
    // change just the last 4 bits to make sure we still pass the code
    (OpenBCI_Radio.bufferSerial.packetBuffer + 1)->data[3] = 0xF1;
    test.assertEqual(OpenBCI_Radio.didPicSendDeviceAStreamPacket(), true, "Detects time sync packet");
    
    // Not a stream packet
    (OpenBCI_Radio.bufferSerial.packetBuffer + 1)->data[3] = 0xC1;
    test.assertEqual(OpenBCI_Radio.didPicSendDeviceAStreamPacket(), false, "Not a time sync packet");

}

