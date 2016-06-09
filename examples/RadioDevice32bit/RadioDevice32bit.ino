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
    // radio.flashNonVolatileMemory();

    // Declare the radio mode and channel
    radio.beginDebug(OPENBCI_MODE_DEVICE,20);
}

void loop() {

    // First we must ask if an emergency stop flag has been triggered, as a Device
    //  we must frequently ask this question as we are the only one that can
    //  initiaite a communication between back to the Driver.
    if (radio.bufferSerial.overflowed) {
        // Clear the buffer holding all serial data.
        radio.bufferCleanSerial(OPENBCI_MAX_NUMBER_OF_BUFFERS);

        // Clear the stream packet buffer
        radio.bufferResetStreamPacketBuffer();

        // Send reset message to the board
        radio.resetPic32();

        // Reset the last time we contacted the host to now
        radio.pollRefresh();

        // Send emergency message to the host
        radio.sendRadioMessageToHost(ORPM_DEVICE_SERIAL_OVERFLOW);

        radio.bufferSerial.overflowed = false;

    } else {
        if (radio.isAStreamPacketWaitingForLaunch()) { // Is there a stream packet waiting to get sent to the Host?
            // Has 90uS passed since the last time we read from the serial port?
            if (micros() > (radio.lastTimeSerialRead + OPENBCI_TIMEOUT_PACKET_STREAM_uS)) {
                radio.debugT3 = micros();
                Serial.print("st:"); Serial.print(radio.debugT3 - radio.lastTimeSerialRead);
                Serial.print(" tt:"); Serial.println(radio.debugT3 - radio.debugT1);
                radio.sendStreamPacketToTheHost();
                // Serial.println(radio.debugT2 - radio.debugT1);
            }
        } else if (radio.didPicSendDeviceSerialData()) { // Is there new serial data available?
            // Get one char and process it
            radio.processChar(Serial.read());
            // Reset the poll timer to prevent contacting the host mid read
            radio.pollRefresh();

            // radio.debugT4 = micros();

            radio.lastTimeSerialRead = micros();


        } else if (radio.thereIsDataInSerialBuffer()) { // Is there data from the Pic waiting to get sent to Host
            // Has 3ms passed since the last time the serial port was read. Only the
            //  first packet get's sent from here

            if ((micros() > (radio.lastTimeSerialRead + OPENBCI_TIMEOUT_PACKET_NRML_uS)) && radio.bufferSerial.numberOfPacketsSent == 0){
                radio.debugT3 = micros();
                // In order to do checksumming we must only send one packet at a time
                //  this stands as the first time we are going to send a packet!
                Serial.print("se:"); Serial.print(radio.debugT3 - radio.lastTimeSerialRead);
                Serial.print(" tt:"); Serial.println(radio.debugT3 - radio.debugT1);
                radio.sendPacketToHost();
                // Serial.print("-"); Serial.println((micros() - (radio.lastTimeSerialRead + OPENBCI_TIMEOUT_PACKET_NRML_uS)));
            }
        }

        if (radio.gotAllRadioPackets) { // Did we recieve all packets in a potential multi packet transmission
            // push radio buffer to pic
            radio.pushRadioBuffer();
            // reset the radio buffer
            radio.bufferCleanRadio();
        }

        if (millis() > (radio.timeOfLastPoll + OPENBCI_TIMEOUT_PACKET_POLL_MS)) {  // Has more than the poll time passed?
            // Refresh the poll timer
            radio.pollRefresh();

            // Poll the host
            radio.sendPollMessageToHost();
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
    radio.debugT5 = micros();
    // Set send data packet flag to false
    boolean sendDataPacket = false;
    // Is the length of the packer equal to one?
    if (len == 1) {
        // Enter process single char subroutine
        sendDataPacket = radio.processRadioCharDevice(data[0]);
    // Is the length of the packet greater than one?
    } else if (len > 1) {
        // Enter process char data packet subroutine
        sendDataPacket = radio.processDeviceRadioCharData(data,len);
    } else {

        // Serial.print("Nr"); Serial.println(radio.debugT5 - radio.debugT4);
        // Are there packets waiting to be sent and was the Serial port read
        //  more then 3 ms ago?
        sendDataPacket = radio.packetToSend();
        if (sendDataPacket == false) {
            radio.bufferCleanSerial(radio.bufferSerial.numberOfPacketsSent);
            // radio.bufferResetStreamPacketBuffer();
        }
    }

    // Is the send data packet flag set to true
    if (sendDataPacket) {
        radio.sendPacketToHost();
    }
}
