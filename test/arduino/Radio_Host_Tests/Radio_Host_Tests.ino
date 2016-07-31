#include <RFduinoGZLL.h>
#include "OpenBCI_Radios.h"
#include "PTW-Arduino-Assert.h"

int ledPin = 2;

void setup() {
    // Get's serial up and running
    pinMode(ledPin,OUTPUT);
    Serial.begin(115200);
    test.setSerial(Serial);
    test.failVerbosity = true;
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
    digitalWrite(ledPin, HIGH);

    testProcessOutboundBuffer();

    digitalWrite(ledPin, LOW);
    test.end();
}

void testProcessOutboundBuffer() {
    testProcessOutboundBufferForTimeSync();
    testProcessOutboundBufferCharDouble();
    testProcessOutboundBufferCharTriple();
}

void testProcessOutboundBufferForTimeSync() {
    test.describe("processOutboundBufferForTimeSync");

    test.it("should work with \'<\' charater");
    radio.bufferSerial.numberOfPacketsToSend = 1;
    radio.bufferSerial.numberOfPacketsSent = 0;
    radio.bufferSerial.packetBuffer->positionWrite = 2;
    radio.bufferSerial.packetBuffer->data[1] = (char)OPENBCI_HOST_TIME_SYNC;
    radio.packetInTXRadioBuffer = false;
    radio.sendSerialAck = false;
    test.assertBoolean(radio.processOutboundBufferForTimeSync(),true,"A < results in sending a packet", __LINE__);
    test.assertBoolean(radio.sendSerialAck,true,"should set send serial act to true",__LINE__);
    test.assertBoolean(radio.packetInTXRadioBuffer,true,"should set packet in tx buffer flag to true",__LINE__);
    test.assertEqualInt(radio.bufferSerial.packetBuffer->positionWrite,0x01, "should set position to 1", __LINE__);


    test.it("should not work with out the correct character");
    radio.bufferSerial.packetBuffer->positionWrite = 2;
    radio.bufferSerial.packetBuffer->data[1] = '?';
    radio.packetInTXRadioBuffer = false;
    radio.sendSerialAck = false;
    test.assertBoolean(radio.processOutboundBufferForTimeSync(),false,"should not find the time sync", __LINE__);
    test.assertBoolean(radio.sendSerialAck,false,"should not set send serial act to true",__LINE__);
    test.assertBoolean(radio.packetInTXRadioBuffer,false,"should set packet in tx buffer flag to false",__LINE__);
    test.assertEqualInt(radio.bufferSerial.packetBuffer->positionWrite,0x02, "should not change the write position", __LINE__);


}

void testProcessOutboundBufferCharDouble() {
    test.describe("testProcessOutboundBufferCharDouble");

    testProcessOutboundBufferCharDouble_OPENBCI_HOST_CMD_CHANNEL_GET();
    testProcessOutboundBufferCharDouble_OPENBCI_HOST_CMD_BAUD_DEFAULT();
    testProcessOutboundBufferCharDouble_OPENBCI_HOST_CMD_BAUD_FAST();
    testProcessOutboundBufferCharDouble_OPENBCI_HOST_CMD_SYS_UP();
    testProcessOutboundBufferCharDouble_OPENBCI_HOST_CMD_POLL_TIME_GET();
    testProcessOutboundBufferCharDouble_default();

}

