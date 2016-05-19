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
        radio.timeOfLastPoll = micros();
    }

  if (radio.didPicSendDeviceSerialData()) {
    radio.getSerialDataFromPicAndPutItInTheDevicesSerialBuffer();
  }

  if (radio.thereIsDataInSerialBuffer()) {
    if (radio.theLastTimeNewSerialDataWasAvailableWasLongEnough()){
      radio.sendTheDevicesFirstPacketToTheHost();
  }
  }

  if (radio.isTheDevicesRadioBufferFilledWithAllThePacketsFromTheHost) {
    radio.writeTheDevicesRadioBufferToThePic();
  }

  if (radio.isAStreamPacketWaitingForLaunch()) {
    if (radio.hasEnoughTimePassedToLaunchStreamPacket()) {
      radio.sendStreamPacketToTheHost();
    }
  }
}
