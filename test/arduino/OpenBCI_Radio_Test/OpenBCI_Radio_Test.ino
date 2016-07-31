#include <RFduinoGZLL.h>
#include "OpenBCI_Radios.h"
#include "PTW-Arduino-Assert.h"

int ledPin = 2;

void setup() {
    pinMode(ledPin,OUTPUT);
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
    testByteId();
    testOutput();
    testBuffer();
    testNonVolatileFunctions();

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

    radio.bufferSerialReset(12);

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
    test.assertBoolean(radio.flashNonVolatileMemory(),true,"Erase page of memory");
    // The channel needs to be set
    test.assertBoolean(radio.needToSetChannelNumber(),true,"Need to set channel number");
    // The poll time should also need to be set
    test.assertBoolean(radio.needToSetPollTime(),true,"Need to set the poll time.");

    test.describe("setPollTime");
    uint32_t expectedPollTime = 200;
    // Set the poll time to new poll time
    test.assertBoolean(radio.setPollTime(expectedPollTime),true,"Able to set poll time");
    // Verify the poll time is set to the new poll time
    test.assertEqualInt((int)radio.getPollTime(),(int)expectedPollTime,"Poll time set correctly");
    // Verify the poll time does not have to be set
    test.assertBoolean(radio.needToSetPollTime(),false,"Don't need to set poll time");
    // Reset the poll time to default
    test.assertBoolean(radio.revertToDefaultPollTime(),true,"Reset poll time to default");
    // Verify the poll time is set to default
    test.assertEqualInt((int)radio.getPollTime(),(int)OPENBCI_TIMEOUT_PACKET_POLL_MS,"Poll time is set to define");
    // Verify the poll time does not have to be set
    test.assertBoolean(radio.needToSetPollTime(),false,"Don't need to set poll time");

    test.describe("setChannelNumber");
    uint32_t newChannelNumber = 10;
    // Set channel number to new channel number
    test.assertBoolean(radio.setChannelNumber(newChannelNumber),true,"Channel set");
    // Verify the channel number has been set to new channel number
    test.assertEqualInt((int)radio.getChannelNumber(),(int)newChannelNumber,"Channel set correctly");
    // Verify the channel number does not have to be set
    test.assertBoolean(radio.needToSetChannelNumber(),false,"Don't need to set the channel number");
    // Verify the poll time is still set to define
    test.assertEqualInt((int)radio.getPollTime(),(int)OPENBCI_TIMEOUT_PACKET_POLL_MS,"Poll time is still set");

    test.describe("setPollAndChan");
    // Set the channel to a new channel number lower than the first
    newChannelNumber = 2;
    test.assertBoolean(radio.setChannelNumber(newChannelNumber),true,"Newer channel set");
    // Verify the channel number has been set to new channel number
    test.assertEqualInt((int)radio.getChannelNumber(),(int)newChannelNumber,"Newer channel set correctly");
    // Verify the channel number does not have to be set
    test.assertBoolean(radio.needToSetChannelNumber(),false,"Still don't need to set the channel number");
    // Verify the poll time is still set to define
    test.assertEqualInt((int)radio.getPollTime(),(int)OPENBCI_TIMEOUT_PACKET_POLL_MS,"Poll time is still set");
    // Set the poll time to a new poll time
    expectedPollTime = 100;
    test.assertBoolean(radio.setPollTime(expectedPollTime),true,"Set poll time");
    // Verify the poll time is set to the new poll time
    test.assertEqualInt((int)radio.getPollTime(),(int)expectedPollTime,"Poll time set correctly");
    // Verify the channel number is still set
    test.assertEqualInt((int)radio.getChannelNumber(),(int)newChannelNumber,"Channel number still set correctly");

}

void testBuffer() {
    testBufferRadio();
    testBufferSerial();
    testBufferStream();
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
    testBufferRadioAddData();
    testBufferRadioClean();
    testBufferRadioHasData();
    testBufferRadioProcessPacket();
    testBufferRadioReadyForNewPage();
    testBufferRadioReset();
    testBufferRadioSwitchToOtherBuffer();
}

void testBufferRadioSetup() {
    radio.currentRadioBuffer = radio.bufferRadio;
}

void testBufferRadioAddData() {
    test.describe("bufferRadioAddData");

    char buffer[] = "AJ Keller is the best programmer";
    int expectedLength = 32; // Then length of the above buffer

    test.assertBoolean(radio.bufferRadioAddData(radio.currentRadioBuffer, (char *)buffer, expectedLength, false),true,"should be able to add buffer to radioBuf");
    test.assertBoolean(radio.currentRadioBuffer->gotAllPackets,false,"should not have all the packets");
    test.assertEqualInt(radio.currentRadioBuffer->positionWrite,expectedLength,"should move positionWrite by 32");
    test.assertEqualBuffer(radio.currentRadioBuffer->data,buffer,expectedLength,"should add the whole buffer");

    // Reset buffer
    radio.currentRadioBuffer->positionWrite = 0;

    // Test how this will work in normal operations, i.e. ignoring the byte id
    test.assertBoolean(radio.bufferRadioAddData(radio.currentRadioBuffer, buffer + 1, expectedLength - 1, true),true,"should be able to add buffer to radioBuf");
    test.assertBoolean(radio.currentRadioBuffer->gotAllPackets,true,"should be able to set gotAllPackets to true");
    test.assertEqualInt(radio.currentRadioBuffer->positionWrite,expectedLength - 1,"should set the positionWrite to 31");
    test.assertEqualBuffer(radio.currentRadioBuffer->data,buffer + 1,expectedLength - 1,"should add the whole buffer");
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
    test.assertBoolean(true,allZeros,"should set all values to zero");
}

void testBufferRadioHasData() {
    test.describe("bufferRadioHasData");

    radio.currentRadioBuffer->positionWrite = 0;

    // Don't add any data
    test.assertBoolean(radio.bufferRadioHasData(radio.currentRadioBuffer),false,"should have no data at first");
    // Add some data
    radio.currentRadioBuffer->positionWrite = 69;
    // Verify!
    test.assertBoolean(radio.bufferRadioHasData(radio.currentRadioBuffer),true,"should have data after moving positionWrite");
}

void testBufferRadioProcessPacket() {

    test.describe("bufferRadioProcessPacket");

    testBufferRadio_OPENBCI_PROCESS_RADIO_PASS_LAST_SINGLE();

    testBufferRadio_OPENBCI_PROCESS_RADIO_PASS_LAST_MULTI();

    testBufferRadio_OPENBCI_PROCESS_RADIO_PASS_SWITCH_LAST();

    testBufferRadio_OPENBCI_PROCESS_RADIO_FAIL_SWITCH_LAST();

    testBufferRadio_OPENBCI_PROCESS_RADIO_FAIL_SWITCH_NOT_LAST();

    testBufferRadio_OPENBCI_PROCESS_RADIO_PASS_SWITCH_NOT_LAST();

    testBufferRadio_OPENBCI_PROCESS_RADIO_FAIL_MISSED_LAST();

    testBufferRadio_OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_MIDDLE();

    testBufferRadio_OPENBCI_PROCESS_RADIO_FAIL_MISSED_NOT_LAST();

}