void testProcessOutboundBufferCharDouble_OPENBCI_HOST_CMD_CHANNEL_GET() {
    test.it("should return to print the get chan success message if the system is up");
    radio.systemUp = true;
    radio.printMessageToDriverFlag = false;
    radio.msgToPrint = 25;
    radio.bufferSerial.packetBuffer->data[1] = (char)OPENBCI_HOST_PRIVATE_CMD_KEY;
    radio.bufferSerial.packetBuffer->data[2] = (char)OPENBCI_HOST_CMD_CHANNEL_GET;
    radio.bufferSerial.packetBuffer->positionWrite = 3;
    test.assertEqualByte(radio.processOutboundBufferCharDouble(radio.bufferSerial.packetBuffer->data),ACTION_RADIO_SEND_NONE, "should not send any message", __LINE__);
    test.assertEqualByte(radio.msgToPrint,radio.HOST_MESSAGE_CHAN_GET_SUCCESS,  "should get chan success message code", __LINE__);
    test.assertBoolean(radio.printMessageToDriverFlag,true,"sets the print flag to high", __LINE__);
    test.assertEqualInt(radio.bufferSerial.packetBuffer->positionWrite,0x01, "should set position to 1", __LINE__);

    test.it("should return to print the get chan failure message if the system is down");
    radio.systemUp = false;
    radio.printMessageToDriverFlag = false;
    radio.msgToPrint = 25;
    radio.bufferSerial.packetBuffer->data[1] = (char)OPENBCI_HOST_PRIVATE_CMD_KEY;
    radio.bufferSerial.packetBuffer->data[2] = (char)OPENBCI_HOST_CMD_CHANNEL_GET;
    radio.bufferSerial.packetBuffer->positionWrite = 3;
    test.assertEqualByte(radio.processOutboundBufferCharDouble(radio.bufferSerial.packetBuffer->data),ACTION_RADIO_SEND_NONE, "should not send any message", __LINE__);
    test.assertEqualByte(radio.msgToPrint,radio.HOST_MESSAGE_CHAN_GET_FAILURE,  "should get chan failure message code", __LINE__);
    test.assertBoolean(radio.printMessageToDriverFlag,true,"sets the print flag to high", __LINE__);
    test.assertEqualInt(radio.bufferSerial.packetBuffer->positionWrite,0x01, "should set position to 1", __LINE__);

}

void testProcessOutboundBufferCharDouble_OPENBCI_HOST_CMD_BAUD_DEFAULT() {
    test.it("should return to print the baud rate change to default message");
    radio.printMessageToDriverFlag = false;
    radio.msgToPrint = 25;
    radio.bufferSerial.packetBuffer->data[1] = (char)OPENBCI_HOST_PRIVATE_CMD_KEY;
    radio.bufferSerial.packetBuffer->data[2] = (char)OPENBCI_HOST_CMD_BAUD_DEFAULT;
    radio.bufferSerial.packetBuffer->positionWrite = 3;
    test.assertEqualByte(radio.processOutboundBufferCharDouble(radio.bufferSerial.packetBuffer->data),ACTION_RADIO_SEND_NONE, "should not send any message", __LINE__);
    test.assertEqualByte(radio.msgToPrint,radio.HOST_MESSAGE_BAUD_DEFAULT, "should get baud rate change to default message code", __LINE__);
    test.assertBoolean(radio.printMessageToDriverFlag,true,"sets the print flag to high", __LINE__);
    test.assertEqualInt(radio.bufferSerial.packetBuffer->positionWrite,0x01, "should set position to 1", __LINE__);

}

void testProcessOutboundBufferCharDouble_OPENBCI_HOST_CMD_BAUD_FAST() {

    test.it("should return to print the baud rate change to fast message");
    radio.printMessageToDriverFlag = false;
    radio.msgToPrint = 25;
    radio.bufferSerial.packetBuffer->data[1] = (char)OPENBCI_HOST_PRIVATE_CMD_KEY;
    radio.bufferSerial.packetBuffer->data[2] = (char)OPENBCI_HOST_CMD_BAUD_FAST;
    radio.bufferSerial.packetBuffer->positionWrite = 3;
    test.assertEqualByte(radio.processOutboundBufferCharDouble(radio.bufferSerial.packetBuffer->data),ACTION_RADIO_SEND_NONE, "should not send any message", __LINE__);
    test.assertEqualByte(radio.msgToPrint,radio.HOST_MESSAGE_BAUD_FAST, "should get baud rate change to fast message code", __LINE__);
    test.assertBoolean(radio.printMessageToDriverFlag,true,"sets the print flag to high", __LINE__);
    test.assertEqualInt(radio.bufferSerial.packetBuffer->positionWrite,0x01, "should set position to 1", __LINE__);

}

