/***************************************************
  This is a library for the OpenBCI 32bit RFduinoGZLL Device and host
  Let us define two over arching operating modes / paradigms: Host and Device, where:
    * Host is connected to PC via USB VCP (FTDI).
    * Device is connectedd to uC (PIC32MX250F128B with UDB32-MX2-DIP).

  These displays use I2C to communicate, 2 pins are required to
  interface
  OpenBCI invests time and resources providing this open source code,
  please support OpenBCI and open-source hardware by purchasing
  products from OpenBCI!

  Written by AJ Keller of Push The World LLC but much credit must also go to
  Joel Murphy who with Conor Russomanno and Leif Percifield created the
  original OpenBCI_32bit_Device.ino and OpenBCI_32bit_Host.ino files in the
  Summer of 2014. Much of this code base is inspired directly from their work.

  MIT license
 ****************************************************/

#include "OpenBCI_Radio.h"

// CONSTRUCTOR
OpenBCI_Radio::OpenBCI_Radio() {
}

/***************************************************/
/** PUBLIC METHODS *********************************/
/***************************************************/

/**
* @description: The function that the radio will call in setup()
* @param: mode [unint8_t] - The mode the radio shall operate in
* @param: channelNumber [int8_t] - The channelNumber the RFduinoGZLL will
*           use to communicate with the other RFduinoGZLL.
*           NOTE: Must be from 2 - 25
* @author: AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radio::begin(unint8_t mode, int8_t channelNumber) {

}

/**
* @description: This function should be called in the loop()
* @author: AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio::readSerial(void) {
  
}

/***************************************************/
/** PRIVATE METHODS ********************************/
/***************************************************/

/**
* @description: Private function to flush the buffer.
* @author: AJ Keller (@pushtheworldllc)
*/
void bufferReset(PacketBuffer *buffer,int numberOfBuffers) {
  for(int i = 0; i < numberOfBuffers; i++) {
    buffer[i].readPosition = 0;
  }
}

/**
* @description: Private function to initialize the OpenBCI_Radio object
* @param: mode [unint8_t] - The mode the radio shall operate in
* @param: channelNumber [int8_t] - The channelNumber the RFduinoGZLL will
*           use to communicate with the other RFduinoGZLL
* @author: AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio::initialize(unint8_t mode, int8_t channelNumber) {
  // Quickly check to see if in pass through mode, if so, call and dip out of func
  if (mode == OPENBCI_MODE_PASS_THRU) {
    configurePassThru();
  } else { // we are either dealing with a Host or a Device
    // We give the oppertunity to call any 'universal' code, rather code, that
    //    gets set up the same on both Host and Device
    RFduinoGZLL.channel = channelNumber;

    // get that main buffer ready
    bufferReset(bufferArray,OPENBCI_MAX_NUMBER_OF_BUFFERS);

    // Diverge program execution based on Device or Host
    switch (mode) {
      case OPENBCI_MODE_DEVICE:
        configureDevice(); // setup for Device
        break;
      default: // we default to OPENBCI_MODE_HOST
        configureHost(); // setup for Host
        break;
    }
  }
}

/**
* @description: Private function to initialize the radio in Device mode
* @author: AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio::configureDevice(void) {
  // Start the RFduinoGZLL in DEVICE0 mode
  RFduinoGZLL.begin(RFDUINOGZLL_ROLE_DEVICE);

  // Configure pins
  pinMode(OPENBCI_PIN_DEVICE_PCG, INPUT); //feel the state of the PIC with this pin

  // Start the serial connection. On the device we must specify which pins are
  //    rx and tx, where:
  //      rx = GPIO3
  //      tx = GPIO2
  Serial.begin(115200, 3, 2);

  timerLastPoll = millis();

}

/**
* @description: Private function to initialize the radio in Host mode
* @author: AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio::configureHost(void) {
  // Start the RFduinoGZLL in HOST mode
  RFduinoGZLL.begin(RFDUINOGZLL_ROLE_HOST);

  // Configure pins
  pinMode(OPENBCI_PIN_HOST_RESET,INPUT);
  pinMode(OPENBCI_PIN_HOST_LED,OUTPUT);

  // Turn LED on
  digitalWrite(OPENBCI_PIN_HOST_LED,HIGH);
}

/**
* @description: Private function to initialize the radio in Pass Through mode
* @author: AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio::configurePassThru(void) {

}