void testBufferRadio_OPENBCI_PROCESS_RADIO_PASS_LAST_SINGLE() {

    char bufferTaco[] = " taco";
    int bufferTacoLength = 5;

    testBufferRadioCleanUp();
    test.detail("OPENBCI_PROCESS_RADIO_PASS_LAST_SINGLE");
    bufferTaco[0] = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    // Last packet
    //      Current buffer has no data
    //          Take it! Mark Last
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_PASS_LAST_SINGLE,"should add to radio buffer 1", __LINE__);
    test.assertBoolean(radio.currentRadioBuffer->gotAllPackets,true,"should be able to set gotAllPackets to true", __LINE__);
    test.assertEqualInt(radio.currentRadioBuffer->positionWrite,bufferTacoLength - 1,"should set the positionWrite to 4", __LINE__);
    test.assertEqualBuffer(radio.currentRadioBuffer->data,bufferTaco + 1, bufferTacoLength - 1, "should have the taco buffer loaded into the first buffer");

    // Also verify that the buffer was loaded into the correct buffer
    test.assertBoolean(radio.bufferRadio->gotAllPackets,true,"should be able to set gotAllPackets to true");
    test.assertEqualInt(radio.bufferRadio->positionWrite,bufferTacoLength - 1,"should set the positionWrite to 4");
    test.assertEqualBuffer(radio.bufferRadio->data,bufferTaco + 1, bufferTacoLength - 1, "currentRadioBuffer should have the taco buffer loaded into it");

}

void testBufferRadio_OPENBCI_PROCESS_RADIO_PASS_LAST_MULTI() {
    char buffer32[] = " AJ Keller is da best programmer";
    char bufferTaco[] = " taco";

    int buffer32Length = 32;
    int bufferTacoLength = 5;
    testBufferRadioCleanUp();
    test.detail("OPENBCI_PROCESS_RADIO_PASS_LAST_MULTI");
    test.it("should be able to add a multi page packet");
    buffer32[0] = radio.byteIdMake(false,1,(char *)buffer32 + 1, buffer32Length - 1);
    bufferTaco[0] = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)buffer32, buffer32Length),OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_FIRST,"should add not the last packet");
    // Not last packet
    //      Current buffer has data
    //          Current buffer does not have all packets
    //              Previous packet number == packetNumber + 1
    //                  Take it! Mark last.
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_PASS_LAST_MULTI,"should add the last packet", __LINE__);
    test.assertBoolean(radio.bufferRadio->gotAllPackets,true,"should set gotAllPackets to true on first buffer", __LINE__);
    test.assertEqualInt(radio.bufferRadio->positionWrite,(bufferTacoLength + buffer32Length) - 2,"should set the positionWrite to size of both packets", __LINE__);
    test.assertEqualBuffer(radio.bufferRadio->data,buffer32 + 1, buffer32Length - 1, "buffer32 loaded into the correct postion in first buffer", __LINE__);
    test.assertEqualBuffer(radio.bufferRadio->data + (buffer32Length - 1),bufferTaco + 1, bufferTacoLength - 1, "taco buffer loaded into correct position in first buffer", __LINE__);
}