void testProcessOutboundBufferCharDouble_OPENBCI_HOST_CMD_SYS_UP() {
    radio.bufferSerialReset(1);

    test.it("should return to print the system status success message if the system is up");
    radio.systemUp = true;
    radio.printMessageToDriverFlag = false;
    radio.msgToPrint = 25;
    radio.bufferSerial.packetBuffer->data[1] = (char)OPENBCI_HOST_PRIVATE_CMD_KEY;
    radio.bufferSerial.packetBuffer->data[2] = (char)OPENBCI_HOST_CMD_SYS_UP;
    radio.bufferSerial.packetBuffer->positionWrite = 3;
    test.assertEqualByte(radio.processOutboundBufferCharDouble(radio.bufferSerial.packetBuffer->data),ACTION_RADIO_SEND_NONE, "should not send any message", __LINE__);
    test.assertEqualByte(radio.msgToPrint,radio.HOST_MESSAGE_SYS_UP, "should get system status success message code", __LINE__);
    test.assertBoolean(radio.printMessageToDriverFlag,true,"sets the print flag to high", __LINE__);
    test.assertEqualInt(radio.bufferSerial.packetBuffer->positionWrite,0x01, "should set position to 1", __LINE__);

    test.it("should return to print the system status failure message if the system is down");
    radio.systemUp = false;
    radio.printMessageToDriverFlag = false;
    radio.msgToPrint = 25;
    radio.bufferSerial.packetBuffer->data[1] = (char)OPENBCI_HOST_PRIVATE_CMD_KEY;
    radio.bufferSerial.packetBuffer->data[2] = (char)OPENBCI_HOST_CMD_SYS_UP;
    radio.bufferSerial.packetBuffer->positionWrite = 3;
    test.assertEqualByte(radio.processOutboundBufferCharDouble(radio.bufferSerial.packetBuffer->data),ACTION_RADIO_SEND_NONE, "should not send any message", __LINE__);
    test.assertEqualByte(radio.msgToPrint,radio.HOST_MESSAGE_SYS_DOWN, "should get system status failure message code", __LINE__);
    test.assertBoolean(radio.printMessageToDriverFlag,true,"sets the print flag to high", __LINE__);
    test.assertEqualInt(radio.bufferSerial.packetBuffer->positionWrite,0x01, "should set position to 1", __LINE__);

}

void testProcessOutboundBufferCharDouble_OPENBCI_HOST_CMD_POLL_TIME_GET() {
    test.it("should return to print the poll time get success message if the system is up");
    radio.systemUp = true;
    radio.printMessageToDriverFlag = false;
    radio.msgToPrint = 25;
    radio.bufferSerial.packetBuffer->data[1] = (char)OPENBCI_HOST_PRIVATE_CMD_KEY;
    radio.bufferSerial.packetBuffer->data[2] = (char)OPENBCI_HOST_CMD_POLL_TIME_GET;
    radio.bufferSerial.packetBuffer->positionWrite = 3;
    radio.singleCharMsg[0] = (char)0xFF;
    test.assertEqualByte(radio.processOutboundBufferCharDouble(radio.bufferSerial.packetBuffer->data),ACTION_RADIO_SEND_SINGLE_CHAR, "should send the single char message", __LINE__);
    test.assertEqualByte(radio.msgToPrint,25, "should not change the message to print", __LINE__);
    test.assertBoolean(radio.printMessageToDriverFlag,false,"should not change the print flag to high", __LINE__);
    test.assertEqualInt(radio.bufferSerial.packetBuffer->positionWrite,0x01, "should clear the serial buffer to position write 1", __LINE__);
    test.assertEqualChar(radio.singleCharMsg[0],(char)ORPM_GET_POLL_TIME, "should store poll time get request in single char buffer", __LINE__);

    test.it("should return to print the poll time get failure message if the system is down");
    radio.systemUp = false;
    radio.printMessageToDriverFlag = false;
    radio.msgToPrint = 25;
    radio.bufferSerial.packetBuffer->data[1] = (char)OPENBCI_HOST_PRIVATE_CMD_KEY;
    radio.bufferSerial.packetBuffer->data[2] = (char)OPENBCI_HOST_CMD_POLL_TIME_GET;
    radio.bufferSerial.packetBuffer->positionWrite = 3;
    test.assertEqualByte(radio.processOutboundBufferCharDouble(radio.bufferSerial.packetBuffer->data),ACTION_RADIO_SEND_NONE, "should not send any message", __LINE__);
    test.assertEqualByte(radio.msgToPrint,radio.HOST_MESSAGE_COMMS_DOWN, "should get poll time get failure message code", __LINE__);
    test.assertBoolean(radio.printMessageToDriverFlag,true,"sets the print flag to high", __LINE__);
    test.assertEqualInt(radio.bufferSerial.packetBuffer->positionWrite,0x01, "should set position to 1", __LINE__);

}

