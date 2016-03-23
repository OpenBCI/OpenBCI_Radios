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
OpenBCI_Radio_Class::OpenBCI_Radio_Class() {
    // Set defaults
    mode = OPENBCI_MODE_DEVICE; // Device mode
    channelNumber = 18; // Channel 18
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
boolean OpenBCI_Radio_Class::begin(uint8_t mode, int8_t channelNumber) {
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
boolean OpenBCI_Radio_Class::readRadio(void) {
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
boolean OpenBCI_Radio_Class::readSerial(void) {
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
void OpenBCI_Radio_Class::writeRadio(void) {
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
void OpenBCI_Radio_Class::writeSerial(void) {
    switch (radioMode) {
        case OPENBCI_MODE_HOST:
        writeSerialHost();
        break;
        case OPENBCI_MODE_DEVICE:
        writeSerialDevice();
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
void OpenBCI_Radio_Class::bufferCleanChar(char *buffer, int bufferLength) {
    for (int i = 0; i < bufferLength; i++) {
        buffer[i] = 0;
    }
}

/**
* @description: Private function to clean a PacketBuffer.
* @author: AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio_Class::bufferCleanPacketBuffer(PacketBuffer *packetBuffer,int numberOfPackets) {
    for(int i = 0; i < numberOfPackets; i++) {
        packetBuffer[i].positionRead = 0;
        packetBuffer[i].positionWrite = 1;
        bufferCleanChar(packetBuffer[i].data, OPENBCI_MAX_PACKET_SIZE_BYTES);
    }
}

/**
* @description: Private function to clean (clear/reset) a Buffer.
* @author: AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio_Class::bufferCleanBuffer(Buffer *buffer) {
    bufferCleanPacketBuffer(buffer->packetBuffer,OPENBCI_MAX_NUMBER_OF_BUFFERS);
    buffer->numberOfPacketsToSend = 0;
    buffer->numberOfPacketsSent = 0;
}

/**
* @description: Private function to clean (clear/reset) the bufferRadio.
* @author: AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio_Class::bufferCleanRadio(void) {
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
void OpenBCI_Radio_Class::bufferCleanSerial(void) {
    bufferCleanBuffer(&bufferSerial);
    currentPacketBufferSerial = bufferSerial.packetBuffer;
}

/**
* @description: Moves bytes on serial port into bufferSerial
* @author: AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio_Class::bufferSerialFetch(void) {
    // store the total number of bytes to read, this is faster then calling
    //  Serial.available() every loop
    int numberOfBytesToRead = Serial.available();

    // Set the number of packets to 1 initally, it will only grow
    bufferSerial.numberOfPacketsToSend = 1;

    // We are going to call Serial.read() as many times as there are bytes on the
    //  buffer, on each call we are going to insert into
    while (numberOfBytesToRead > 0) {

        // If positionWrite >= OPENBCI_BUFFER_LENGTH need to wrap around
        if (currentPacketBufferSerial->positionWrite >= OPENBCI_MAX_PACKET_SIZE_BYTES) {
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
            currentPacketBufferSerial->data[currentPacketBufferSerial->positionWrite] = Serial.read();

            // Increment currentPacketBufferSerial write postion
            currentPacketBufferSerial->positionWrite++;
        }

        // Decrement the number of Bytes to read
        numberOfBytesToRead--;
    }
}

/**
* @description Strips and gets the packet number from a byteId
* @param byteId [char] a byteId (see ::byteIdMake for description of bits)
* @returns [int] the packetNumber
*/
int OpenBCI_Radio_Class::byteIdGetPacketNumber(char byteId) {
    return (int)byteId & 0x78;
}

/**
* @description Strips and gets the check sum from a byteId
* @param byteId [char] a byteId (see ::byteIdMake for description of bits)
* @returns [int] the check sum
*/
char OpenBCI_Radio_Class::byteIdGetCheckSum(char byteId) {
    return byteId & 0x07;
}

/**
* @description Creates a byteId for sending data over the RFduinoGZLL
* @param isStreamPacket [boolean] Set true if this is a streaming packet
* @param packetNumber [int] What number packet are you trying to send?
* @param data [char *] The data you want to send. NOTE: Do not send the address
*           of the entire buffer, send this method address of buffer + 1
* @param length [int] The length of the data buffer
* @return [char] The newly formed byteId where a byteId is defined as
*           Bit 7 - Streaming byte packet
*           Bits[6:3] - Packet count
*           Bits[2:0] - The check sum
* @author AJ Keller (@pushtheworldllc)
*/
char OpenBCI_Radio_Class::byteIdMake(boolean isStreamPacket, int packetNumber, char *data, int length) {
    // Set output initially equal to 0
    char output = 0x00;

    // Set first bit if this is a streaming packet
    if  (isStreamPacket) output = output | 0x80;

    // Set packet count bits Bits[6:3] NOTE: 0xFF is error
    // convert int to char then shift then or
    output = output | ((packetNumber & 0x0F) << 3);

    // Set the check sum bits Bits[2:0]
    output = output | checkSumMake(data,length);

    return output;
}

/**
* @description Convience method to compare two chars for equality
* @param checkSum1 [char] The first char to compare
* @param checkSum2 [char] The second char to compare
* @return boolean if equal
*/
boolean OpenBCI_Radio_Class::checkSumIsEqual(char checkSum1, char checkSum2) {
    return checkSum1 == checkSum2;
}

/**
* @description makes a check sum based off the data of specified length
* @param data [char *] array of bytes to create check sum with
* @param length [int] length of data (you better make sure this is right!)
* @returns [char] of the check sum
* @author AJ Keller (@pushtheworldllc) with Leif Percifield
*/
char OpenBCI_Radio_Class::checkSumMake(char *data, int length) {
    int count;
    unsigned int sum = 0;

    // initialize count to 0
    count = 0;

    // Use a do-while loop to execute
    do {
        sum = sum + data[count];
    } while(--length);

    // Bit smash/smush with unsigned int hack
    sum = -sum;

    return (sum & 0x07);
}

/**
* @description: Private function to initialize the OpenBCI_Radio_Class object
* @param: mode [unint8_t] - The mode the radio shall operate in
* @param: channelNumber [int8_t] - The channelNumber the RFduinoGZLL will
*           use to communicate with the other RFduinoGZLL
* @author: AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio_Class::configure(uint8_t mode, int8_t channelNumber) {
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
void OpenBCI_Radio_Class::configureDevice(void) {
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
void OpenBCI_Radio_Class::configureHost(void) {
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
* @description Private function to initialize the radio in Pass Through mode
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio_Class::configurePassThru(void) {

}

void OpenBCI_Radio_Class::readRadioDevice(void) {
    if (bufferPositionWriteRadio > bufferPositionReadRadio) {
        return true;
    }
}

/**
* @description Private function to handle a request to read serial as a device
* @return Returns TRUE if there is data to read! FALSE if not...
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radio_Class::readSerialDevice(void) {
    if (Serial.available() > 0) {
        return true;
    } else {
        return false;
    }
}

/**
* @description Private function to handle a request to read serial as a host
* @return boolean - TRUE if there is data to read! FALSE if not...
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radio_Class::readSerialHost(void) {
    if (Serial.available() > 0) {
        return true;
    } else {
        return false;
    }
}



/**
* @description Sends a data of length 31 to the board in OpenBCI V3 data format
* @param data [char *] The 31 byte buffer
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio_Class::writeStreamPacket(char *data) {
    // Create temp char array
    char temp[OPENBCI_MAX_PACKET_SIZE_STREAM_BYTES];

    // Set start byte
    temp[0] = OPENBCI_STREAM_BYTE_START;

    // Move the bytes from data to temp
    memcpy(temp + 1, data, OPENBCI_MAX_DATA_BYTES_IN_PACKET);

    // Set stop byte
    temp[OPENBCI_MAX_PACKET_SIZE_STREAM_BYTES - 1] = OPENBCI_STREAM_BYTE_STOP;

    // Send it!
    Serial.write(temp,OPENBCI_MAX_PACKET_SIZE_STREAM_BYTES);
}

/**
* @description Private function to take data from the serial port and send it
*                 to the HOST
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio_Class::writeSerialDevice(void) {

    // Get everything from the serial port and store to bufferSerial
    bufferSerialFetch();

    // Is there data in bufferSerial?
    if (bufferSerial.numberOfPacketsToSend > 0) {
        // Has enough time passed?
        if (timeOfLastSerialRead > OPENBCI_MAX_SERIAL_TIMEOUT_MS) {
            // Do we still have packets to send?
            while (bufferSerial.numberOfPacketsSent < bufferSerial.numberOfPacketsToSend) {
                // Grab the packet we want to send
                PacketBuffer *packetToSend = bufferSerial.packetBuffer + bufferSerial.numberOfPacketsSent;

                // Send that buffer to
                RFduinoGZLL.sendToHost(packetToSend->data,packetToSend->positionWrite);

                // Increment the number of packets we have sent
                bufferSerial.numberOfPacketsSent++;
            }
        }
    }
}

/**
* @description: Private function to read data from serial port and write into
*                 the bufferSerial. The important thing to note here is that
*                 this function stores the incoming data into groups of 32 for
*                 O(1) time to send data over the radio
* @author: AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio_Class::writeSerialHost(void) {

    // Get everything from the serial port and store to bufferSerial
    bufferSerialFetch();

    // Save the time this finished execution
    timeOfLastSerialRead = millis();
}

OpenBCI_Radio_Class OpenBCI_Radio;

/***************************************************/
/** RFDUINOGZLL DELEGATE ***************************/
/***************************************************/
void RFduinoGZLL_onReceive(device_t device, int rssi, char *data, int len) {

    // Is this a resend case?
    if (data[0] == 0xFF) {
        // Resend the last packet
        return;
    }

    // Grab the check sum
    char checkSumActual = OpenBCI_Radio.byteIdGetCheckSum(data[0]);

    // Compute the check sum
    char checkSumExpected = OpenBCI_Radio.checkSumMake(data + 1, len > 0 ? len - 1 : 0);

    // Do the check sums check out?
    boolean checkSumTestPassed = checkSumActual == checkSumExpected;

    if (checkSumTestPassed) {
        // Is this a streaming packet?
        if (OpenBCI_Radio.radioMode == OPENBCI_MODE_HOST && data[0] > 127) {
            OpenBCI_Radio.writeStreamPacket(data + 1);
        } else { // We got some time
            // Is this the first packet we are receiving?
            if (OpenBCI_Radio.bufferPacketsToReceive == 0) {
                OpenBCI_Radio.bufferPacketsToReceive = OpenBCI_Radio.byteIdGetPacketNumber(data[0]);
            }

            // Increment the packets recieved, because every time this function gets
            //  called, we are getting a packet.
            OpenBCI_Radio.bufferPacketsReceived++;

            // Write *data to bufferRadio right away
            for (int i = 0; i < len; i++) {
                // Check to see if we reached the end of the buffer
                if (OpenBCI_Radio.bufferPositionWriteRadio >= OPENBCI_BUFFER_LENGTH) {
                    // Eject! Mark this data ready to go and get out of here!
                    OpenBCI_Radio.bufferPacketsReceived = OpenBCI_Radio.bufferPacketsToReceive;
                } else { // There is room!
                    // #SaveThatData
                    OpenBCI_Radio.bufferRadio[OpenBCI_Radio.bufferPositionWriteRadio] = data[i];
                    // Increment the write position
                    OpenBCI_Radio.bufferPositionWriteRadio++;
                }
            }

            // If HOST, is there data in bufferSerial?
            if (OpenBCI_Radio.radioMode == OPENBCI_MODE_HOST) {
                // Is there data in bufferSerial?
                if (OpenBCI_Radio.bufferSerial.numberOfPacketsToSend > 0) {
                    // Has enough time passed?
                    if (OpenBCI_Radio.timeOfLastSerialRead > OPENBCI_MAX_SERIAL_TIMEOUT_MS) {
                        // Do we still have packets to send?
                        if (OpenBCI_Radio.bufferSerial.numberOfPacketsSent < OpenBCI_Radio.bufferSerial.numberOfPacketsToSend) {
                            // Grab the packet we want to send
                            PacketBuffer *packetToSend = OpenBCI_Radio.bufferSerial.packetBuffer + OpenBCI_Radio.bufferSerial.numberOfPacketsSent;

                            // Send back some data!
                            RFduinoGZLL.sendToDevice(device,packetToSend->data, packetToSend->positionWrite);

                            // Increment the number of packets we have sent
                            OpenBCI_Radio.bufferSerial.numberOfPacketsSent++;
                        }
                    }
                }
            } else {
                // Send NULL packet if we are the device
                RFduinoGZLL.sendToHost(NULL,0);
            }
        }
    } else { // Send error message
        if (OpenBCI_Radio.radioMode == OPENBCI_MODE_HOST) {
            RFduinoGZLL.sendToDevice(device, 0xFF, 1);
        } else {
            RFduinoGZLL.sendToHost(0xFF,1);
        }
    }
}