void testBufferRadio_OPENBCI_PROCESS_RADIO_PASS_SWITCH_LAST() {
    char buffer32[] = " AJ Keller is da best programmer";
    char bufferCali[] = " caliLucyMaggie";
    char bufferTaco[] = " taco";

    int buffer32Length = 32;
    int bufferCaliLength = 15;
    int bufferTacoLength = 5;
    testBufferRadioCleanUp();
    test.detail("OPENBCI_PROCESS_RADIO_PASS_SWITCH_LAST");
    // Need the first buffer to be full
    test.it("should switch to second buffer when first buffer is full and id last packet");
    buffer32[0] = radio.byteIdMake(false,1,(char *)buffer32 + 1, buffer32Length - 1);
    bufferTaco[0] = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)buffer32, buffer32Length),OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_FIRST,"should add not the last packet", __LINE__);
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_PASS_LAST_MULTI,"should add the last packet", __LINE__);

    bufferCali[0] = radio.byteIdMake(false,0,(char *)bufferCali + 1, bufferCaliLength - 1);
    // Last packet
    //      Current buffer has data
    //          Current buffer has all packets
    //              Can swtich to other buffer
    //                  Take it! Mark Last
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferCali, bufferCaliLength),OPENBCI_PROCESS_RADIO_PASS_SWITCH_LAST,"should switch and add the last packet", __LINE__);
    test.assertBoolean((radio.bufferRadio + 1)->gotAllPackets,true,"should set gotAllPackets to true for second buffer", __LINE__);
    test.assertEqualInt((radio.bufferRadio + 1)->positionWrite,bufferCaliLength - 1,"should set the positionWrite to size of cali buffer", __LINE__);
    test.assertEqualBuffer((radio.bufferRadio + 1)->data,bufferCali + 1, bufferCaliLength - 1, "should have loaded cali buffer in the second buffer correctly", __LINE__);

    // Verify that both of the buffers are full
    test.assertBoolean(radio.bufferRadio->gotAllPackets,true,"should still have a full first buffer after switch", __LINE__);
    test.assertEqualInt(radio.bufferRadio->positionWrite,(bufferTacoLength + buffer32Length) - 2,"first buffer should still have correct size", __LINE__);

    test.assertBoolean(radio.currentRadioBuffer->gotAllPackets,true,"should set got all packets full on currentRadioBuffer", __LINE__);
    test.assertEqualInt(radio.currentRadioBuffer->positionWrite,bufferCaliLength - 1,"should set positionWrite of currentRadioBuffer to that of the second buffer", __LINE__);
    test.assertEqualBuffer(radio.currentRadioBuffer->data,bufferCali + 1, bufferCaliLength - 1, "should have loaded cali buffer into the buffer currentRadioBuffer points to", __LINE__);


    // Do it again in reverse, where the second buffer is full
    // So clear the first buffer and point to the second
    test.it("should switch to first buffer when second buffer is full and id last packet");
    testBufferRadioCleanUp();
    radio.currentRadioBuffer = radio.bufferRadio + 1;
    bufferCali[0] = radio.byteIdMake(false,0,(char *)bufferCali + 1, bufferCaliLength - 1);
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferCali, bufferCaliLength),OPENBCI_PROCESS_RADIO_PASS_LAST_SINGLE,"should add the last packet", __LINE__);
    test.assertBoolean((radio.bufferRadio + 1)->gotAllPackets,true,"should set gotAllPackets to true for second buffer", __LINE__);
    test.assertEqualBuffer((radio.bufferRadio + 1)->data,bufferCali + 1, bufferCaliLength - 1, "should have loaded cali buffer in the second buffer correctly", __LINE__);
    test.assertEqualBuffer(radio.currentRadioBuffer->data,bufferCali + 1, bufferCaliLength - 1, "should have loaded cali buffer into the buffer currentRadioBuffer points to", __LINE__);

    // point to the second buffer
    bufferTaco[0] = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    // Last packet
    //      Current buffer has data
    //          Current buffer has all packets
    //              Can swtich to other buffer
    //                  Take it! Mark Last
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_PASS_SWITCH_LAST,"should add the last packet", __LINE__);
    test.assertBoolean(radio.bufferRadio->gotAllPackets,true,"should mark the first buffer full after switch", __LINE__);
    test.assertEqualBuffer(radio.bufferRadio->data,bufferTaco + 1, bufferTacoLength - 1, "should have the taco buffer loaded into the first buffer", __LINE__);
    test.assertEqualBuffer(radio.currentRadioBuffer->data,bufferTaco + 1, bufferTacoLength - 1, "should have the taco buffer loaded into currentRadioBuffer", __LINE__);
    // Verify the first buffer is still loaded with the cali buffer
    test.assertBoolean((radio.bufferRadio + 1)->gotAllPackets,true,"should set gotAllPackets to true for second buffer", __LINE__);
    test.assertEqualInt((radio.bufferRadio + 1)->positionWrite,bufferCaliLength - 1,"should set the positionWrite to size of cali buffer", __LINE__);
    test.assertEqualBuffer((radio.bufferRadio + 1)->data,bufferCali + 1, bufferCaliLength - 1, "should have loaded cali buffer in the second buffer correctly", __LINE__);

    test.it("should switch to second buffer when first is flushing and id last packet");
    // First buffer flushing, second empty
    testBufferRadioCleanUp();
    // Load the cali buffer into the first buffer
    bufferCali[0] = radio.byteIdMake(false,0,(char *)bufferCali + 1, bufferCaliLength - 1);
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferCali, bufferCaliLength),OPENBCI_PROCESS_RADIO_PASS_LAST_SINGLE,"should add the last packet", __LINE__);
    test.assertBoolean(radio.bufferRadio->gotAllPackets,true,"should set gotAllPackets to true for first buffer", __LINE__);
    test.assertEqualBuffer(radio.bufferRadio->data,bufferCali + 1, bufferCaliLength - 1, "should have loaded cali buffer in the first buffer correctly", __LINE__);
    test.assertEqualBuffer(radio.currentRadioBuffer->data,bufferCali + 1, bufferCaliLength - 1, "should have loaded cali buffer into the buffer currentRadioBuffer points to", __LINE__);
    // Test is to simulate the first one is being flushed as this new packet comes in
    // Set the first buffer to flushing
    radio.bufferRadio->flushing = true;
    bufferTaco[0] = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    // Last packet
    //      Current buffer has data
    //          Current buffer has all packets
    //              Can swtich to other buffer
    //                  Take it! Mark Last
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_PASS_SWITCH_LAST,"should switch and add the last packet when first is flushing", __LINE__);
    test.assertBoolean((radio.bufferRadio + 1)->gotAllPackets,true,"should mark the second buffer full", __LINE__);
    test.assertEqualBuffer((radio.bufferRadio + 1)->data,bufferTaco + 1, bufferTacoLength - 1, "should have the taco buffer loaded into the second buffer", __LINE__);
    test.assertEqualBuffer(radio.currentRadioBuffer->data,bufferTaco + 1, bufferTacoLength - 1, "should have the taco buffer loaded into currentRadioBuffer", __LINE__);
    // Verify the first buffer is still loaded with the cali buffer
    test.assertBoolean(radio.bufferRadio->flushing,true,"should have flushing true for first buffer", __LINE__);
    test.assertBoolean(radio.bufferRadio->gotAllPackets,true,"should still have gotAllPackets true for first buffer", __LINE__);
    test.assertEqualInt(radio.bufferRadio->positionWrite,bufferCaliLength - 1,"should still have positionWrite to size of cali buffer in buffer 1", __LINE__);
    test.assertEqualBuffer(radio.bufferRadio->data,bufferCali + 1, bufferCaliLength - 1, "should still have loaded cali buffer in the first buffer correctly", __LINE__);

    test.it("should switch to first buffer when second is flushing and id last packet");
    // Second buffer flushing, first empty
    testBufferRadioCleanUp();
    // Load the cali buffer into the second buffer
    radio.currentRadioBuffer = radio.bufferRadio + 1;
    bufferCali[0] = radio.byteIdMake(false,0,(char *)bufferCali + 1, bufferCaliLength - 1);
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferCali, bufferCaliLength),OPENBCI_PROCESS_RADIO_PASS_LAST_SINGLE,"should add the last packet", __LINE__);
    test.assertBoolean((radio.bufferRadio + 1)->gotAllPackets,true,"should set gotAllPackets to true for second buffer", __LINE__);
    test.assertEqualBuffer((radio.bufferRadio + 1)->data,bufferCali + 1, bufferCaliLength - 1, "should have loaded cali buffer in the second buffer correctly", __LINE__);
    test.assertEqualBuffer(radio.currentRadioBuffer->data,bufferCali + 1, bufferCaliLength - 1, "should have loaded cali buffer into the buffer currentRadioBuffer points to", __LINE__);
    // Test is to simulate the second one is being flushed as this new packet comes in
    // Set the second buffer to flushing
    (radio.bufferRadio + 1)->flushing = true;
    bufferTaco[0] = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    // Last packet
    //      Current buffer has data
    //          Current buffer has all packets
    //              Can swtich to other buffer
    //                  Take it! Mark Last
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_PASS_SWITCH_LAST,"should switch and add the last packet when second is flushing", __LINE__);
    test.assertBoolean(radio.bufferRadio->gotAllPackets,true,"should mark the first buffer full", __LINE__);
    test.assertEqualBuffer(radio.bufferRadio->data,bufferTaco + 1, bufferTacoLength - 1, "should have the taco buffer loaded into the first buffer", __LINE__);
    test.assertEqualBuffer(radio.currentRadioBuffer->data,bufferTaco + 1, bufferTacoLength - 1, "should have the taco buffer loaded into currentRadioBuffer", __LINE__);
    // Verify the first buffer is still loaded with the cali buffer
    test.assertBoolean((radio.bufferRadio + 1)->flushing,true,"should have flushing true for second buffer", __LINE__);
    test.assertBoolean((radio.bufferRadio + 1)->gotAllPackets,true,"should still have gotAllPackets true for second buffer", __LINE__);
    test.assertEqualInt((radio.bufferRadio + 1)->positionWrite,bufferCaliLength - 1,"should still have positionWrite to size of cali buffer in buffer 2", __LINE__);
    test.assertEqualBuffer((radio.bufferRadio + 1)->data,bufferCali + 1, bufferCaliLength - 1, "should still have loaded cali buffer in the first buffer correctly", __LINE__);
}

