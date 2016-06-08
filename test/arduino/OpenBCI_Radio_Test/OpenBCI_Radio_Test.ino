#include <RFduinoGZLL.h>
#include "OpenBCI_Radios.h"
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
    testByteId();
    testOutput();
    testBuffer();
    testNonVolatileFunctions();

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


    char checkSum1 = radio.checkSumMake(temp1 + 1,2);
    char checkSum2 = radio.checkSumMake(temp2 + 1,2);

    test.assertEqualChar(checkSum1,0x05,"Get's correct check sum for given char *");
    test.assertGreaterThanChar(checkSum1,checkSum2,"Data with one byte of 1 less than other has smaller check sum");
    test.assertEqualInt(checkSum1 - checkSum2,1,"Check sums of two different packets changed by 1 bit shall be one 1 less");

    // Get byteId for this packet
    char byteId = radio.byteIdMake(false, 0, temp1 + 1, 2);
    // Set byteId for this packet
    temp1[0] = byteId;
    test.assertEqualBoolean(radio.checkSumsAreEqual(temp1,3),true,"Check sums verification works in good condition");

    // Mess it up on purpose
    temp1[0] = byteId & 0x00;
    test.assertEqualBoolean(radio.checkSumsAreEqual(temp1,3),false,"Check sums verification works in bad condition");

}

void testByteId() {
    testByteIdMake();
    testByteIdGetPacketNumber();
    testByteIdGetStreamPacketType();
    testByteIdGetCheckSum();
}

void testByteIdMake() {
    char byteId;

    test.describe("byteIdMake");
    // Streaming
    byteId = radio.byteIdMake(true,0,NULL,0);
    test.assertGreaterThanChar(byteId,(char)0x7f,"Streaming byteId has 1 in MSB");

    // Not streaming
    byteId = radio.byteIdMake(false,0,NULL,0);
    test.assertLessThanChar(byteId,(char)0x80,"Non streaming byteId has 0 in MSB");

    byteId = radio.byteIdMake(false,1,NULL,0);
    test.assertEqualChar(byteId,(char)0x08,"Can set packet number of 1 in byteId");

    byteId = radio.byteIdMake(false,9,NULL,0);
    test.assertEqualChar(byteId,(char)0x48,"Can set packet number of 9 in byteId");

    radio.bufferCleanSerial(12);

    for (int i = 0; i < OPENBCI_MAX_DATA_BYTES_IN_PACKET; i++) {
        radio.bufferSerial.packetBuffer->data[i] = 0;
        radio.bufferSerial.packetBuffer->positionWrite++;
    }
    (radio.bufferSerial.packetBuffer + 1)->data[1] = 'A';
    (radio.bufferSerial.packetBuffer + 1)->data[2] = 'J';
    (radio.bufferSerial.packetBuffer + 1)->data[3] = 0xF0;

    radio.bufferSerial.numberOfPacketsToSend = 2;
    (radio.bufferSerial.packetBuffer + 1)->positionWrite = 4;

    byteId = radio.byteIdMake(true,0,radio.bufferSerial.packetBuffer->data + 1,radio.bufferSerial.packetBuffer->positionWrite - 1);
    test.assertEqualChar(byteId,(char)0x80,"byteId for empty data packet is 0x80");
}

void testByteIdGetPacketNumber() {
    test.describe("byteIdGetPacketNumber");

    int expectedPacketNumber = 10; // Expected packet number
    char byteId = 0x50;
    int actualPacketNumber = radio.byteIdGetPacketNumber(byteId);
    test.assertEqualInt(actualPacketNumber,expectedPacketNumber,"Extracts packet number from byteId");
}

void testByteIdGetStreamPacketType() {
    test.describe("byteIdGetStreamPacketType");

    char expectedStreamPacketType = 0x00;
    char byteId = 0x80;
    char actualStreamPacketType = radio.byteIdGetStreamPacketType(byteId);

    test.assertEqualChar(expectedStreamPacketType,actualStreamPacketType,"Stream Packet");

    expectedStreamPacketType = 0x01;
    byteId = 0x88;
    actualStreamPacketType = radio.byteIdGetStreamPacketType(byteId);

    test.assertEqualChar(expectedStreamPacketType,actualStreamPacketType,"Time Sync Packet");

}

void testByteIdGetCheckSum() {
    test.describe("byteIdGetCheckSum");

    char checkSum = radio.byteIdGetCheckSum(0x1B);

    test.assertEqualChar(checkSum,(char)0x03,"Extracts check sum from byte id");
}

