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
* In streamingData mode, Host insterts a pre-fix and post-fix to the data for PC coordination.
*
* Single byte serial messages sent from PC are modified by the Host to include a '+' before and after
* This is to avoid an error experienced when the uC gets a 'ghost' command byte during streamData mode
*
* This software is provided as-is with no promise of workability
* Use at your own risk, wysiwyg.
*
* Made by Push The World LLC 2016
*/
#include <RFduinoGZLL.h>
#include "OpenBCI_Radios.h"

// OpenBCI_Radios_Class radio = OpenBCI_Radio_Class();

void setup() {
    // If you forgot your channel numbers, then force a reset by uncommenting
    //  the line below. This will force a reflash of the non-volitile memory space.
    // radio.setChannelNumber(20);

    // Declare the radio mode and channel number. Note this channel is only set on init flash
    radio.beginDebug(OPENBCI_MODE_HOST,20);
}

void loop() {

    // Is there a stream packet waiting to get sent to the PC
    if (radio.hasStreamPacket()) {
        // Send all the stream packets to the Driver/PC
        //  For resiliancy there is an oppertunity to have multiple stream
        //  packets waiting to get sent.
        radio.sendStreamPackets();
    }

    // Is there data in the radio buffer ready to be sent to the Driver?
    if (radio.gotAllRadioPackets) {
        // Write the buffer to the driver
        radio.writeTheHostsRadioBufferToThePC();
    }

    // Is there new data from the PC/Driver?
    if (radio.didPCSendDataToHost()) {
        // Get data and put it on the serial buffer
        boolean success = radio.storeCharToSerialBuffer(Serial.read());

        if (!success) {
            Serial.print("Input too large!$$$");
        }

        radio.lastTimeSerialRead = micros();
    }


    if (radio.hasItBeenTooLongSinceHostHeardFromDevice()) {
        if (radio.isWaitingForNewChannelNumberConfirmation) {
            radio.revertToPreviousChannelNumber();
            Serial.println("Timeout failed to reestablish connection.$$$");
        } else {
            if (radio.bufferSerial.numberOfPacketsToSend == 1) {
                if (radio.bufferSerial.packetBuffer->data[1] == OPENBCI_HOST_CHANNEL_QUERY) {
                    Serial.print("Host is on channel number: "); Serial.print(radio.getChannelNumber()); Serial.print(" no word from the Device though...");
                } else if (radio.bufferSerial.packetBuffer->data[1] == OPENBCI_HOST_CHANNEL_CHANGE_OVERIDE) {
                    // radio.setChannelNumber((uint32_t)radio.bufferSerial.packetBuffer->data[2]);
                    radio.setChannelNumber((uint32_t)radio.bufferSerial.packetBuffer->data[2]);
                    Serial.print("Channel Set To Number: "); Serial.println(radio.getChannelNumber());
                } else {
                    Serial.print("Error: No communications from Device/Board. Serial buffer cleared. Is your device is on the right channel? Is your board powered up?");
                }
                radio.bufferCleanSerial(radio.bufferSerial.numberOfPacketsToSend);
                Serial.print("$$$");
            } else if (radio.bufferSerial.numberOfPacketsToSend > 1) {
                radio.bufferCleanSerial(radio.bufferSerial.numberOfPacketsToSend);
                Serial.print("Error: No communications from Device/Board. Serial buffer cleared. Is your device is on the right channel? Is your board powered up?");
                Serial.print("$$$");
            }
        }
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
    // Reset the last time heard from host timer
    radio.lastTimeHostHeardFromDevice = millis();
    // Set send data packet flag to false
    boolean sendDataPacket = false;
    // Is the length of the packer equal to one?
    if (len == 1) {
        // Enter process single char subroutine
        sendDataPacket = radio.processRadioChar(device,data[0]);
        // Is the length of the packet greater than one?
    } else if (len > 1) {
        // Enter process char data packet subroutine
        sendDataPacket = radio.processHostRadioCharData(device,data,len);

    } else {
        // Are there packets waiting to be sent and was the Serial port read
        //  more then 3 ms ago?
        sendDataPacket = radio.packetToSend();
        if (sendDataPacket == false) {
            if (radio.bufferSerial.numberOfPacketsSent > 0) {
                radio.bufferCleanSerial(radio.bufferSerial.numberOfPacketsSent);
            }
        }
    }

    // Is the send data packet flag set to true
    if (sendDataPacket) {
        radio.sendPacketToDevice(device);
    }
}
