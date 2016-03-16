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
*                 is intended to be called in the loop(). check and see if we
*                 are ready to send data from the bufferRadio. We should only
*                 send data when we are sure we got all the packets from
*                 RFduinoGZLL_onReceive()
* @return: Returns a TRUE if got all the packets and FALSE if not
* @author: AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radio::readRadio(void) {
  // Check to see if we got all the packets
  if (bufferPacketsToReceive == bufferPacketsReceived) {
    return true;
  } else {
    return false;
  }
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
  // How many bytes do we need to send?
  // Let's just send till we hit the write position
  while (bufferPositionReadRadio < bufferPositionWriteRadio) {
    Serial.write(bufferRadio[bufferPositionReadRadio]);
    bufferPositionReadRadio++;
  }
  bufferPositionWriteRadio = 0;
  bufferPositionReadRadio = 0;
}

/**
* @description: Called when readSerial returns true, if radio is Device we will
*                 send data to Host. If radio is Host, we will write Serial to
*                 bufferSerial.
* @author: AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio::writeSerial(void) {
  switch (radioMode) {
    case OPENBCI_MODE_HOST:
      writeSerialHost();
      break;
    case OPENBCI_MODE_DEVICE:
      writeSerialDevice():
      break;
    default:
      break;
  }
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
void OpenBCI_Radio::bufferCleanPacketBuffer(PacketBuffer *packetBuffer,int numberOfPacketsToSend) {
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
  buffer.numberOfPacketsToSend = 0;
  buffer.numberOfPacketsSent = 0;
}

/**
* @description: Private function to clean (clear/reset) the bufferRadio.
* @author: AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio::bufferCleanRadio(void) {
  bufferCleanChar(bufferRadio,OPENBCI_BUFFER_LENGTH);
  bufferPacketsReceived = 0;
  bufferPacketsToReceive = 0;
  bufferPositionReadRadio = 0;
  bufferPositionWriteRadio = 0;
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
*                 the bufferSerial. The important thing to note here is that
*                 this function stores the incoming data into groups of 32 for
*                 O(1) time to send data over the radio
* @author: AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio::writeSerialHost(void) {
  // store the total number of bytes to read, this is faster then calling
  //  Serial.available() every loop
  int numberOfBytesToRead = Serial.available();

  // Set the number of packets to 1 initally, it will only grow
  bufferSerial.numberOfPacketsToSend = 1;

  // We are going to call Serial.read() as many times as there are bytes on the
  //  buffer, on each call we are going to insert into
  while (numberOfBytesToRead > 0) {

    // If positionWrite >= OPENBCI_BUFFER_LENGTH need to wrap around
    if (currentPacketBufferSerial.positionWrite >= OPENBCI_MAX_PACKET_SIZE_BYTES) {
      // Go to the next packet
      bufferSerial.numberOfPacketsToSend++;
      // Did we run out of buffers?
      if (bufferSerial.numberOfPacketsToSend >= OPENBCI_MAX_NUMBER_OF_BUFFERS) {
        // this is bad, so something, throw error, explode... idk yet...
        //  for now set currentPacketBufferSerial to NULL
        currentPacketBufferSerial = NULL;
      } else {
        // move the pointer 1 struct
        currentPacketBufferSerial++;
      }
    }

    // We are only going to mess with the current packet if it's not null
    if (currentPacketBufferSerial) {
      // Store the byte to current buffer at write postition
      currentPacketBufferSerial.data[currentPacketBufferSerial.positionWrite] = Serial.read();

      // Increment currentPacketBufferSerial write postion
      currentPacketBufferSerial.positionWrite++;
    }

    // Decrement the number of Bytes to read
    numberOfBytesToRead--;
  }

  // Save the time this finished execution
  timeOfLastSerialRead = millis();
}



/***************************************************/
/** RFDUINOGZLL DELEGATE ***************************/
/***************************************************/
void RFduinoGZLL_onReceive(device_t device, int rssi, char *data, int len) {

  // Is this the first packet we are receiving?
  if (bufferPacketsToReceive == 0) {
    bufferPacketsToReceive = data[i];
  }

  // Increment the packets recieved, because every time this function gets
  //  called, we are getting a packet.
  bufferPacketsReceived++;

  // Write *data to bufferRadio right away
  for (int i = 0; i < len; i++) {
    // Check to see if we reached the end of the buffer
    if (bufferPositionWriteRadio >= OPENBCI_BUFFER_LENGTH) {
      // Eject! Mark this data ready to go and get out of here!
      bufferPacketsReceived = bufferPacketsToReceive;
    } else { // There is room!
      // #SaveThatData
      bufferRadio[bufferPositionWriteRadio] = data[i];
      // Increment the write position
      bufferPositionWriteRadio++;
    }
  }


  // Is there data in bufferSerial?
  if (bufferSerial.numberOfPacketsToSend > 0) {
    // Has enough time passed?
    if (timeOfLastSerialRead > OPENBCI_MAX_SERIAL_TIMEOUT_MS) {
      // Do we still have packets to send?
      if (bufferSerial.numberOfPacketsSent < bufferSerial.numberOfPacketsToSend) {
        // Grab the packet we want to send
        PacketBuffer *packetToSend = bufferSerial.packetBuffer[bufferSerial.numberOfPacketsSent];

        if (radioMode == OPENBCI_MODE_HOST) {
          // Send it to the device
          RFduinoGZLL.sendToDevice(device,packetToSend.data, packetToSend.positionWrite);
        } else {
          // Send it to the device
          RFduinoGZLL.sendToHost(NULL,0);

        }

        // Increment the number of packets we have sent
        bufferSerial.numberOfPacketsSent++;
      }
    }
  }
}
