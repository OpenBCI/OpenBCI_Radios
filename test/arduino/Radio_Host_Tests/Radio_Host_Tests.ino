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

    testProcessOutboundBuffer();

    test.end();
}

void testProcessOutboundBuffer() {
    testProcessOutboundBufferCharSingle();
    testProcessOutboundBufferCharDouble();
}

void testProcessOutboundBufferCharSingle() {
    test.describe("processOutboundBufferCharSingle");

    test.assertEqualInt((int)radio.processOutboundBufferCharSingle('<'),ACTION_RADIO_SEND_NORMAL,"A < still results in sending a packet");
    test.assertEqualInt((int)radio.processOutboundBufferCharSingle(0x00),ACTION_RADIO_SEND_NONE,"Host channel query won't send packets");
    test.assertEqualInt((int)radio.processOutboundBufferCharSingle('a'),ACTION_RADIO_SEND_NORMAL,"Sends packet with A");
    test.assertEqualInt((int)radio.processOutboundBufferCharSingle(0xF6),ACTION_RADIO_SEND_NORMAL,"Sends packet with 0xF6");
}

void testProcessOutboundBufferCharDouble() {

    testProcessOutboundBufferHostChannelChange();
    testProcessOutboundBufferPollTimeChange();
    testProcessOutboundBufferCharDoubleNormal();

}

void testProcessOutboundBufferHostChannelChange() {
    test.describe("processOutboundBufferHostChannelChange");

    char data[OPENBCI_MAX_PACKET_SIZE_BYTES];

    int newChan = 3;
    int previousChan = 0;

    // Set the channel in memory
    radio.setChannelNumber(previousChan);

    // The code the driver sends to host to start a channel change process
    data[1] = (char)OPENBCI_HOST_CHANNEL_CHANGE;
    // Verify the define
    test.assertEqualInt(OPENBCI_HOST_CHANNEL_CHANGE,0x01,"#define for cmd verify");
    // The channel to change to
    data[2] = (char)newChan;

    // Call the function to test
    byte actualRadioAction = radio.processOutboundBufferCharDouble(data);
    // Will a single message be sent?
    test.assertEqualInt((int)actualRadioAction,ACTION_RADIO_SEND_SINGLE_CHAR, "Radio will send private radio msg");
    // Is the correct channel selected?
    test.assertEqualInt(radio.radioChannel,newChan,"Radio channel set to new channel");
    // Is the correct message in the singleCharMsg
    test.assertEqualChar(radio.singleCharMsg[0],(char)ORPM_CHANGE_CHANNEL_HOST_REQUEST,"Will send channel change request");
    // Is the previous channel saved?
    test.assertEqualInt(radio.previousRadioChannel,previousChan,"Previoud channel captured");

    // Now test the case where the channel number is too high
    newChan = 50;
    data[2] = (char)newChan; // Set the new channel

    // Send the buffer for processing
    actualRadioAction = radio.processOutboundBufferCharDouble(data);
    // No message should be sent
    test.assertEqualInt((int)actualRadioAction,ACTION_RADIO_SEND_NONE, "No message will be sent");

}

void testProcessOutboundBufferPollTimeChange() {
    test.describe("processOutboundBufferPollTimeChange");

    char data[OPENBCI_MAX_PACKET_SIZE_BYTES];

    uint8_t expectedPollTime = 100;

    // The code the driver sends to host to start a time change
    data[1] = (char)OPENBCI_HOST_POLL_TIME_CHANGE;
    // Verify the define
    test.assertEqualInt(OPENBCI_HOST_POLL_TIME_CHANGE,0x07,"#define for cmd verify");
    // The channel to change to
    data[2] = (char)expectedPollTime;

    // Call the function to test
    byte actualRadioAction = radio.processOutboundBufferCharDouble(data);
    // Will a single message be sent?
    test.assertEqualInt((int)actualRadioAction,ACTION_RADIO_SEND_SINGLE_CHAR, "Radio will send private radio msg");
    // Is the correct channel selected?
    test.assertEqualInt(radio.newPollTime,expectedPollTime,"Captured new radio time from buffer");
    // Is the correct message in the singleCharMsg
    test.assertEqualChar(radio.singleCharMsg[0],(char)ORPM_CHANGE_POLL_TIME_HOST_REQUEST,"Will send channel change request");

}

void testProcessOutboundBufferCharDoubleNormal() {
    test.describe("processOutboundBufferCharDoubleNormal");

    char data[OPENBCI_MAX_PACKET_SIZE_BYTES];

    // The code the driver sends to host to start a channel change process
    data[1] = 0x0B;
    data[2] = data[1];

    // Call the function to test
    byte actualRadioAction = radio.processOutboundBufferCharDouble(data);
    // Will a single message be sent?
    test.assertEqualInt((int)actualRadioAction,ACTION_RADIO_SEND_NORMAL, "Radio will send normal msg");
}