void testProcessOutboundBufferCharDouble_default() {
    test.it("should not work even a little bit");
    radio.systemUp = false;
    radio.printMessageToDriverFlag = false;
    radio.msgToPrint = 25;
    radio.bufferSerial.packetBuffer->data[1] = (char)OPENBCI_HOST_PRIVATE_CMD_KEY;
    radio.bufferSerial.packetBuffer->data[2] = (char)0x21;
    radio.bufferSerial.packetBuffer->positionWrite = 3;
    test.assertEqualByte(radio.processOutboundBufferCharDouble(radio.bufferSerial.packetBuffer->data),ACTION_RADIO_SEND_NORMAL,"should take no radio action", __LINE__);
    test.assertEqualByte(radio.msgToPrint,25, "should not change message to print", __LINE__);
    test.assertBoolean(radio.printMessageToDriverFlag,false,"should leave the print flag to false", __LINE__);
    test.assertEqualInt(radio.bufferSerial.packetBuffer->positionWrite,0x03, "should still be at write position to 3", __LINE__);

}

void testProcessOutboundBufferCharTriple() {
    test.describe("testProcessOutboundBufferCharTriple");

    testProcessOutboundBufferCharTriple_OPENBCI_HOST_CMD_CHANNEL_SET();
    testProcessOutboundBufferCharTriple_OPENBCI_HOST_CMD_POLL_TIME_SET();
    testProcessOutboundBufferCharTriple_OPENBCI_HOST_CMD_CHANNEL_SET_OVERIDE();
    testProcessOutboundBufferCharTriple_default();

}

void testProcessOutboundBufferCharTriple_OPENBCI_HOST_CMD_CHANNEL_SET() {

    test.it("should send a request to the device to change channels when channel in range");
    byte newChannelNumber = 0x01;
    radio.bufferSerial.packetBuffer->data[1] = (char)OPENBCI_HOST_PRIVATE_CMD_KEY;
    radio.bufferSerial.packetBuffer->data[2] = (char)OPENBCI_HOST_CMD_CHANNEL_SET;
    radio.bufferSerial.packetBuffer->data[3] = (char)newChannelNumber;
    radio.bufferSerial.packetBuffer->positionWrite = 4;
    radio.singleCharMsg[0] = (char)0xFF;
    radio.previousRadioChannel = 0x30;
    test.assertEqualByte(radio.processOutboundBufferCharTriple(radio.bufferSerial.packetBuffer->data),ACTION_RADIO_SEND_SINGLE_CHAR,"should send a private radio message", __LINE__);
    test.assertEqualInt((int)radio.radioChannel, newChannelNumber,"should capture new radio channel number", __LINE__);
    test.assertBetweenInclusiveInt((int)radio.previousRadioChannel,RFDUINOGZLL_CHANNEL_LIMIT_LOWER,RFDUINOGZLL_CHANNEL_LIMIT_UPPER, "should get previousRadioChannel number to be >= 0 and <= 25", __LINE__);
    test.assertEqualInt(radio.bufferSerial.packetBuffer->positionWrite,0x01, "should reset the write position to 1", __LINE__);
    test.assertEqualChar(radio.singleCharMsg[0],(char)ORPM_CHANGE_CHANNEL_HOST_REQUEST, "should store host channel change requestin single char buffer", __LINE__);

    test.it("should not send a request to the device to change channels when channel number out of range");
    newChannelNumber = 0x40;
    radio.bufferSerial.packetBuffer->data[1] = (char)OPENBCI_HOST_PRIVATE_CMD_KEY;
    radio.bufferSerial.packetBuffer->data[2] = (char)OPENBCI_HOST_CMD_CHANNEL_SET;
    radio.bufferSerial.packetBuffer->data[3] = (char)newChannelNumber;
    radio.bufferSerial.packetBuffer->positionWrite = 4;
    radio.singleCharMsg[0] = (char)0xFF;
    radio.previousRadioChannel = 0x30;
    radio.msgToPrint = 25;
    test.assertEqualByte(radio.processOutboundBufferCharTriple(radio.bufferSerial.packetBuffer->data),ACTION_RADIO_SEND_NONE,"should take no radio action", __LINE__);
    test.assertEqualByte(radio.msgToPrint,radio.HOST_MESSAGE_CHAN_VERIFY, "should send verify channel number message", __LINE__);
    test.assertBoolean(radio.printMessageToDriverFlag,true,"should set print flag to true", __LINE__);
    test.assertEqualInt(radio.bufferSerial.packetBuffer->positionWrite,0x01, "should reset the write position to 1", __LINE__);

}