void testBufferRadio_OPENBCI_PROCESS_RADIO_FAIL_SWITCH_LAST() {
    char bufferCali[] = " caliLucyMaggie";
    char bufferTaco[] = " taco";
    char bufferTomatoPotato[] = " tomatoPotato";

    int bufferCaliLength = 15;
    int bufferTacoLength = 5;
    int bufferTomatoPotatoLength = 13;
    test.detail("OPENBCI_PROCESS_RADIO_FAIL_SWITCH_LAST");
    test.it("should not be able to switch to other buffer when both are full");
    testBufferRadioCleanUp();
    // Fill the two buffers
    bufferCali[0] = radio.byteIdMake(false,0,(char *)bufferCali + 1, bufferCaliLength - 1);
    bufferTaco[0] = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferCali, bufferCaliLength),OPENBCI_PROCESS_RADIO_PASS_LAST_SINGLE,"should add the last packet to buffer 1", __LINE__);
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_PASS_SWITCH_LAST,"should add the last packet to buffer 2", __LINE__);

    bufferTomatoPotato[0] = radio.byteIdMake(false,0,(char *)bufferTomatoPotato + 1, bufferTomatoPotatoLength - 1);
    // Last packet
    //      Current buffer has data
    //          Current buffer has all packets
    //              Cannot switch to other buffer
    //                  Reject it!
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferTomatoPotato, bufferTomatoPotatoLength),OPENBCI_PROCESS_RADIO_FAIL_SWITCH_LAST,"should reject the addition of this buffer", __LINE__);

    test.it("should not be able to switch to other buffer when the buffers are flushing");
    testBufferRadioCleanUp();
    radio.bufferRadio->flushing = true;
    (radio.bufferRadio+1)->flushing = true;
    bufferTomatoPotato[0] = radio.byteIdMake(false,0,(char *)bufferTomatoPotato + 1, bufferTomatoPotatoLength - 1);
    // Last packet
    //      Current buffer has data
    //          Current buffer has all packets
    //              Cannot switch to other buffer
    //                  Reject it!
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferTomatoPotato, bufferTomatoPotatoLength),OPENBCI_PROCESS_RADIO_FAIL_SWITCH_LAST,"should reject the addition of this buffer", __LINE__);

}

void testBufferRadio_OPENBCI_PROCESS_RADIO_FAIL_SWITCH_NOT_LAST() {
    char buffer32Hey[] = " hey there, my name is AJ Keller";
    char bufferCali[] = " caliLucyMaggie";
    char bufferTaco[] = " taco";

    int buffer32Length = 32;
    int bufferCaliLength = 15;
    int bufferTacoLength = 5;
    int bufferTomatoPotatoLength = 13;
    testBufferRadioCleanUp();
    test.detail("OPENBCI_PROCESS_RADIO_FAIL_SWITCH_NOT_LAST");
    // Fill both buffers
    bufferCali[0] = radio.byteIdMake(false,0,(char *)bufferCali + 1, bufferCaliLength - 1);
    bufferTaco[0] = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferCali, bufferCaliLength),OPENBCI_PROCESS_RADIO_PASS_LAST_SINGLE,"should add the last packet to buffer 1", __LINE__);
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_PASS_SWITCH_LAST,"should add the last packet to buffer 2", __LINE__);

    buffer32Hey[0] = radio.byteIdMake(false,1,(char *)buffer32Hey + 1, buffer32Length - 1);
    // Not last packet
    //      Current buffer has data
    //          Current buffer has all packets
    //              Cannot switch to other buffer
    //                  Reject it!
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)buffer32Hey, buffer32Length),OPENBCI_PROCESS_RADIO_FAIL_SWITCH_NOT_LAST,"should reject the addition of this multi page buffer", __LINE__);
    test.assertEqualBuffer(radio.bufferRadio->data,bufferCali + 1, bufferCaliLength - 1, "should still have loaded cali buffer in the first buffer correctly", __LINE__);
    test.assertEqualBuffer((radio.bufferRadio + 1)->data,bufferTaco + 1, bufferTacoLength - 1, "should still have loaded taco buffer in the second buffer correctly", __LINE__);

}

void testBufferRadio_OPENBCI_PROCESS_RADIO_PASS_SWITCH_NOT_LAST() {
    char bufferCali[] = " caliLucyMaggie";
    char buffer32Hey[] = " hey there, my name is AJ Keller";
    int bufferCaliLength = 15;
    int buffer32Length = 32;

    test.detail("OPENBCI_PROCESS_RADIO_PASS_SWITCH_NOT_LAST");
    testBufferRadioCleanUp();
    // Clear the first buffer, second buffer still has stuff in it
    radio.bufferRadioReset(radio.bufferRadio);
    // Make sure currentRadioBuffer pointer it on the second buffer
    radio.currentRadioBuffer = radio.bufferRadio + 1;
    bufferCali[0] = radio.byteIdMake(false,0,(char *)bufferCali + 1, bufferCaliLength - 1);
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferCali, bufferCaliLength),OPENBCI_PROCESS_RADIO_PASS_LAST_SINGLE,"should add the last packet to buffer 1", __LINE__);

    // Load it
    buffer32Hey[0] = radio.byteIdMake(false,2,(char *)buffer32Hey + 1, buffer32Length - 1);
    // Not last packet
    //      Current buffer has data
    //          Current buffer has all packets
    //              Can switch to other buffer
    //                  Take it! Not last
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)buffer32Hey, buffer32Length),OPENBCI_PROCESS_RADIO_PASS_SWITCH_NOT_LAST,"should reject the addition of this multi page buffer", __LINE__);
}

void testBufferRadio_OPENBCI_PROCESS_RADIO_FAIL_MISSED_LAST() {
    char buffer32[] = " AJ Keller is da best programmer";
    char bufferTaco[] = " taco";

    int buffer32Length = 32;
    int bufferTacoLength = 5;
    // # CLEANUP
    testBufferRadioCleanUp();

    test.detail("OPENBCI_PROCESS_RADIO_FAIL_MISSED_LAST");
    buffer32[0] = radio.byteIdMake(false,2,(char *)buffer32 + 1, buffer32Length - 1);
    bufferTaco[0] = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)buffer32, buffer32Length),OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_FIRST,"should add not the last packet", __LINE__);
    // Last packet
    //      Current buffer has data
    //          Current buffer does not have all packets
    //              Missed a packet
    //                  Reject it! Reset current buffer
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_FAIL_MISSED_LAST,"should not add the last packet because missed packet 1", __LINE__);
    test.assertBoolean(radio.bufferRadio->gotAllPackets,false,"should not have gotAllPackets", __LINE__);
    test.assertEqualInt(radio.bufferRadio->positionWrite,buffer32Length - 1,"should set the positionWrite to size of first packet", __LINE__);

}

