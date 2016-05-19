/*
 * Sets up RFduino Device for OpenBCI_32bit using RFduinoGZLL library
 *
 * This test behaves as a serial pass thru between two RFduinos,
 * To Program, user must have RFduino core files installed in Arduino 1.5.8 or later
 * Use the RFRST, RFRX, RFTX, and GND on the board to connect to USB<>Serial device
 * Your USB<>Serial device must connect RFRST with DTR through 0.1uF capacitor (sorry)
 *
 * Made by AJ Keller, Spring 2016
 * Free to use and share. This code presented as-is.
*/
#include <RFduinoGZLL.h>
#include "OpenBCI_Radios.h"

void setup() {
    // If you forgot your channel numbers, then force a reset by uncommenting
    //  the line below. This will force a reflash of the non-volitile memory space.
    // radio.setChannelNumber(20);

    // Declare the radio mode and channel
    radio.begin(OPENBCI_MODE_DEVICE,20);
}

void loop() {

    // First we must ask if an emergency stop flag has been triggered, as a Device
    //  we must frequently ask this question as we are the only one that can
    //  initiaite a communication between back to the Driver.
    if (radio.emergencyStop) {
        // Clear the buffer holding all serial data.
        radio.bufferCleanSerial(radio.bufferSerial.numberOfPacketsToSend);

        // Clear the stream packet buffer
        radio.bufferResetStreamPacketBuffer();

        // Send reset message to the board
        radio.resetPic32();

        // Reset the last time we contacted the host to now
        radio.pollRefresh();

        // Send emergency message to the host
        radio.sendRadioMessageToHost(ORPM_DEVICE_SERIAL_OVERFLOW);

        // Reset the emergencyStop flag
        radio.emergencyStop = false;

    } else if (radio.didPicSendDeviceSerialData()) { // Is there new serial data available?
        radio.processChar(Serial.read());
        // Fetch serial data. This enters a subroutine.
        radio.bufferSerialFetch();

    } else if (radio.isAStreamPacketWaitingForLaunch()) { // Is there a stream packet waiting to get sent to the Host?
        // Has 90uS passed since the last time we read from the serial port?
        if (micros() > (lastTimeSerialRead + OPENBCI_TIMEOUT_PACKET_STREAM_uS)) {
            radio.sendStreamPacketToTheHost();
        }

    } else if (radio.thereIsDataInSerialBuffer()) { // Is there data from the Pic waiting to get sent to Host
        // Has 3ms passed since the last time the serial port was read
        if (micros() > (lastTimeSerialRead + OPENBCI_TIMEOUT_PACKET_NRML_uS)){
            // In order to do checksumming we must only send one packet at a time
            //  this stnads as the first time we are going to send a packet!
            radio.sendTheDevicesFirstPacketToTheHost();
        }

    } else if (radio.gotAllRadioPackets) { // Did we recieve all packets in a potential multi packet transmission
        // push radio buffer to pic
        radio.pushRadioBuffer();
        // reset the radio buffer
        bufferCleanRadio();
    } else if (millis() > (radio.timeOfLastPoll + OPENBCI_TIMEOUT_PACKET_POLL_MS)) {  // Has more than the poll time passed?
        // Refresh the timer
        radio.pollRefresh();

        // Poll the host
        radio.sendPollMessageToHost();
    }
}