void testProcessOutboundBufferCharTriple_OPENBCI_HOST_CMD_POLL_TIME_SET() {
    test.it("should send a request to the device to change poll time");
    byte newPollTime = 0x11;
    radio.bufferSerial.packetBuffer->data[1] = (char)OPENBCI_HOST_PRIVATE_CMD_KEY;
    radio.bufferSerial.packetBuffer->data[2] = (char)OPENBCI_HOST_CMD_POLL_TIME_SET;
    radio.bufferSerial.packetBuffer->data[3] = (char)newPollTime;
    radio.bufferSerial.packetBuffer->positionWrite = 4;
    radio.singleCharMsg[0] = (char)0xFF;
    radio.previousRadioChannel = 0x30;
    test.assertEqualByte(radio.processOutboundBufferCharTriple(radio.bufferSerial.packetBuffer->data),ACTION_RADIO_SEND_SINGLE_CHAR,"should send a private radio message", __LINE__);
    test.assertEqualInt((int)radio.pollTime, newPollTime,"should capture new radio poll time", __LINE__);
    test.assertEqualInt(radio.bufferSerial.packetBuffer->positionWrite,0x01, "should reset the write position to 1", __LINE__);
    test.assertEqualChar(radio.singleCharMsg[0],(char)ORPM_CHANGE_POLL_TIME_HOST_REQUEST, "should store host poll change request in single char buffer", __LINE__);
}

void testProcessOutboundBufferCharTriple_OPENBCI_HOST_CMD_CHANNEL_SET_OVERIDE() {
    test.it("should print message and change channels when channel in range");
    byte newChannelNumber = 0x01;
    radio.bufferSerial.packetBuffer->data[1] = (char)OPENBCI_HOST_PRIVATE_CMD_KEY;
    radio.bufferSerial.packetBuffer->data[2] = (char)OPENBCI_HOST_CMD_CHANNEL_SET_OVERIDE;
    radio.bufferSerial.packetBuffer->data[3] = (char)newChannelNumber;
    radio.bufferSerial.packetBuffer->positionWrite = 4;
    radio.previousRadioChannel = 0x30;
    radio.msgToPrint = 25;
    test.assertEqualByte(radio.processOutboundBufferCharTriple(radio.bufferSerial.packetBuffer->data),ACTION_RADIO_SEND_NONE,"should send a private radio message", __LINE__);
    test.assertEqualInt((int)radio.radioChannel, newChannelNumber,"should capture new radio channel number", __LINE__);
    test.assertEqualByte(radio.msgToPrint,radio.HOST_MESSAGE_CHAN_OVERRIDE, "should change message to print to host channel override", __LINE__);
    test.assertBoolean(radio.printMessageToDriverFlag,true,"should change the print flag to true", __LINE__);
    test.assertEqualInt(radio.bufferSerial.packetBuffer->positionWrite,0x01, "should reset the write position to 1", __LINE__);

    test.it("should not send a request to the device to change channels when channel number out of range");
    newChannelNumber = 0x40;
    radio.bufferSerial.packetBuffer->data[1] = (char)OPENBCI_HOST_PRIVATE_CMD_KEY;
    radio.bufferSerial.packetBuffer->data[2] = (char)OPENBCI_HOST_CMD_CHANNEL_SET_OVERIDE;
    radio.bufferSerial.packetBuffer->data[3] = (char)newChannelNumber;
    radio.bufferSerial.packetBuffer->positionWrite = 4;
    radio.previousRadioChannel = 0x30;
    radio.msgToPrint = 25;
    test.assertEqualByte(radio.processOutboundBufferCharTriple(radio.bufferSerial.packetBuffer->data),ACTION_RADIO_SEND_NONE,"should take no radio action", __LINE__);
    test.assertEqualByte(radio.msgToPrint,radio.HOST_MESSAGE_CHAN_VERIFY, "should send verify channel number message", __LINE__);
    test.assertBoolean(radio.printMessageToDriverFlag,true,"should set print flag to true", __LINE__);
    test.assertEqualInt(radio.bufferSerial.packetBuffer->positionWrite,0x01, "should reset the write position to 1", __LINE__);

}