void testBufferRadio_OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_MIDDLE() {
    char buffer32[] = " AJ Keller is da best programmer";
    char buffer32Hey[] = " hey there, my name is AJ Keller";
    int buffer32Length = 32;

    // # CLEANUP
    testBufferRadioCleanUp();
    test.detail("OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_MIDDLE");
    buffer32[0] = radio.byteIdMake(false,2,(char *)buffer32 + 1, buffer32Length - 1);
    buffer32Hey[0] = radio.byteIdMake(false,1,(char *)buffer32Hey + 1, buffer32Length - 1);
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)buffer32, buffer32Length),OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_FIRST,"should add first packet of several", __LINE__);
    // Not last packet
    //      Current buffer has data
    //          Current buffer does not have all packets
    //              Previous packet number == packetNumber + 1
    //                  Take it! Not last.
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)buffer32Hey, buffer32Length),OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_MIDDLE,"should add middle packet", __LINE__);
    test.assertBoolean(radio.bufferRadio->gotAllPackets,false,"should not have gotAllPackets", __LINE__);
    test.assertEqualInt(radio.bufferRadio->positionWrite,buffer32Length - 1 + buffer32Length - 1,"should set the positionWrite to size of first packet", __LINE__);

}

void testBufferRadio_OPENBCI_PROCESS_RADIO_FAIL_MISSED_NOT_LAST() {
    char buffer32[] = " AJ Keller is da best programmer";
    char buffer32Hey[] = " hey there, my name is AJ Keller";
    int buffer32Length = 32;

    // # CLEANUP
    testBufferRadioCleanUp();
    test.detail("OPENBCI_PROCESS_RADIO_FAIL_MISSED_NOT_LAST");
    buffer32[0] = radio.byteIdMake(false,3,(char *)buffer32 + 1, buffer32Length - 1);
    buffer32Hey[0] = radio.byteIdMake(false,1,(char *)buffer32Hey + 1, buffer32Length - 1);
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)buffer32, buffer32Length),OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_FIRST,"should add first packet of several", __LINE__);
    // Not last packet
    //      Current buffer has data
    //          Current buffer does not have all packets
    //              Missed a packet
    //                  Reject it! Reset current buffer
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)buffer32Hey, buffer32Length),OPENBCI_PROCESS_RADIO_FAIL_MISSED_NOT_LAST,"should not be able to add middle packet because not last", __LINE__);
    test.assertBoolean(radio.bufferRadio->gotAllPackets,false,"should not have gotAllPackets", __LINE__);
    test.assertEqualInt(radio.bufferRadio->positionWrite,buffer32Length - 1,"should set the positionWrite to size of first packet", __LINE__);

}

void testBufferRadioReadyForNewPage() {
    // # CLEANUP
    testBufferRadioCleanUp();

    test.describe("bufferRadioReadyForNewPage");

    char bufferTomatoPotato[] = " tomatoPotato";
    int bufferTomatoPotatoLength = 13;
    bufferTomatoPotato[0] = radio.byteIdMake(false,0,(char *)bufferTomatoPotato + 1, bufferTomatoPotatoLength - 1);


    test.it("works with clean state");
    test.assertBoolean(radio.bufferRadioReadyForNewPage(radio.bufferRadio),true,"should be ready to add new page in the first buffer", __LINE__);
    test.assertBoolean(radio.bufferRadioReadyForNewPage(radio.bufferRadio + 1),true,"should be ready to add new page in the second buffer", __LINE__);
    test.assertBoolean(radio.bufferRadioReadyForNewPage(radio.currentRadioBuffer),true,"should be ready to add new page in the currentRadioBuffer", __LINE__);

    // Add data to buffer 1
    test.it("cannot add a page to first buffer but can the second when filled");
    radio.bufferRadioAddData(radio.currentRadioBuffer, (char *)bufferTomatoPotato, bufferTomatoPotatoLength, true);
    test.assertBoolean(radio.bufferRadioReadyForNewPage(radio.bufferRadio),false,"should not be ready to add new page in the first buffer", __LINE__);
    test.assertBoolean(radio.bufferRadioReadyForNewPage(radio.bufferRadio + 1),true,"should be ready to add new page in the second buffer", __LINE__);
    test.assertBoolean(radio.bufferRadioReadyForNewPage(radio.currentRadioBuffer),false,"should not be ready to add new page in the currentRadioBuffer", __LINE__);

    // Increment the currentRadioBuffer pointer
    radio.currentRadioBuffer++;

    // Add data to buffer 2
    test.it("cannot add a page to either the first or second buffer when both are filled");
    radio.bufferRadioAddData(radio.currentRadioBuffer, (char *)bufferTomatoPotato, bufferTomatoPotatoLength, true);
    test.assertBoolean(radio.bufferRadioReadyForNewPage(radio.bufferRadio),false,"should not be ready to add new page in the first buffer", __LINE__);
    test.assertBoolean(radio.bufferRadioReadyForNewPage(radio.bufferRadio + 1),false,"should not be ready to add new page in the second buffer", __LINE__);
    test.assertBoolean(radio.bufferRadioReadyForNewPage(radio.currentRadioBuffer),false,"should not be ready to add new page in the currentRadioBuffer", __LINE__);

    // Clear the buffers
    // # CLEANUP
    testBufferRadioCleanUp();

    // Mark first buffer as flushing
    test.it("cannot add a page to first buffer but can the second when flushing");
    radio.bufferRadio->flushing = true;
    test.assertBoolean(radio.bufferRadioReadyForNewPage(radio.bufferRadio),false,"should not be ready to add new page in the first buffer", __LINE__);
    test.assertBoolean(radio.bufferRadioReadyForNewPage(radio.bufferRadio + 1),true,"should be ready to add new page in the second buffer", __LINE__);
    test.assertBoolean(radio.bufferRadioReadyForNewPage(radio.currentRadioBuffer),false,"should not be ready to add new page in the currentRadioBuffer", __LINE__);
    radio.bufferRadio->flushing = false;
    // Mark second buffer as flushing
    test.it("cannot add a page to second buffer but can the first when flushing");
    (radio.bufferRadio + 1)->flushing = true;
    test.assertBoolean(radio.bufferRadioReadyForNewPage(radio.bufferRadio),true,"should be ready to add new page in the first buffer", __LINE__);
    test.assertBoolean(radio.bufferRadioReadyForNewPage(radio.bufferRadio + 1),false,"should not be ready to add new page in the second buffer", __LINE__);
    test.assertBoolean(radio.bufferRadioReadyForNewPage(radio.currentRadioBuffer),true,"should be ready to add new page in the currentRadioBuffer", __LINE__);
    (radio.bufferRadio + 1)->flushing = true;

    // Both flushing
    test.it("cannot add a page to either when both flushing");
    radio.bufferRadio->flushing = true;
    (radio.bufferRadio + 1)->flushing = true;
    test.assertBoolean(radio.bufferRadioReadyForNewPage(radio.bufferRadio),false,"should not be ready to add new page in the first buffer", __LINE__);
    test.assertBoolean(radio.bufferRadioReadyForNewPage(radio.bufferRadio + 1),false,"should not be ready to add new page in the second buffer", __LINE__);
    test.assertBoolean(radio.bufferRadioReadyForNewPage(radio.currentRadioBuffer),false,"should not be ready to add new page in the currentRadioBuffer", __LINE__);

    // # CLEANUP
    testBufferRadioCleanUp();
}

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
    test.assertBoolean(radio.currentRadioBuffer->flushing,false,"should set flushing to false");
    test.assertBoolean(radio.currentRadioBuffer->gotAllPackets,false,"should set got all packets to false");
    test.assertEqualInt(radio.currentRadioBuffer->positionWrite,0,"should set positionWrite to 0");
    test.assertEqualInt(radio.currentRadioBuffer->previousPacketNumber,0,"should set previousPacketNumber to 0");
}

