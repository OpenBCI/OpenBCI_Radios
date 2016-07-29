#include <RFduinoGZLL.h>
#include "OpenBCI_Radios.h"
#include "PTW-Arduino-Assert.h"

int ledPin = 2;

void setup() {
    // put your setup code here, to run once:
    pinMode(ledPin,OUTPUT);
    // put your setup code here, to run once:
    Serial.begin(115200);
    test.setSerial(Serial);
    test.failVerbosity = true;
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
    digitalWrite(ledPin, HIGH);
    // testByteId();
    // testOutput();
    testBuffer();
    // testNonVolatileFunctions();

    test.end();
    digitalWrite(ledPin, LOW);
}

void testByteId() {
    testByteIdMake();
    testByteIdGetPacketNumber();
    testByteIdGetStreamPacketType();
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
    uint32_t expectedPollTime = 200;
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
    // Verify the poll time is still set to define
    test.assertEqualInt((int)radio.getPollTime(),(int)OPENBCI_TIMEOUT_PACKET_POLL_MS,"Poll time is still set");

    test.describe("setPollAndChan");
    // Set the channel to a new channel number lower than the first
    newChannelNumber = 2;
    test.assertEqualBoolean(radio.setChannelNumber(newChannelNumber),true,"Newer channel set");
    // Verify the channel number has been set to new channel number
    test.assertEqualInt((int)radio.getChannelNumber(),(int)newChannelNumber,"Newer channel set correctly");
    // Verify the channel number does not have to be set
    test.assertEqualBoolean(radio.needToSetChannelNumber(),false,"Still don't need to set the channel number");
    // Verify the poll time is still set to define
    test.assertEqualInt((int)radio.getPollTime(),(int)OPENBCI_TIMEOUT_PACKET_POLL_MS,"Poll time is still set");
    // Set the poll time to a new poll time
    expectedPollTime = 100;
    test.assertEqualBoolean(radio.setPollTime(expectedPollTime),true,"Set poll time");
    // Verify the poll time is set to the new poll time
    test.assertEqualInt((int)radio.getPollTime(),(int)expectedPollTime,"Poll time set correctly");
    // Verify the channel number is still set
    test.assertEqualInt((int)radio.getChannelNumber(),(int)newChannelNumber,"Channel number still set correctly");

}

void testBuffer() {
    testBufferRadio();
    // testBufferSerial();
}

