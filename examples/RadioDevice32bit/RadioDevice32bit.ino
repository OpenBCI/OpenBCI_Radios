/*
* Sets up RFduino Device for OpenBCI_32bit using RFduinoGZLL library
*
* This test behaves as a serial pass thru between two RFduinos,
* To Program, user must have RFduino core files installed in Arduino 1.5.8 or later
* Use the RFRST, RFRX, RFTX, and GND on the board to connect to USB<>Serial device
* Your USB<>Serial device must connect RFRST with DTR through 0.1uF capacitor (sorry)
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
  // MAKE SURE THIS CHANNEL NUMBER MATCHES THE HOST!
  radio.begin(OPENBCI_MODE_DEVICE,20);
}

void loop() {

  // First we must ask if an emergency stop flag has been triggered, as a Device
  //  we must frequently ask this question as we are the only one that can
  //  initiaite a communication between back to the Driver.
  if (radio.bufferSerial.overflowed) {
    // Clear the buffer holding all serial data.
    radio.bufferSerialReset(OPENBCI_NUMBER_SERIAL_BUFFERS);

    // Reset the stream buffer
    radio.bufferStreamReset();

    // Send reset message to the board
    radio.resetPic32();

    // Reset the last time we contacted the host to now
    radio.pollRefresh();

    // Send emergency message to the host
    radio.singleCharMsg[0] = (char)ORPM_DEVICE_SERIAL_OVERFLOW;

    if (RFduinoGZLL.sendToHost(radio.singleCharMsg,1)) {
      radio.bufferSerial.overflowed = false;  
    }
  } else {
    if (Serial.available()) { // Is there new serial data available?
      char newChar = Serial.read();
      // Mark the last serial as now;
      radio.lastTimeSerialRead = micros();
      // Store it to serial buffer
      radio.bufferSerialAddChar(newChar);
      // Get one char and process it
      radio.bufferStreamAddChar((radio.streamPacketBuffer + radio.streamPacketBufferHead), newChar);
      // Reset the poll timer to prevent contacting the host mid read
      radio.pollRefresh();
    }

    if ((radio.streamPacketBuffer + radio.streamPacketBufferHead)->state == radio.STREAM_STATE_READY) { // Is there a stream packet waiting to get sent to the Host?
      // Has 92uS passed since the last time we read from the serial port?
      if (radio.bufferStreamTimeout()) {
        // We are sure this is a streaming packet.
        radio.streamPacketBufferHead++;
        if (radio.streamPacketBufferHead > (OPENBCI_NUMBER_STREAM_BUFFERS - 1)) {
          radio.streamPacketBufferHead = 0;
        }
      }
    }

    if ((radio.streamPacketBuffer + radio.streamPacketBufferTail)->state == radio.STREAM_STATE_READY) { // Is there a stream packet waiting to get sent to the Host?
      if (radio.streamPacketBufferHead != radio.streamPacketBufferTail) {
        // Try to add the tail to the TX buffer
        if (radio.bufferStreamSendToHost(radio.streamPacketBuffer + radio.streamPacketBufferTail)) {
          radio.streamPacketBufferTail++;
          if (radio.streamPacketBufferTail > (OPENBCI_NUMBER_STREAM_BUFFERS - 1)) {
            radio.streamPacketBufferTail = 0;
          }
        }
      }
    }

    if (radio.bufferSerialHasData()) { // Is there data from the Pic waiting to get sent to Host
      // Has 3ms passed since the last time the serial port was read. Only the
      //  first packet get's sent from here
      if (radio.bufferSerialTimeout() && radio.bufferSerial.numberOfPacketsSent == 0 ) {
        // In order to do checksumming we must only send one packet at a time
        //  this stands as the first time we are going to send a packet!
        radio.sendPacketToHost();
      }
    }

    radio.bufferRadioFlushBuffers();

    if (millis() > (radio.timeOfLastPoll + radio.pollTime)) {  // Has more than the poll time passed?
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
    // Are there packets waiting to be sent and was the Serial port read
    //  more then 3 ms ago?
    sendDataPacket = radio.packetToSend();
    if (sendDataPacket == false) {
      if (radio.bufferSerial.numberOfPacketsSent > 0) {
        radio.bufferSerialReset(radio.bufferSerial.numberOfPacketsSent);
      }
    }
  }

  // Is the send data packet flag set to true
  if (sendDataPacket) {
    radio.sendPacketToHost();
  }
}