void testBufferRadioSwitchToOtherBuffer() {
    // # CLEANUP
    testBufferRadioCleanUp();

    test.describe("bufferRadioSwitchToOtherBuffer");

    char bufferTomatoPotato[] = " tomatoPotato";
    int bufferTomatoPotatoLength = 13;

    test.it("should return true if buffer 2 does not have data and should move the pointer");
    radio.currentRadioBuffer = radio.bufferRadio;
    test.assertBoolean(radio.bufferRadioSwitchToOtherBuffer(),true,"can switch to other empty buffer", __LINE__);
    test.assertBoolean(radio.currentRadioBuffer == (radio.bufferRadio + 1), true, "currentRadioBuffer points to second buffer", __LINE__);

    test.it("should return true if buffer 1 does not have data and should move the pointer");
    radio.currentRadioBuffer = radio.bufferRadio + 1;
    test.assertBoolean(radio.bufferRadioSwitchToOtherBuffer(),true,"can switch to other empty buffer", __LINE__);
    test.assertBoolean(radio.currentRadioBuffer == radio.bufferRadio, true, "currentRadioBuffer points to first buffer", __LINE__);

    // # CLEANUP
    testBufferRadioCleanUp();

    test.it("should return false when currently pointed at buf 1 and buf 2 has data");
    radio.currentRadioBuffer = radio.bufferRadio;
    radio.bufferRadioAddData(radio.bufferRadio + 1, (char *)bufferTomatoPotato, bufferTomatoPotatoLength, true);
    test.assertBoolean(radio.bufferRadioSwitchToOtherBuffer(),false,"cannot switch to buffer with data", __LINE__);
    test.assertBoolean(radio.currentRadioBuffer == radio.bufferRadio, true, "currentRadioBuffer still points to first buffer", __LINE__);

    // # CLEANUP
    testBufferRadioCleanUp();

    test.it("should return false when currently pointed at buf 2 and buf 1 has data");
    radio.currentRadioBuffer = radio.bufferRadio + 1;
    radio.bufferRadioAddData(radio.bufferRadio, (char *)bufferTomatoPotato, bufferTomatoPotatoLength, true);
    test.assertBoolean(radio.bufferRadioSwitchToOtherBuffer(),false,"cannot switch to buffer with data", __LINE__);
    test.assertBoolean(radio.currentRadioBuffer == radio.bufferRadio + 1, true, "currentRadioBuffer still points to second buffer", __LINE__);

    // # CLEANUP
    testBufferRadioCleanUp();

    test.it("should return false when both buffers have data");
    radio.bufferRadioAddData(radio.bufferRadio, (char *)bufferTomatoPotato, bufferTomatoPotatoLength, true);
    radio.bufferRadioAddData(radio.bufferRadio + 1, (char *)bufferTomatoPotato, bufferTomatoPotatoLength, true);
    test.assertBoolean(radio.bufferRadioSwitchToOtherBuffer(),false,"can't switch to second", __LINE__);
    radio.currentRadioBuffer++;
    test.assertBoolean(radio.bufferRadioSwitchToOtherBuffer(),false,"can't switch back to first", __LINE__);

    // # CLEANUP
    testBufferRadioCleanUp();

    test.it("should return false when other buffer is flushing");
    radio.bufferRadioAddData(radio.bufferRadio, (char *)bufferTomatoPotato, bufferTomatoPotatoLength, true);
    (radio.bufferRadio + 1)->flushing = true; // don't add data, just set it to flushing
    test.assertBoolean(radio.bufferRadioSwitchToOtherBuffer(),false,"can't switch to second because it's flushing", __LINE__);

    // # CLEANUP
    testBufferRadioCleanUp();

    test.it("should not switch when buffers are flushing");
    radio.bufferRadio->flushing = true; // don't add data, just set it to flushing
    (radio.bufferRadio + 1)->flushing = true; // don't add data, just set it to flushing
    test.assertBoolean(radio.bufferRadioSwitchToOtherBuffer(),false,"can't switch to any buffer", __LINE__);


}

void testBufferRadioCleanUp() {
    radio.bufferRadioReset(radio.bufferRadio);
    radio.bufferRadioReset(radio.bufferRadio + 1);
    radio.currentRadioBuffer = radio.bufferRadio;
}

void testBufferStream() {
    test.describe("testBufferStream");

    testBufferStreamAddData();
    testBufferStreamReadyForNewPacket();
    testBufferStreamReset();
    testBufferStreamStoreData();
}

