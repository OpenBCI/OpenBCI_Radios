/*
* This sketch is OpenBCI_32bit specific and sets up a HOST module
* If you need to add custom files to your application software (sorry)
* If you want to modify this firmware, go to RFduino and download their latest release
* Install RFduino libraries as instructed from their website
* go here (there) to download the adjusted library components,
* and follow the instalation instructions to use the custom tools for OpenBCI
*
* Since the Device must initiate communication, the
* device "polls" the Host evey 63mS when not sending packets.
* Host is connected to PC via USB VCP (FTDI).
* Device is connectedd to uC (PIC32MX250F128B with UDB32-MX2-DIP).
*
* This software is provided as-is with no promise of workability
* Use at your own risk, wysiwyg.
*
* Written by Push The World LLC 2016 inspired by Joel Murphy, Leif Percifield
*  and Conor Russomanno. You should have recieved a copy of the license when
*  you downloaded from github. Free to use and share. This code presented for
*  use as-is.
*/
#include <RFduinoGZLL.h>
#include "OpenBCI_Radios.h"

void setup() {
    // Declare the radio mode and channel number. Note this channel is only
    //  set the first time the board powers up OR after a flash of the non-
    //  volatile memory space with a call to `flashNonVolatileMemory`.
    // MAKE SURE THIS CHANNEL NUMBER MATCHES THE DEVICE!
    radio.begin(OPENBCI_MODE_HOST,20);
}

void loop() {

    // Check the stream packet buffers for data
    if (radio.streamPacketBuffer1.full) {
        radio.bufferAddStreamPacket(&radio.streamPacketBuffer1);
    }
    if (radio.streamPacketBuffer2.full) {
        radio.bufferAddStreamPacket(&radio.streamPacketBuffer2);
    }
    if (radio.streamPacketBuffer3.full) {
        radio.bufferAddStreamPacket(&radio.streamPacketBuffer3);
    }
    // Is there a stream packet waiting to get sent to the PC
    if (radio.ringBufferWrite > 0) {
        for (int i = 0; i < radio.ringBufferWrite; i++) {
            Serial.write(radio.ringBuffer[i]);
        }
        radio.ringBufferWrite = 0;
    }

    radio.bufferRadioFlushBuffers();

    // Is there new data from the PC/Driver?
    // While loop to read successive bytes
    if (radio.didPCSendDataToHost()) {
        char newChar = Serial.read();
        // Save the last time serial data was read to now
        radio.lastTimeSerialRead = micros();
        // Get data and put it on the serial buffer
        boolean success = radio.storeCharToSerialBuffer(newChar);
        if (!success) {
            Serial.print("Failure: Input too large!$$$");
        }
    }

    // Set system to down if we experience a comms timout
    if (radio.commsFailureTimeout()) {
        // Mark the system as down
        radio.systemUp = false;
        // Check to see if data was left in the radio buffer from an incomplete
        //  multi packet transfer.. i.e. a failed over the air upload
        if (radio.bufferRadioHasData(radio.currentRadioBuffer)) {
            // Reset the radio buffer flags
            radio.bufferRadioReset(radio.currentRadioBuffer);
            // Clean the buffer.. fill with zeros
            radio.bufferRadioClean(radio.currentRadioBuffer);
        }
    }

    if (radio.serialWriteTimeOut()) {
        // Is the time the Device contacted the host greater than 0? This is
        //  true if the Device has NEVER contacted the Host
        if (radio.lastTimeHostHeardFromDevice > 0) {
            // Is a packet in the TX buffer
            if (radio.packetInTXRadioBuffer == false) {
                //packets in the serial buffer and there is 1 packet to send
                if (radio.bufferSerial.numberOfPacketsSent == 0 && radio.bufferSerial.numberOfPacketsToSend == 1) {
                    // process with a send to device
                    radio.processOutboundBufferForTimeSync();
                }
            } else {
                // Comms time out?
                if (radio.commsFailureTimeout()) {
                    if (radio.isWaitingForNewChannelNumberConfirmation && !radio.channelNumberSaveAttempted) {
                        RFduinoGZLL.end();
                        RFduinoGZLL.channel = radio.getChannelNumber();
                        RFduinoGZLL.begin(RFDUINOGZLL_ROLE_HOST);
                        radio.lastTimeHostHeardFromDevice = millis();
                        radio.channelNumberSaveAttempted = true;
                    } else {
                        radio.processCommsFailure();
                    }
                }
            }
        } else { // lastTimeHostHeardFromDevice has not been changed
            // comms time out?
            if (radio.commsFailureTimeout()) {
                radio.processCommsFailure();
            }
        }
    }

    if (radio.printMessageToDriverFlag) {
        radio.printMessageToDriverFlag = false;
        radio.printMessageToDriver(radio.msgToPrint);
    }
}

/**
 * @description A packet with 1 byte is a private radio message, a packet with
 *                  more than 1 byte is a standard packet with a checksum. and
 *                  a packet with no length is a NULL packet that indicates a
 *                  successful message transmission
 * @param device {device_t} - The host in this case
 * @param rssi {int} - NOT used
 * @param data {char *} - The packet of data sent in the packet
 * @param len {int} - The length of the `data` packet
 */
void RFduinoGZLL_onReceive(device_t device, int rssi, char *data, int len) {
    // We know that the last packet was just sent
    if (radio.packetInTXRadioBuffer) {
        radio.packetInTXRadioBuffer = false;
    }
    // Send a time sync ack to driver?
    if (radio.sendSerialAck) {
        radio.bufferAddTimeSyncSentAck();
    }
    // If system is not up, set it up!
    radio.systemUp = true;

    // Reset the last time heard from host timer
    radio.lastTimeHostHeardFromDevice = millis();
    // Set send data packet flag to false
    boolean sendDataPacket = false;
    // Is the length of the packer equal to one?
    if (len == 1) {
        // Enter process single char subroutine
        sendDataPacket = radio.processRadioCharHost(device,data[0]);
        // Is the length of the packet greater than one?
    } else if (len > 1) {
        // Enter process char data packet subroutine
        sendDataPacket = radio.processHostRadioCharData(device,data,len);

    } else {
        // Condition
        if (radio.isWaitingForNewChannelNumberConfirmation) {
            if (!radio.channelNumberSaveAttempted) {
                RFduinoGZLL.end();
                RFduinoGZLL.channel = radio.getChannelNumber();
                RFduinoGZLL.begin(RFDUINOGZLL_ROLE_HOST);
            }
            radio.msgToPrint = OPENBCI_HOST_MSG_CHAN_GET_SUCCESS;
            radio.printMessageToDriverFlag = true;
            radio.isWaitingForNewChannelNumberConfirmation = false;
        } else if (radio.isWaitingForNewPollTimeConfirmation) {
            radio.msgToPrint = OPENBCI_HOST_MSG_POLL_TIME;
            radio.printMessageToDriverFlag = true;
            radio.isWaitingForNewPollTimeConfirmation = false;
        }
        // Are there packets waiting to be sent and was the Serial port read
        //  more then 3 ms ago?
        sendDataPacket = radio.hostPacketToSend();
        if (sendDataPacket == false) {
            if (radio.bufferSerial.numberOfPacketsSent > 0) {
                radio.bufferCleanSerial(radio.bufferSerial.numberOfPacketsSent);
            }
        }
    }

    // Is the send data packet flag set to true
    if (sendDataPacket) {
        radio.sendPacketToDevice(device, false);
    }
}
