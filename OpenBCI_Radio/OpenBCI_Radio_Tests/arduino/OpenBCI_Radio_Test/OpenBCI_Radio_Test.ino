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

    test.end();
}

void test() {
    // Start the test
    test.begin();

    testCheckSum();
    testByteIdMake();

    test.end();
}

void testCheckSum() {
    test.describe("checkSum");

    char temp1[3];
    temp2[1] = 'a';
    temp2[2] = 'j';

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


  testByteIdMake_IsStreamPacket();
  testByteIdMake_NotStreamPacket();
  testByteIdMake_PacketNumber1();
  testByteIdMake_PacketNumber9();
  testByteIdMake_GetPacketNumber();
  testByteIdMake_GetStreamPacketType_Stream();
  testByteIdMake_GetStreamPacketType_TimeSync();
  testByteIdMake_GetCheckSum();

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

}

void testByteIdGetStreamPacketType() {
    test.describe("byteIdGetStreamPacketType");

    byte expectedStreamPacketType = 0x01;
    char byteId = 0x88;
    byte actualStreamPacketType = OpenBCI_Radio.byteIdGetStreamPacketType(byteId);

    test.assertEqual(expectedStreamPacketType,actualStreamPacketType,"Time Sync Packet");
}

void testByteIdGetCheckSum() {
    test.describe("byteIdGetCheckSum");

    char checkSum = OpenBCI_Radio.byteIdGetCheckSum(0x1B);

    test.assertEqualChar(checkSum,0x03,"Extracts check sum from byte id");
}




boolean testOutput() {
    test.describe("outputGetStopByteFromByteId");

    // Stream
    byte expectedStopByte = 0b11000000;
    char byteId = 0b10000000; // normal stream packet
    byte actualStopByte = OpenBCI_Radio.outputGetStopByteFromByteId(byteId);
    test.assertEqualByte(expectedStopByte,actualStopByte,"Gets a Stream Packet");

    // Time sync packet
    expectedStopByte = 0b11000111;
    byteId = 0b10111111; // 0b1(0111)111

    actualStopByte = OpenBCI_Radio.outputGetStopByteFromByteId(byteId);
    test.assertEqualByte(expectedStopByte,actualStopByte);

}