void testBufferStreamAddData() {
    test.describe("bufferStreamAddData");
    char buffer32[] = " AJ Keller is da best programmer";
    char buffer32Hey[] = " hey there, my name is AJ Keller";
    char buffer32Q[] = " so you want to be a programmer?";
    char buffer32At[] = " if so you should use auto tests";
    int buffer32Length = 32;
    int packetType1 = 0x00;
    int packetType2 = 0x00;
    int packetType3 = 0x00;

    test.it("should be able to add a packet to the first streamPacketBuffer");
    testBufferStreamCleanUp();
    packetType1 = 0x01;
    buffer32[0] = radio.byteIdMake(true,packetType1,(char *)buffer32 + 1, buffer32Length - 1);
    test.assertBoolean(radio.bufferStreamAddData((char *)buffer32),true,"should add the stream packet",__LINE__);
    test.assertEqualBuffer(radio.streamPacketBuffer->data,buffer32 + 1, buffer32Length - 1, "buffer32 loaded into the correct postion in first buffer", __LINE__);
    test.assertEqualInt(radio.streamPacketBuffer->bytesIn, buffer32Length - 1, "should add 31 bytes to the stream packet buffer", __LINE__);
    test.assertEqualByte(radio.streamPacketBuffer->typeByte, packetType1 | OPENBCI_STREAM_BYTE_STOP, "should set the type byte to 0xCX where X is the packetType",__LINE__);

    test.it("should be able to add a packet to the second streamPacketBuffer when first is not ready for a new packet");
    testBufferStreamCleanUp();
    packetType1 = 0x01;
    packetType2 = 0x02;
    buffer32[0] = radio.byteIdMake(true,packetType1,(char *)buffer32 + 1, buffer32Length - 1);
    buffer32Hey[0] = radio.byteIdMake(true,packetType2,(char *)buffer32Hey + 1, buffer32Length - 1);

    test.assertBoolean(radio.bufferStreamAddData((char *)buffer32),true,"should add the first stream packet",__LINE__);
    test.assertBoolean(radio.bufferStreamAddData((char *)buffer32Hey),true,"should add the second stream packet",__LINE__);
    test.assertEqualBuffer(radio.streamPacketBuffer->data,buffer32 + 1, buffer32Length - 1, "buffer32 loaded into the correct postion in first buffer", __LINE__);
    test.assertEqualInt(radio.streamPacketBuffer->bytesIn, buffer32Length - 1, "should add 31 bytes to the first stream packet buffer", __LINE__);
    test.assertEqualByte(radio.streamPacketBuffer->typeByte, packetType1 | OPENBCI_STREAM_BYTE_STOP, "should set the type byte to 0xCX where X is the packetType",__LINE__);
    test.assertEqualBuffer((radio.streamPacketBuffer + 1)->data,buffer32Hey + 1, buffer32Length - 1, "buffer32 loaded into the correct postion in second buffer", __LINE__);
    test.assertEqualInt((radio.streamPacketBuffer + 1)->bytesIn, buffer32Length - 1, "should add 31 bytes to the second stream packet buffer", __LINE__);
    test.assertEqualByte((radio.streamPacketBuffer + 1)->typeByte, packetType2 | OPENBCI_STREAM_BYTE_STOP, "should set the type byte to 0xCX where X is the packetType",__LINE__);

    test.it("should be able to add a packet to the third streamPacketBuffer when first and second are not ready for a new packet");
    testBufferStreamCleanUp();
    packetType1 = 0x01;
    packetType2 = 0x02;
    packetType3 = 0x03;
    buffer32[0] = radio.byteIdMake(true,packetType1,(char *)buffer32 + 1, buffer32Length - 1);
    buffer32Hey[0] = radio.byteIdMake(true,packetType2,(char *)buffer32Hey + 1, buffer32Length - 1);
    buffer32Q[0] = radio.byteIdMake(true,packetType3,(char *)buffer32Q + 1, buffer32Length - 1);

    test.assertBoolean(radio.bufferStreamAddData((char *)buffer32),true,"should add the first stream packet",__LINE__);
    test.assertBoolean(radio.bufferStreamAddData((char *)buffer32Hey),true,"should add the second stream packet",__LINE__);
    test.assertBoolean(radio.bufferStreamAddData((char *)buffer32Q),true,"should add the third stream packet",__LINE__);
    test.assertEqualBuffer(radio.streamPacketBuffer->data,buffer32 + 1, buffer32Length - 1, "buffer32 loaded into the correct postion in first buffer", __LINE__);
    test.assertEqualInt(radio.streamPacketBuffer->bytesIn, buffer32Length - 1, "should add 31 bytes to the first stream packet buffer", __LINE__);
    test.assertEqualByte(radio.streamPacketBuffer->typeByte, packetType1 | OPENBCI_STREAM_BYTE_STOP, "should set the type byte to 0xCX where X is the packetType",__LINE__);
    test.assertEqualBuffer((radio.streamPacketBuffer + 1)->data,buffer32Hey + 1, buffer32Length - 1, "buffer32 loaded into the correct postion in second buffer", __LINE__);
    test.assertEqualInt((radio.streamPacketBuffer + 1)->bytesIn, buffer32Length - 1, "should add 31 bytes to the second stream packet buffer", __LINE__);
    test.assertEqualByte((radio.streamPacketBuffer + 1)->typeByte, packetType2 | OPENBCI_STREAM_BYTE_STOP, "should set the type byte to 0xCX where X is the packetType",__LINE__);
    test.assertEqualBuffer((radio.streamPacketBuffer + 2)->data,buffer32Q + 1, buffer32Length - 1, "buffer32 loaded into the correct postion in third buffer", __LINE__);
    test.assertEqualInt((radio.streamPacketBuffer + 2)->bytesIn, buffer32Length - 1, "should add 31 bytes to the third stream packet buffer", __LINE__);
    test.assertEqualByte((radio.streamPacketBuffer + 2)->typeByte, packetType3 | OPENBCI_STREAM_BYTE_STOP, "should set the type byte to 0xCX where X is the packetType",__LINE__);

    test.it("should not be able to add a packet when the first, second and third are not ready for a new packet");
    testBufferStreamCleanUp();
    packetType1 = 0x01;
    packetType2 = 0x02;
    packetType3 = 0x03;
    buffer32[0] = radio.byteIdMake(true,packetType1,(char *)buffer32 + 1, buffer32Length - 1);
    buffer32Hey[0] = radio.byteIdMake(true,packetType2,(char *)buffer32Hey + 1, buffer32Length - 1);
    buffer32Q[0] = radio.byteIdMake(true,packetType3,(char *)buffer32Q + 1, buffer32Length - 1);
    buffer32At[0] = radio.byteIdMake(true,packetType3 + 1,(char *)buffer32At + 1, buffer32Length - 1);

    test.assertBoolean(radio.bufferStreamAddData((char *)buffer32),true,"should add the first stream packet",__LINE__);
    test.assertBoolean(radio.bufferStreamAddData((char *)buffer32Hey),true,"should add the second stream packet",__LINE__);
    test.assertBoolean(radio.bufferStreamAddData((char *)buffer32Q),true,"should add the third stream packet",__LINE__);
    test.assertBoolean(radio.bufferStreamAddData((char *)buffer32At),false,"should not be able to add the forth stream packet",__LINE__);
    test.assertEqualBuffer(radio.streamPacketBuffer->data,buffer32 + 1, buffer32Length - 1, "buffer32 loaded into the correct postion in first buffer", __LINE__);
    test.assertEqualInt(radio.streamPacketBuffer->bytesIn, buffer32Length - 1, "should add 31 bytes to the first stream packet buffer", __LINE__);
    test.assertEqualByte(radio.streamPacketBuffer->typeByte, packetType1 | OPENBCI_STREAM_BYTE_STOP, "should set the type byte to 0xCX where X is the packetType",__LINE__);
    test.assertEqualBuffer((radio.streamPacketBuffer + 1)->data,buffer32Hey + 1, buffer32Length - 1, "buffer32 loaded into the correct postion in second buffer", __LINE__);
    test.assertEqualInt((radio.streamPacketBuffer + 1)->bytesIn, buffer32Length - 1, "should add 31 bytes to the second stream packet buffer", __LINE__);
    test.assertEqualByte((radio.streamPacketBuffer + 1)->typeByte, packetType2 | OPENBCI_STREAM_BYTE_STOP, "should set the type byte to 0xCX where X is the packetType",__LINE__);
    test.assertEqualBuffer((radio.streamPacketBuffer + 2)->data,buffer32Q + 1, buffer32Length - 1, "buffer32 loaded into the correct postion in third buffer", __LINE__);
    test.assertEqualInt((radio.streamPacketBuffer + 2)->bytesIn, buffer32Length - 1, "should add 31 bytes to the third stream packet buffer", __LINE__);
    test.assertEqualByte((radio.streamPacketBuffer + 2)->typeByte, packetType3 | OPENBCI_STREAM_BYTE_STOP, "should set the type byte to 0xCX where X is the packetType",__LINE__);

    test.it("should be able to add a packet to the second streamPacketBuffer when first is flushing");
    testBufferStreamCleanUp();
    packetType2 = 0x02;
    buffer32Hey[0] = radio.byteIdMake(true,packetType2,(char *)buffer32Hey + 1, buffer32Length - 1);
    radio.streamPacketBuffer->flushing = true;
    test.assertBoolean(radio.bufferStreamAddData((char *)buffer32Hey),true,"should add the second stream packet",__LINE__);
    test.assertEqualBuffer((radio.streamPacketBuffer + 1)->data,buffer32Hey + 1, buffer32Length - 1, "buffer32 loaded into the correct postion in second buffer", __LINE__);
    test.assertEqualInt((radio.streamPacketBuffer + 1)->bytesIn, buffer32Length - 1, "should add 31 bytes to the second stream packet buffer", __LINE__);
    test.assertEqualByte((radio.streamPacketBuffer + 1)->typeByte, packetType2 | OPENBCI_STREAM_BYTE_STOP, "should set the type byte to 0xCX where X is the packetType",__LINE__);

}

