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
#include "OpenBCI_Radio.h"



void setup() {
  // put your setup code here, to run once:
  radio.begin(OPENBCI_MODE_HOST,20);
}

void loop() {
    // put your main code here, to run repeatedly:

    if (radio.doesTheHostHaveAStreamPacketToSendToPC()) {
        radio.writeTheHostsStreamPacketBufferToThePC();
    }

    if (radio.didPCSendDataToHost()) {
        radio.getSerialDataFromPCAndPutItInHostsSerialBuffer();
    }

    if (radio.isTheHostsRadioBufferFilledWithAllThePacketsFromTheDevice) {
        radio.writeTheHostsRadioBufferToThePC();
    }

    if (radio.hasItBeenTooLongSinceHostHeardFromDevice()) {
        if (radio.isWaitingForNewChannelNumberConfirmation) {
            radio.revertToPreviousChannelNumber();
            Serial.println("Timeout failed to restablish connection.$$$");
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
