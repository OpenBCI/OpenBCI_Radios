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
    char buffer32Hey[] = " hey there, my name is AJ Keller";
    char bufferCali[] = " caliLucyMaggie";
    char bufferTaco[] = " taco";
    char bufferTomatoPotato[] = " tomatoPotato";

    int buffer32Length = 32;
    int bufferCaliLength = 15;
    int bufferTacoLength = 5;
    int bufferTomatoPotatoLength = 13;

    boolean allDataCorrect = true;

    // # CLEANUP
    testBufferRadioCleanUp();

    ////////////////////////////////////////////////////////////
    ////// Test: OPENBCI_PROCESS_RADIO_PASS_LAST_SINGLE
    ////////////////////////////////////////////////////////////
    bufferTaco[0] = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    // Last packet
    //      Current buffer has no data
    //          Take it! Mark Last
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_PASS_LAST_SINGLE,"should add to radio buffer 1");
    test.assertEqualBoolean(radio.currentRadioBuffer->gotAllPackets,true,"should be able to set gotAllPackets to true");
    test.assertEqualInt(radio.currentRadioBuffer->positionWrite,bufferTacoLength - 1,"should set the positionWrite to 4");
    allDataCorrect = true;
    for (int i = 1; i < bufferTacoLength; i++) {
        // Verify that we have a missing first char and off by one offset on the
        //  index.
        if (radio.currentRadioBuffer->data[i-1] != bufferTaco[i]) {
            allDataCorrect = false;
        }
    }
    test.assertEqualBoolean(allDataCorrect, true, "currentRadioBuffer should have the taco buffer loaded into it");
    // Also verify that the buffer was loaded into the correct buffer
    test.assertEqualBoolean(radio.bufferRadio->gotAllPackets,true,"should be able to set gotAllPackets to true");
    test.assertEqualInt(radio.bufferRadio->positionWrite,bufferTacoLength - 1,"should set the positionWrite to 4");
    allDataCorrect = true;
    for (int i = 1; i < bufferTacoLength; i++) {
        // Verify that we have a missing first char and off by one offset on the
        //  index.
        if (radio.bufferRadio->data[i-1] != bufferTaco[i]) {
            allDataCorrect = false;
        }
    }
    test.assertEqualBoolean(allDataCorrect, true, "currentRadioBuffer should have the taco buffer loaded into it");


    // # CLEANUP
    testBufferRadioCleanUp();

    ////////////////////////////////////////////////////////////
    ////// Test: OPENBCI_PROCESS_RADIO_PASS_LAST_MULTI
    ////////////////////////////////////////////////////////////
    buffer32[0] = radio.byteIdMake(false,1,(char *)buffer32 + 1, buffer32Length - 1);
    bufferTaco[0] = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)buffer32, buffer32Length),OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_FIRST,"should add not the last packet");
    // Not last packet
    //      Current buffer has data
    //          Current buffer does not have all packets
    //              Previous packet number == packetNumber + 1
    //                  Take it! Mark last.
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_PASS_LAST_MULTI,"should add the last packet");
    test.assertEqualBoolean(radio.bufferRadio->gotAllPackets,true,"should set gotAllPackets to true on first buffer");
    test.assertEqualInt(radio.bufferRadio->positionWrite,(bufferTacoLength + buffer32Length) - 2,"should set the positionWrite to size of both packets");

    allDataCorrect = true;
    for (int i = 1; i < buffer32Length; i++) {
        // Verify that we have a missing first char and off by one offset on the
        //  index.
        if (radio.bufferRadio->data[i-1] != buffer32[i]) {
            allDataCorrect = false;
        }
    }
    test.assertEqualBoolean(allDataCorrect, true, "buffer32 loaded into the correct postion in first buffer");
    allDataCorrect = true;
    for (int i = buffer32Length; i < buffer32Length + bufferTacoLength; i++) {
        // Verify that we have a missing first char and off by one offset on the
        //  index.
        if (radio.bufferRadio->data[i-1] != bufferTaco[i - buffer32Length + 1]) {
            allDataCorrect = false;
        }
    }
    test.assertEqualBoolean(allDataCorrect,true, "taco buffer loaded into correct position in first buffer");

    // # CLEANUP
    testBufferRadioCleanUp();

    ////////////////////////////////////////////////////////////
    ////// Test: OPENBCI_PROCESS_RADIO_PASS_SWITCH_LAST
    ////////////////////////////////////////////////////////////
    // Need the first buffer to be full
    buffer32[0] = radio.byteIdMake(false,1,(char *)buffer32 + 1, buffer32Length - 1);
    bufferTaco[0] = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)buffer32, buffer32Length),OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_FIRST,"should add not the last packet");
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_PASS_LAST_MULTI,"should add the last packet");

    bufferCali[0] = radio.byteIdMake(false,0,(char *)bufferCali + 1, bufferCaliLength - 1);
    // Last packet
    //      Current buffer has data
    //          Current buffer has all packets
    //              Can swtich to other buffer
    //                  Take it! Mark Last
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferCali, bufferCaliLength),OPENBCI_PROCESS_RADIO_PASS_SWITCH_LAST,"should switch and add the last packet");
    test.assertEqualBoolean((radio.bufferRadio + 1)->gotAllPackets,true,"should set gotAllPackets to true for second buffer");
    test.assertEqualInt((radio.bufferRadio + 1)->positionWrite,bufferCaliLength - 1,"should set the positionWrite to size of cali buffer");
    allDataCorrect = true;
    for (int i = 1; i < bufferCaliLength; i++) {
        // Verify that we have a missing first char and off by one offset on the
        //  index.
        if ((radio.bufferRadio + 1)->data[i-1] != bufferCali[i]) {
            allDataCorrect = false;
        }
    }
    test.assertEqualBoolean(allDataCorrect, true, "should have loaded cali buffer in the second buffer correctly");
    // Verify that both of the buffers are full
    test.assertEqualBoolean(radio.bufferRadio->gotAllPackets,true,"should still have a full first buffer after switch");
    test.assertEqualInt(radio.bufferRadio->positionWrite,(bufferTacoLength + buffer32Length) - 2,"first buffer should still have correct size");

    test.assertEqualBoolean(radio.currentRadioBuffer->gotAllPackets,true,"should set got all packets full on currentRadioBuffer");
    test.assertEqualInt(radio.currentRadioBuffer->positionWrite,bufferCaliLength - 1,"should set positionWrite of currentRadioBuffer to that of the second buffer");
    allDataCorrect = true;
    for (int i = 1; i < bufferCaliLength; i++) {
        // Verify that we have a missing first char and off by one offset on the
        //  index.
        if (radio.currentRadioBuffer->data[i-1] != bufferCali[i]) {
            allDataCorrect = false;
        }
    }
    test.assertEqualBoolean(allDataCorrect, true, "currentRadioBuffer should have the cali buffer loaded into it");

    // Do it again in reverse, where the second buffer is full
    // So clear the first buffer and point to the second
    radio.bufferRadioReset(radio.bufferRadio);
    radio.currentRadioBuffer = radio.bufferRadio + 1;

    bufferTaco[0] = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    // Last packet
    //      Current buffer has data
    //          Current buffer has all packets
    //              Can swtich to other buffer
    //                  Take it! Mark Last
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_PASS_SWITCH_LAST,"should add the last packet");
    test.assertEqualBoolean(radio.bufferRadio->gotAllPackets,true,"should mark the first buffer full after switch");
    allDataCorrect = true;
    for (int i = 1; i < bufferTacoLength; i++) {
        // Verify that we have a missing first char and off by one offset on the
        //  index.
        if (radio.bufferRadio->data[i-1] != bufferTaco[i]) {
            allDataCorrect = false;
        }
    }
    test.assertEqualBoolean(allDataCorrect, true, "should have the taco buffer loaded into the first buffer");
    allDataCorrect = true;
    for (int i = 1; i < bufferTacoLength; i++) {
        // Verify that we have a missing first char and off by one offset on the
        //  index.
        if (radio.currentRadioBuffer->data[i-1] != bufferTaco[i]) {
            allDataCorrect = false;
        }
    }
    test.assertEqualBoolean(allDataCorrect, true, "should have the taco buffer loaded into currentRadioBuffer");

    test.assertEqualBoolean((radio.bufferRadio + 1)->gotAllPackets,true,"should set gotAllPackets to true for second buffer");
    test.assertEqualInt((radio.bufferRadio + 1)->positionWrite,bufferCaliLength - 1,"should set the positionWrite to size of cali buffer");
    allDataCorrect = true;
    for (int i = 1; i < bufferCaliLength; i++) {
        // Verify that we have a missing first char and off by one offset on the
        //  index.
        if ((radio.bufferRadio + 1)->data[i-1] != bufferCali[i]) {
            allDataCorrect = false;
        }
    }
    test.assertEqualBoolean(allDataCorrect, true, "should have loaded the cali buffer loaded into currentRadioBuffer");


    // Make one of the buffers flushing

    // # CLEANUP
    testBufferRadioCleanUp();

    // Have buffer 1 full
    bufferCali[0] = radio.byteIdMake(false,0,(char *)bufferCali + 1, bufferCaliLength - 1);
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferCali, bufferCaliLength),OPENBCI_PROCESS_RADIO_PASS_SWITCH_LAST,"should add the last packet to buffer 1");
    // Make buffer 1 be flushing
    radio.bufferRadio->flushing = true;

    bufferTaco[0] = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    // Last packet
    //      Current buffer has data
    //          Current buffer has all packets
    //              Can swtich to other buffer
    //                  Take it! Mark Last
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_PASS_SWITCH_LAST,"should add the last packet while other is flushing");

    // # CLEANUP
    testBufferRadioCleanUp();

    ////////////////////////////////////////////////////////////
    ////// Test: OPENBCI_PROCESS_RADIO_FAIL_SWITCH_LAST
    ////////////////////////////////////////////////////////////

    bufferCali[0] = radio.byteIdMake(false,0,(char *)bufferCali + 1, bufferCaliLength - 1);
    bufferTaco[0] = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferCali, bufferCaliLength),OPENBCI_PROCESS_RADIO_PASS_SWITCH_LAST,"should add the last packet to buffer 1");
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_PASS_LAST_MULTI,"should add the last packet to buffer 2");

    bufferTomatoPotato[0] = radio.byteIdMake(false,0,(char *)bufferTomatoPotato + 1, bufferTomatoPotatoLength - 1);
    // Last packet
    //      Current buffer has data
    //          Current buffer has all packets
    //              Cannot switch to other buffer
    //                  Reject it!
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferTomatoPotato, bufferTomatoPotatoLength),OPENBCI_PROCESS_RADIO_FAIL_SWITCH_LAST,"should reject the addition of this buffer");

    radio.bufferRadio->flushing = true;

    bufferTomatoPotato[0] = radio.byteIdMake(false,0,(char *)bufferTomatoPotato + 1, bufferTomatoPotatoLength - 1);
    // Last packet
    //      Current buffer has data
    //          Current buffer has all packets
    //              Cannot switch to other buffer
    //                  Reject it!
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferTomatoPotato, bufferTomatoPotatoLength),OPENBCI_PROCESS_RADIO_FAIL_SWITCH_LAST,"should reject the addition of this buffer");

    // # CLEANUP
    testBufferRadioCleanUp();

    ////////////////////////////////////////////////////////////
    ////// Test: OPENBCI_PROCESS_RADIO_FAIL_SWITCH_NOT_LAST
    ////////////////////////////////////////////////////////////
    // Fill both buffers
    bufferCali[0] = radio.byteIdMake(false,0,(char *)bufferCali + 1, bufferCaliLength - 1);
    bufferTaco[0] = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferCali, bufferCaliLength),OPENBCI_PROCESS_RADIO_PASS_SWITCH_LAST,"should add the last packet to buffer 1");
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_PASS_LAST_MULTI,"should add the last packet to buffer 2");

    buffer32Hey[0] = radio.byteIdMake(false,1,(char *)buffer32Hey + 1, buffer32Length - 1);
    // Not last packet
    //      Current buffer has data
    //          Current buffer has all packets
    //              Cannot switch to other buffer
    //                  Reject it!
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)buffer32Hey, buffer32Length),OPENBCI_PROCESS_RADIO_FAIL_SWITCH_NOT_LAST,"should reject the addition of this multi page buffer");
    allDataCorrect = true;
    for (int i = 1; i < bufferCaliLength; i++) {
        // Verify that we have a missing first char and off by one offset on the
        //  index.
        if ((radio.bufferRadio + 1)->data[i-1] != bufferCali[i]) {
            allDataCorrect = false;
        }
    }
    test.assertEqualBoolean(allDataCorrect, true, "should still have cali buffer loaded in the second buffer");
    // TODO: Add test for taco buffer
    ////////////////////////////////////////////////////////////
    ////// Test: OPENBCI_PROCESS_RADIO_PASS_SWITCH_NOT_LAST
    ////////////////////////////////////////////////////////////
    // Clear the first buffer, second buffer still has stuff in it
    radio.bufferRadioReset(radio.bufferRadio);
    // Make sure currentRadioBuffer pointer it on the second buffer
    radio.currentRadioBuffer = radio.bufferRadio + 1;
    // Load it
    buffer32Hey[0] = radio.byteIdMake(false,2,(char *)buffer32Hey + 1, buffer32Length - 1);
    // Not last packet
    //      Current buffer has data
    //          Current buffer has all packets
    //              Can switch to other buffer
    //                  Take it! Not last
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)buffer32Hey, buffer32Length),OPENBCI_PROCESS_RADIO_PASS_SWITCH_NOT_LAST,"should reject the addition of this multi page buffer");

    // # CLEANUP
    testBufferRadioCleanUp();

    ////////////////////////////////////////////////////////////
    ////// Test: OPENBCI_PROCESS_RADIO_FAIL_MISSED_LAST
    ////////////////////////////////////////////////////////////
    buffer32[0] = radio.byteIdMake(false,2,(char *)buffer32 + 1, buffer32Length - 1);
    bufferTaco[0] = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)buffer32, buffer32Length),OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_FIRST,"should add not the last packet");
    // Last packet
    //      Current buffer has data
    //          Current buffer does not have all packets
    //              Missed a packet
    //                  Reject it! Reset current buffer
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_FAIL_MISSED_LAST,"should not add the last packet because missed packet 1");
    test.assertEqualBoolean(radio.bufferRadio->gotAllPackets,false,"should not have gotAllPackets");
    test.assertEqualInt(radio.bufferRadio->positionWrite,buffer32Length - 1,"should set the positionWrite to size of first packet");

    // # CLEANUP
    testBufferRadioCleanUp();

    ////////////////////////////////////////////////////////////
    ////// Test: OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_MIDDLE
    ////////////////////////////////////////////////////////////
    buffer32[0] = radio.byteIdMake(false,2,(char *)buffer32 + 1, buffer32Length - 1);
    buffer32Hey[0] = radio.byteIdMake(false,1,(char *)buffer32Hey + 1, buffer32Length - 1);
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)buffer32, buffer32Length),OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_FIRST,"should add first packet of several");
    // Not last packet
    //      Current buffer has data
    //          Current buffer does not have all packets
    //              Previous packet number == packetNumber + 1
    //                  Take it! Not last.
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)buffer32Hey, buffer32Length),OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_MIDDLE,"should add middle packet");
    test.assertEqualBoolean(radio.bufferRadio->gotAllPackets,false,"should not have gotAllPackets");
    test.assertEqualInt(radio.bufferRadio->positionWrite,buffer32Length - 1,"should set the positionWrite to size of first packet");


    // # CLEANUP
    testBufferRadioCleanUp();

    ////////////////////////////////////////////////////////////
    ////// Test: OPENBCI_PROCESS_RADIO_FAIL_MISSED_NOT_LAST
    ////////////////////////////////////////////////////////////
    buffer32[0] = radio.byteIdMake(false,3,(char *)buffer32 + 1, buffer32Length - 1);
    buffer32Hey[0] = radio.byteIdMake(false,1,(char *)buffer32Hey + 1, buffer32Length - 1);
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)buffer32, buffer32Length),OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_FIRST,"should add first packet of several");
    // Not last packet
    //      Current buffer has data
    //          Current buffer does not have all packets
    //              Missed a packet
    //                  Reject it! Reset current buffer
    test.assertEqualByte(radio.bufferRadioProcessPacket((char *)buffer32Hey, buffer32Length),OPENBCI_PROCESS_RADIO_FAIL_MISSED_NOT_LAST,"should not be able to add middle packet because not last");
    test.assertEqualBoolean(radio.bufferRadio->gotAllPackets,false,"should not have gotAllPackets");
    test.assertEqualInt(radio.bufferRadio->positionWrite,buffer32Length - 1,"should set the positionWrite to size of first packet");

    // # CLEANUP
    testBufferRadioCleanUp();



}

void testBufferRadioCleanUp() {
    radio.bufferRadioReset(radio.bufferRadio);
    radio.bufferRadioReset(radio.bufferRadio + 1);
    radio.currentRadioBuffer = radio.bufferRadio;
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