void testBufferStreamCleanUp() {
    radio.bufferStreamReset(radio.streamPacketBuffer);
    radio.bufferStreamReset(radio.streamPacketBuffer + 1);
    radio.bufferStreamReset(radio.streamPacketBuffer + 2);
}

void testBufferStreamReadyForNewPacket() {
    test.describe("bufferStreamReadyForNewPacket");

    test.it("should be able to add a packet after reset");
    radio.bufferStreamReset(radio.streamPacketBuffer);
    test.assertBoolean(radio.bufferStreamReadyForNewPacket(radio.streamPacketBuffer),true,"should add buffer",__LINE__);

    test.it("should not be able to add a packet if flushing");
    radio.bufferStreamReset(radio.streamPacketBuffer);
    radio.streamPacketBuffer->flushing = true;
    test.assertBoolean(radio.bufferStreamReadyForNewPacket(radio.streamPacketBuffer),false,"should not add buffer if flushing",__LINE__);

    test.it("should not be able to add a packet if it has data in it");
    radio.bufferStreamReset(radio.streamPacketBuffer);
    radio.streamPacketBuffer->bytesIn = 31;
    test.assertBoolean(radio.bufferStreamReadyForNewPacket(radio.streamPacketBuffer),false,"should not add buffer if it has data in it",__LINE__);

}

void testBufferStreamReset() {
    test.describe("bufferStreamReset");

    test.it("should reset the first stream packet and the curStreamState to init");
    radio.streamPacketBuffer->flushing = true;
    radio.streamPacketBuffer->bytesIn = 32;
    radio.streamPacketBuffer->typeByte = 2;
    radio.streamPacketBuffer->state = radio.STREAM_STATE_READY;

    radio.bufferStreamReset();
    test.assertBoolean(radio.streamPacketBuffer->flushing,false,"should set flushing to false",__LINE__);
    test.assertEqualInt(radio.streamPacketBuffer->bytesIn,0,"should set bytesIn to 0",__LINE__);
    test.assertEqualInt(radio.streamPacketBuffer->typeByte,0,"should set typeByte to 0",__LINE__);
    test.assertEqualByte(radio.streamPacketBuffer->state, radio.STREAM_STATE_INIT, "should set stream state back to init",__LINE__);

    test.it("should be able to reset the second stream buffer");
    (radio.streamPacketBuffer + 1)->flushing = true;
    (radio.streamPacketBuffer + 1)->bytesIn = 32;
    (radio.streamPacketBuffer + 1)->typeByte = 2;

    radio.bufferStreamReset(radio.streamPacketBuffer + 1);
    test.assertBoolean((radio.streamPacketBuffer + 1)->flushing,false,"should set flushing to false",__LINE__);
    test.assertEqualInt((radio.streamPacketBuffer + 1)->bytesIn,0,"should set bytesIn to 0",__LINE__);
    test.assertEqualInt((radio.streamPacketBuffer + 1)->typeByte,0,"should set typeByte to 0",__LINE__);

}

void testBufferStreamStoreData() {
    test.describe("bufferStreamStoreData");
    char buffer32[] = " AJ Keller is da best programmer";
    int buffer32Length = 32;
    int packetType = 0;


    test.it("should be able to store a buffer in the stream packet");
    packetType = 0x06;
    radio.bufferStreamReset(radio.streamPacketBuffer);

    buffer32[0] = radio.byteIdMake(true,packetType,(char *)buffer32 + 1, buffer32Length - 1);
    radio.bufferStreamStoreData(radio.streamPacketBuffer,(char *)buffer32);
    test.assertEqualBuffer(radio.streamPacketBuffer->data,buffer32 + 1, buffer32Length - 1, "buffer32 loaded into the correct postion in first buffer", __LINE__);
    test.assertEqualInt(radio.streamPacketBuffer->bytesIn,buffer32Length - 1, "should store 31 bytes to the first streamPacketBuffer", __LINE__);
    test.assertEqualByte(radio.streamPacketBuffer->typeByte,packetType | OPENBCI_STREAM_BYTE_STOP,"should store the packetNumber OR'd with a stop byte", __LINE__);

}