void testOutput() {
    test.describe("outputGetStopByteFromByteId");

    // Stream
    byte expectedStopByte = 0b11000000;
    char byteId = 0b10000000; // normal stream packet
    byte actualStopByte = radio.outputGetStopByteFromByteId(byteId);
    test.assertEqualChar((char)expectedStopByte,(char)actualStopByte,"Gets a Stream Packet");

    // Time sync packet
    expectedStopByte = 0b11000111;
    byteId = 0b10111111; // 0b1(0111)111

    actualStopByte = radio.outputGetStopByteFromByteId(byteId);
    test.assertEqualChar((char)expectedStopByte,(char)actualStopByte,"Time sync packet");

}

void testBuffer() {
    testBufferCleanChar();
    testBufferCleanPacketBuffer();
}


void testBufferCleanChar() {
    test.describe("bufferCleanChar");

    char buffer[] = "AJ";
    char testMessage[] = "buf at index 0 cleared to 0x00";

    radio.bufferCleanChar(buffer, sizeof(buffer));

    for (int i = 0; i < sizeof(buffer); i++) {
        testMessage[13] = (char)i + '0';
        test.assertEqualChar(buffer[i],(char)0x00, testMessage);
    }
}

void testBufferCleanPacketBuffer() {
    test.describe("bufferCleanPacketBuffer");

    int numberOfPackets = 10;

    for (int i = 0; i < numberOfPackets; i++) {
        (radio.bufferSerial.packetBuffer + i)->positionRead = 5 + i;
        (radio.bufferSerial.packetBuffer + i)->positionWrite = 6 + i;
    }

    radio.bufferCleanPacketBuffer(radio.bufferSerial.packetBuffer,numberOfPackets);

    char testMessage1[] = "buf at index 0 positionRead reset";
    char testMessage2[] = "buf at index 0 positionWrite reset";
    for (int j = 0; j < numberOfPackets; j++) {
        testMessage1[13] = (char)j + '0';
        test.assertEqualInt((radio.bufferSerial.packetBuffer + j)->positionRead, 0x00, testMessage1);
        testMessage2[13] = (char)j + '0';
        test.assertEqualInt((radio.bufferSerial.packetBuffer + j)->positionWrite, 0x01, testMessage2);
    }
}

void testNonVolatileFunctions() {
    testNonVolatileFlashNonVolatileMemory();
}

void testNonVolatileFlashNonVolatileMemory() {
    test.describe("flashNonVolatileMemory");

    // Try to flash the memory space
    test.assertEqualBoolean(radio.flashNonVolatileMemory(),true,"Erase page of memory");
    // The channel needs to be set
    test.assertEqualBoolean(radio.needToSetChannelNumber(),true,"Need to set channel number");
    // The poll time should also need to be set
    test.assertEqualBoolean(radio.needToSetPollTime(),true,"Need to set the poll time.");

    test.describe("setPollTime");
    uint32_t expectedPollTime = 100;
    // Set the poll time to new poll time
    test.assertEqualBoolean(radio.setPollTime(expectedPollTime),true,"Able to set poll time");
    // Verify the poll time is set to the new poll time
    test.assertEqualInt((int)radio.getPollTime(),(int)expectedPollTime,"Poll time set correctly");
    // Verify the poll time does not have to be set
    test.assertEqualBoolean(radio.needToSetPollTime(),false,"Don't need to set poll time");
    // Reset the poll time to default
    test.assertEqualBoolean(radio.revertToDefaultPollTime(),true,"Reset poll time to default");
    // Verify the poll time is set to default
    test.assertEqualInt((int)radio.getPollTime(),(int)OPENBCI_TIMEOUT_PACKET_POLL_MS,"Poll time is set to define");
    // Verify the poll time does not have to be set
    test.assertEqualBoolean(radio.needToSetPollTime(),false,"Don't need to set poll time");

    test.describe("setChannelNumber");
    uint32_t newChannelNumber = 10;
    // Set channel number to new channel number
    test.assertEqualBoolean(radio.setChannelNumber(newChannelNumber),true,"Channel set");
    // Verify the channel number has been set to new channel number
    test.assertEqualInt((int)radio.getChannelNumber(),(int)newChannelNumber,"Channel set correctly");
    // Verify the channel number does not have to be set
    test.assertEqualBoolean(radio.needToSetChannelNumber(),false,"Don't need to set the channel number");
}
