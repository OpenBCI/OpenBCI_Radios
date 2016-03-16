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
  // Save global radio mode
  radioMode = mode;

  // configure radio
  configure(mode,channelNumber);

}

/**
* @description: Check to see if there is data in the bufferRadio. This function
*                 is intended to be called in the loop()
* @return: Returns a TRUE if there is data and FALSE if not
* @author: AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radio::readRadio(void) {

}

/**
* @description: Check to see if there is serial data available. This function
*                 is intended to be called in the loop()
* @return: Returns a TRUE if there is data and FALSE if not
* @author: AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radio::readSerial(void) {
  if (radioMode == OPENBCI_MODE_HOST) {
    return readSerialHost();
  } else if (radioMode == OPENBCI_MODE_DEVICE) {
    return readSerialDevice();
  } else {
    return false;
  }
}

/**
* @description: Called when readRadio returns true. We always write the contents
*                 of bufferRadio over Serial.
* @author: AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio::writeRadio(void) {

}

/**
* @description: Called when readSerial returns true, if radio is Device we will
*                 send data to Host. If radio is Host, we will write Serial to
*                 bufferSerial.
* @author: AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio::writeSerial(void) {

}

/***************************************************/
/** PRIVATE METHODS ********************************/
/***************************************************/

/**
* @description: Private function to clear the given buffer of length
* @author: AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio::bufferCleanChar(char *buffer, int bufferLength) {
  for (int i = 0; i < bufferLength; i++) {
    buffer[i] = 0;
  }
}

/**
* @description: Private function to clean a PacketBuffer.
* @author: AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio::bufferCleanPacketBuffer(PacketBuffer *packetBuffer,int numberOfPackets) {
  for(int i = 0; i < numberOfPackets; i++) {
    packetBuffer[i].positionRead = 0;
    packetBuffer[i].positionWrite = 0;
    bufferCleanChar(packetBuffer[i].data, OPENBCI_MAX_PACKET_SIZE_BYTES);
  }
}

/**
* @description: Private function to clean (clear/reset) a Buffer.
* @author: AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio::bufferCleanBuffer(Buffer *buffer) {
  bufferCleanPacketBuffer(buffer.packetBuffer,OPENBCI_MAX_NUMBER_OF_BUFFERS);
  buffer.numberOfPackets = 0;
}

/**
* @description: Private function to clean (clear/reset) the bufferRadio.
* @author: AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio::bufferCleanRadio(void) {
  bufferCleanBuffer(bufferRadio);
}

/**
* @description: Private function to clean (clear/reset) the bufferSerial.
* @author: AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio::bufferCleanSerial(void) {
  bufferCleanBuffer(bufferSerial);
  currentPacketBufferSerial = bufferSerial.packetBuffer[0];
}

/**
* @description: Private function to initialize the OpenBCI_Radio object
* @param: mode [unint8_t] - The mode the radio shall operate in
* @param: channelNumber [int8_t] - The channelNumber the RFduinoGZLL will
*           use to communicate with the other RFduinoGZLL
* @author: AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio::configure(unint8_t mode, int8_t channelNumber) {
  // Quickly check to see if in pass through mode, if so, call and dip out of func
  if (mode == OPENBCI_MODE_PASS_THRU) {
    configurePassThru();
  } else { // we are either dealing with a Host or a Device
    // We give the oppertunity to call any 'universal' code, rather code, that
    //    gets set up the same on both Host and Device
    RFduinoGZLL.channel = channelNumber;

    // get the buffers ready
    bufferCleanRadio();
    bufferCleanSerial();

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

  // timeOfLastPoll = millis();

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

  // Open the Serial connection
  Serial.begin(115200);

}

/**
* @description: Private function to initialize the radio in Pass Through mode
* @author: AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio::configurePassThru(void) {

}

/**
* @description: Private function to handle a request to read serial as a host
* @return: Returns TRUE if there is data to read! FALSE if not...
* @author: AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radio::readSerialHost(void) {
  if (Serial.available() > 0) {
    return true;
  } else {
    return false;
  }
}

/**
* @description: Private function to read data from serial port and write into
*                 the bufferSerial
* @author: AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio::writeSerialHost(void) {
  // store the total number of bytes to read, this is faster then calling
  //  Serial.available() every loop
  int numberOfBytesToRead = Serial.available();

  // We are going to call Serial.read() as many times as there are bytes on the
  //  buffer, on each call we are going to insert into
  while (numberOfBytesToRead > 0) {

    // If bufferPositionWriteSerial >= OPENBCI_BUFFER_LENGTH need to wrap around
    if (currentPacketBufferSerial->positionWrite >= OPENBCI_MAX_PACKET_SIZE_BYTES) {
      // Go to the next packet
      bufferSerial->numberOfPackets++;
      // Did we run out of buffers?
      if (bufferSerial->numberOfPackets >= OPENBCI_MAX_NUMBER_OF_BUFFERS) {
        // this is bad, so something, throw error, explode... idk yet...
        //  for now set currentPacketBufferSerial to NULL
        currentPacketBufferSerial = NULL;
      } else {
        // move the pointer 1 struct
        currentPacketBufferSerial++;
      }
    }

    // Store the byte to current buffer at write postition 
    currentPacketBufferSerial.data[currentPacketBufferSerial->positionWrite] = Serial.read();

    // Increment currentPacketBufferSerial write postion
    currentPacketBufferSerial->positionWrite++;

    // Decrement the number of Bytes to read
    numberOfBytesToRead--;
  }
}