void testBufferSerial() {
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

void testBufferRadio() {
    testBufferRadioSetup();

    // testBufferRadioAddData();
    // testBufferRadioClean();
    // testBufferRadioHasData();
    testBufferRadioProcessPacket();
    // testBufferRadioReset();
}

void testBufferRadioSetup() {
    radio.currentRadioBuffer = radio.bufferRadio;
}

void testBufferRadioAddData() {
    test.describe("bufferRadioAddData");

    char buffer[] = "AJ Keller is the best programmer";
    int expectedLength = 32; // Then length of the above buffer

    test.assertEqualBoolean(radio.bufferRadioAddData(radio.currentRadioBuffer, (char *)buffer, expectedLength, false),true,"should be able to add buffer to radioBuf");
    test.assertEqualBoolean(radio.currentRadioBuffer->gotAllPackets,false,"should not have all the packets");
    test.assertEqualInt(radio.currentRadioBuffer->positionWrite,expectedLength,"should move positionWrite by 32");
    for (int i = 0; i < expectedLength; i++) {
        test.assertEqualChar(radio.currentRadioBuffer->data[i],buffer[i], "Char is correct");
    }

    // Reset buffer
    radio.currentRadioBuffer->positionWrite = 0;

    // Test how this will work in normal operations, i.e. ignoring the byte id
    test.assertEqualBoolean(radio.bufferRadioAddData(radio.currentRadioBuffer, buffer+1, expectedLength-1, true),true,"should be able to add buffer to radioBuf");
    test.assertEqualBoolean(radio.currentRadioBuffer->gotAllPackets,true,"should be able to set gotAllPackets to true");
    test.assertEqualInt(radio.currentRadioBuffer->positionWrite,expectedLength - 1,"should set the positionWrite to 31");

    for (int i = 1; i < expectedLength; i++) {
        // Verify that we have a missing first char and off by one offset on the
        //  index.
        test.assertEqualChar(radio.currentRadioBuffer->data[i-1],buffer[i], "Char is correct");
    }
}

void testBufferRadioClean() {
    test.describe("bufferRadioClean");

    for (int i = 0; i < OPENBCI_BUFFER_LENGTH_MULTI; i++) {
        radio.currentRadioBuffer->data[i] = 1;
    }

    // Call the function under test
    radio.bufferRadioClean(radio.currentRadioBuffer);

    // Should fill the array with all zeros
    boolean allZeros = true;
    for (int j = 0; j < OPENBCI_BUFFER_LENGTH_MULTI; j++) {
        if (radio.currentRadioBuffer->data[j] != 0) {
            allZeros = false;
        }
    }
    test.assertEqualBoolean(true,allZeros,"should set all values to zero");
}

void testBufferRadioHasData() {
    test.describe("bufferRadioHasData");

    radio.currentRadioBuffer->positionWrite = 0;

    // Don't add any data
    test.assertEqualBoolean(radio.bufferRadioHasData(radio.currentRadioBuffer),false,"should have no data at first");
    // Add some data
    radio.currentRadioBuffer->positionWrite = 69;
    // Verify!
    test.assertEqualBoolean(radio.bufferRadioHasData(radio.currentRadioBuffer),true,"should have data after moving positionWrite");
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////


void testBufferRadioProcessPacket() {
    test.describe("bufferRadioProcessPacket");

    char buffer32[] = " AJ Keller is da best programmer";
    char bufferCali[] = " caliLucyMaggie";
    char bufferTaco[] = " taco";

    int buffer32Length = 32;
    int bufferCaliLength = 15;
    int bufferTacoLength = 5;

    // Last packet
    char byteId = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    // Store that byteId
    bufferTaco[0] = byteId;
    //      Current buffer has no data
    radio.bufferRadioReset(radio.bufferRadio);
    radio.bufferRadioReset(radio.bufferRadio + 1);
    radio.currentRadioBuffer = radio.bufferRadio;
    //          Take it! Mark Last
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_LAST,"should add to radio buffer 1");
    test.assertEqualBoolean(radio.currentRadioBuffer->gotAllPackets,true,"should be able to set gotAllPackets to true");
    test.assertEqualInt(radio.currentRadioBuffer->positionWrite,bufferTacoLength - 1,"should set the positionWrite to 4");
    for (int i = 1; i < bufferTacoLength; i++) {
        // Verify that we have a missing first char and off by one offset on the
        //  index.
        test.assertEqualChar(radio.currentRadioBuffer->data[i-1],bufferTaco[i], "Char is correct");
    }
    // # CLEANUP
    radio.bufferRadioReset(radio.bufferRadio);
    radio.bufferRadioReset(radio.bufferRadio + 1);
    radio.currentRadioBuffer = radio.bufferRadio;
    //      Current buffer has data
    char byteId1 = radio.byteIdMake(false,1,(char *)buffer32 + 1, buffer32Length - 1);
    char byteId2 = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    // Store that byteId
    buffer32[0] = byteId1;
    bufferTaco[0] = byteId2;
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)buffer32, buffer32Length),OPENBCI_PROCESS_RADIO_LAST_NOT,"should add not the last packet");
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_LAST,"should add the last packet");
    test.assertEqualBoolean(radio.currentRadioBuffer->gotAllPackets,true,"should be able to set gotAllPackets to true");
    test.assertEqualInt(radio.currentRadioBuffer->positionWrite,(bufferTacoLength + buffer32Length) - 2,"should set the positionWrite to size of both packets");
    for (int i = 1; i < buffer32Length; i++) {
        // Verify that we have a missing first char and off by one offset on the
        //  index.
        test.assertEqualChar(radio.currentRadioBuffer->data[i-1],buffer32[i], "Char is correct");
    }
    for (int i = buffer32Length; i < buffer32Length + bufferTacoLength; i++) {
        // Verify that we have a missing first char and off by one offset on the
        //  index.
        test.assertEqualChar(radio.currentRadioBuffer->data[i-1],bufferTaco[i - buffer32Length + 1], "Char is correct");
    }

    // Last packet
    byteId = radio.byteIdMake(false,0,(char *)bufferCali + 1, bufferCaliLength - 1);
    // Store that byteId
    bufferCali[0] = byteId;
    //      Current buffer has data
    // Don't do any clean up
    //          Current buffer has all packets
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferCali, bufferCaliLength),OPENBCI_PROCESS_RADIO_LAST,"should switch add the last packet");

}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

void testBufferRadioReset() {
    // Test the reset functions
    test.describe("bufferRadioReset");

    radio.currentRadioBuffer->flushing = true;
    radio.currentRadioBuffer->gotAllPackets = true;
    radio.currentRadioBuffer->positionWrite = 60;
    radio.currentRadioBuffer->previousPacketNumber = 3;

    // Reset the flags
    radio.bufferRadioReset(radio.currentRadioBuffer);

    // Verify they got Reset
    test.assertEqualBoolean(radio.currentRadioBuffer->flushing,false,"should set flushing to false");
    test.assertEqualBoolean(radio.currentRadioBuffer->gotAllPackets,false,"should set got all packets to false");
    test.assertEqualInt(radio.currentRadioBuffer->positionWrite,0,"should set positionWrite to 0");
    test.assertEqualInt(radio.currentRadioBuffer->previousPacketNumber,0,"should set previousPacketNumber to 0");
}