void testProcessOutboundBufferCharTriple_default() {
    test.it("should do nothing and take a normal radio action");
    byte newChannelNumber = 0x40;
    radio.bufferSerial.packetBuffer->data[1] = (char)OPENBCI_HOST_PRIVATE_CMD_KEY;
    radio.bufferSerial.packetBuffer->data[2] = (char)0xFF;
    radio.bufferSerial.packetBuffer->data[3] = (char)newChannelNumber;
    radio.bufferSerial.packetBuffer->positionWrite = 4;
    radio.previousRadioChannel = 0x30;
    radio.singleCharMsg[0] = (char)0xFF;

    test.assertEqualByte(radio.processOutboundBufferCharTriple(radio.bufferSerial.packetBuffer->data),ACTION_RADIO_SEND_NORMAL,"should take normal radio action", __LINE__);

}



void testProcessOutboundBufferHostChannelChange() {
    test.describe("processOutboundBufferHostChannelChange");

    char data[OPENBCI_MAX_PACKET_SIZE_BYTES];

    int newChan = 3;
    int previousChan = 0;

    // Set the channel in memory
    radio.setChannelNumber(previousChan);

    data[1] = (char)OPENBCI_HOST_PRIVATE_CMD_KEY;
    // The code the driver sends to host to start a channel change process
    data[2] = (char)OPENBCI_HOST_CMD_CHANNEL_SET_OVERIDE;
    // Verify the define
    test.assertEqualInt(OPENBCI_HOST_CMD_CHANNEL_SET_OVERIDE,0x02,"#define for cmd verify", __LINE__);
    // The channel to change to
    data[3] = (char)newChan;

    // Call the function to test
    byte actualRadioAction = radio.processOutboundBufferCharDouble(data);
    // Will a single message be sent?
    test.assertEqualByte(actualRadioAction,ACTION_RADIO_SEND_SINGLE_CHAR, "Radio will send private radio msg", __LINE__);
    // Is the correct channel selected?
    test.assertEqualInt(radio.radioChannel,newChan,"Radio channel set to new channel", __LINE__);
    // Is the correct message in the singleCharMsg
    test.assertEqualChar(radio.singleCharMsg[0],(char)ORPM_CHANGE_CHANNEL_HOST_REQUEST,"Will send channel change request", __LINE__);
    // Is the previous channel saved?
    test.assertEqualInt((int)radio.previousRadioChannel,previousChan,"Previoud channel captured", __LINE__);

    // Now test the case where the channel number is too high
    newChan = 50;
    data[2] = (char)newChan; // Set the new channel

    // Send the buffer for processing
    actualRadioAction = radio.processOutboundBufferCharTriple(data);
    // No message should be sent
    test.assertEqualByte(actualRadioAction,ACTION_RADIO_SEND_NONE, "No message will be sent", __LINE__);

}

void testProcessOutboundBufferPollTimeChange() {
    test.describe("processOutboundBufferPollTimeChange");

    char data[OPENBCI_MAX_PACKET_SIZE_BYTES];

    uint8_t expectedPollTime = 100;

    data[1] = (char)OPENBCI_HOST_PRIVATE_CMD_KEY;
    // The code the driver sends to host to start a time change
    data[2] = (char)OPENBCI_HOST_CMD_POLL_TIME_GET;
    // Verify the define
    test.assertEqualInt(OPENBCI_HOST_CMD_POLL_TIME_SET,0x04,"#define for cmd verify", __LINE__);
    // The channel to change to
    data[3] = (char)expectedPollTime;

    // Call the function to test
    byte actualRadioAction = radio.processOutboundBufferCharTriple(data);
    // Will a single message be sent?
    test.assertEqualByte(actualRadioAction,ACTION_RADIO_SEND_SINGLE_CHAR, "Radio will send private radio msg", __LINE__);
    // Is the correct poll time set selected?
    test.assertEqualInt(radio.pollTime,expectedPollTime,"Captured new radio time from buffer", __LINE__);
    // Is the correct message in the singleCharMsg
    test.assertEqualChar(radio.singleCharMsg[0],(char)ORPM_CHANGE_POLL_TIME_HOST_REQUEST,"Will send channel change request", __LINE__);

}

void testProcessOutboundBufferCharDoubleNormal() {
    test.describe("processOutboundBufferCharDoubleNormal");

    char data[OPENBCI_MAX_PACKET_SIZE_BYTES];

    // Write code that is not a special case
    data[1] = 0x0B;
    data[2] = data[1];

    // Call the function to test
    byte actualRadioAction = radio.processOutboundBufferCharDouble(data);
    // Will a single message be sent?
    test.assertEqualByte(actualRadioAction,ACTION_RADIO_SEND_NORMAL, "Radio will send normal msg", __LINE__);
}
