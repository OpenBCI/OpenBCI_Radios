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
* Written by Push The World LLC 2016 inspired by Joel Murphy, Leif Percifield
*  and Conor Russomanno. You should have recieved a copy of the license when
*  you downloaded from github. Free to use and share. This code presented for
*  use as-is.
*/
#include <RFduinoGZLL.h>
#include "OpenBCI_Radios.h"

// OpenBCI_Radios_Class radio = OpenBCI_Radio_Class();

void setup() {
    // If you forgot your channel numbers, then force a reset by uncommenting
    //  the line below. This will force a reflash of the non-volitile memory space.
    // radio.flashNonVolatileMemory();

    // Declare the radio mode and channel number. Note this channel is only
    //  set on init flash. MAKE SURE THIS CHANNEL NUMBER MATCHES THE DEVICE!
    radio.begin(OPENBCI_MODE_HOST,20);
}

void loop() {

    // Is there a stream packet waiting to get sent to the PC
    while (radio.ringBufferNumBytes > 0) {
        Serial.write(radio.ringBuffer[radio.ringBufferRead]);
        radio.ringBufferRead++;
        if (radio.ringBufferRead >= OPENBCI_BUFFER_LENGTH) {
            radio.ringBufferRead = 0;
        }
        radio.ringBufferNumBytes--;
     }

    // Is there data in the radio buffer ready to be sent to the Driver?
    if (radio.gotAllRadioPackets) {
        // Flush radio buffer to the driver
        radio.bufferRadioFlush();
        // Reset the radio buffer flags
        radio.bufferRadioReset();
        // Clean the buffer.. fill with zeros
        radio.bufferRadioClean();
    }

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

    // Is there data waiting to be sent out to the host?
    //  If there is code that shall be sent to device, then we want to move it
    //  into the TX buffer right away, because it will be sent the next time the
    //  device contacts the Host!
    if (radio.hostPacketToSend()) {
        radio.sendPacketToDevice(DEVICE0);
    }

    // Has more than 500ms passed since last contact from Device?
    if (radio.commsFailureTimeout() && (micros() > (radio.lastTimeSerialRead + OPENBCI_TIMEOUT_PACKET_NRML_uS))){
        radio.processCommsFailure();
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
            radio.printSuccess();
            radio.printChannelNumber(radio.getChannelNumber());
            radio.printEOT();
            radio.isWaitingForNewChannelNumberConfirmation = false;
        } else if (radio.isWaitingForNewPollTimeConfirmation) {
            radio.printSuccess();
            radio.printPollTime(radio.getPollTime());
            radio.printEOT();
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
        radio.sendPacketToDevice(device);
    }
}
