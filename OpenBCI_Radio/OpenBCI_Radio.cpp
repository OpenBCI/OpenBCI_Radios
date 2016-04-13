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
    radioMode = OPENBCI_MODE_DEVICE; // Device mode
    radioChannel = 18; // Channel 18
    verbosePrintouts = false;
    debugMode = false; // Set true if doing dongle-dongle sim
}

/**
* @description The function that the radio will call in setup()
* @param: mode [unint8_t] - The mode the radio shall operate in
* @param: channelNumber [int8_t] - The channelNumber the RFduinoGZLL will
*           use to communicate with the other RFduinoGZLL.
*           NOTE: Must be from 2 - 25
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radio_Class::begin(uint8_t mode, int8_t channelNumber) {
    // Save global radio mode
    radioMode = mode;
    radioChannel = channelNumber;

    // configure radio
    configure(mode,channelNumber);

}

/**
* @description Private function to initialize the OpenBCI_Radio_Class object
* @param: mode [unint8_t] - The mode the radio shall operate in
* @param: channelNumber [int8_t] - The channelNumber the RFduinoGZLL will
*           use to communicate with the other RFduinoGZLL
* @author AJ Keller (@pushtheworldllc)
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
        bufferCleanSerial(OPENBCI_MAX_NUMBER_OF_BUFFERS);

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
* @description Private function to initialize the radio in Device mode
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio_Class::configureDevice(void) {
    // Start the RFduinoGZLL in DEVICE0 mode
    RFduinoGZLL.begin(RFDUINOGZLL_ROLE_DEVICE);

    // Configure pins

    if (!debugMode) {
        // BEGIN: To run host normally
        pinMode(OPENBCI_PIN_DEVICE_PCG, INPUT); //feel the state of the PIC with this pin
        // Start the serial connection. On the device we must specify which pins are
        //    rx and tx, where:
        //      rx = GPIO3
        //      tx = GPIO2
        Serial.begin(115200, 3, 2);
        // END: To run host normally
    } else { // Dongle to Dongle debug mode
        // BEGIN: To run host as device
        pinMode(OPENBCI_PIN_HOST_RESET,INPUT);
        pinMode(OPENBCI_PIN_HOST_LED,OUTPUT);
        digitalWrite(OPENBCI_PIN_HOST_LED,HIGH);
        Serial.begin(115200);
        // END: To run host as device
    }

    isHost = false;
    isDevice = true;
    // TODO: Implement init hand shaking
    // char msg[1];
    // msg[0] = RFDUINOGZLL_PACKET_INIT & 0xFF;
    // RFduinoGZLL.sendToHost(msg,1);
    pollRefresh();

}

/**
* @description Private function to initialize the radio in Host mode
* @author AJ Keller (@pushtheworldllc)
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

    isHost = true;
    isDevice = false;

    if (verbosePrintouts) {
        Serial.println("Host radio up");
    }
}

/**
* @description Private function to initialize the radio in Pass Through mode
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio_Class::configurePassThru(void) {

}

/********************************************/
/********************************************/
/************    HOST CODE    ***************/
/********************************************/
/********************************************/


/**
* @description Private function to handle a request to read serial as a host
* @return boolean - TRUE if there is data to read! FALSE if not...
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radio_Class::didPCSendDataToHost(void) {
    if (Serial.available() > 0) {
        return true;
    } else {
        return false;
    }
}

/**
* @description Private function to read data from serial port and write into
*                 the bufferSerial. The important thing to note here is that
*                 this function stores the incoming data into groups of 32 for
*                 O(1) time to send data over the radio
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio_Class::getSerialDataFromPCAndPutItInHostsSerialBuffer(void) {

    // Get everything from the serial port and store to bufferSerial
    bufferSerialFetch();

}

// boolean OpenBCI_Radio_Class::isTheHostsRadioBufferFilledWithAllThePacketsFromTheDevice(void) {
//     // Check to see if we got all the packets
//     if (bufferPacketsToReceive == bufferPacketsReceived) {
//         return true;
//     } else {
//         return false;
//     }
// }

/**
* @description Called when readRadio returns true. We always write the contents
*                 of bufferRadio over Serial.
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio_Class::writeTheHostsRadioBufferToThePC(void) {
    if (debugMode) {
        for (int j = 0; j < OpenBCI_Radio.bufferPositionWriteRadio; j++) {
            Serial.print(OpenBCI_Radio.bufferRadio[j]);
        }
        Serial.println();
    } else {
        for (int j = 0; j < OpenBCI_Radio.bufferPositionWriteRadio; j++) {
            Serial.write(OpenBCI_Radio.bufferRadio[j]);
        }
    }
    OpenBCI_Radio.bufferCleanRadio();
}

boolean OpenBCI_Radio_Class::hasItBeenTooLongSinceHostHeardFromDevice(void) {
    if (millis() > lastTimeHostHeardFromDevice + (OPENBCI_POLL_TIME_DURATION_MS * 2)) {
        return true;
    } else {
        return false;
    }
}

/********************************************/
/********************************************/
/***********    DEVICE CODE    **************/
/********************************************/
/********************************************/

/**
* @description Private function to handle a request to read serial as a device
* @return Returns TRUE if there is data to read! FALSE if not...
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radio_Class::didPicSendDeviceSerialData(void) {
    if (Serial.available() > 0) {
        // Serial.print("Pic sent device serial data of length ");
        // Serial.println(Serial.available());
        return true;
    } else {
        if (pollNow()) {
            pollHost();
        }
        return false;
    }
}

/**
* @description Moves data from the Pic to the devices Serial buffer (bufferSerial)
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio_Class::getSerialDataFromPicAndPutItInTheDevicesSerialBuffer(void) {
    // Get everything from the serial port and store to bufferSerial
    bufferSerialFetch();
}

boolean OpenBCI_Radio_Class::thereIsDataInSerialBuffer(void) {
    if (bufferSerial.numberOfPacketsSent < bufferSerial.numberOfPacketsToSend) {
        return true;
    } else {
        return false;
    }
}

/**
* @description Checks to see the last time serial data was written from the Pic
*               to the Device over serial was longer then the alloted time
* @returns true if yes, and false if no
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radio_Class::theLastTimeNewSerialDataWasAvailableWasLongEnough(void) {
    if (millis() > lastTimeNewSerialDataWasAvailable +  OPENBCI_MAX_SERIAL_TIMEOUT_MS) {
        return true;
    } else {
        return false;
    }
}

/**
* @description Private function to take data from the serial port and send it
*                 to the HOST
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio_Class::sendTheDevicesFirstPacketToTheHost(void) {

    // Is there data in bufferSerial?
    if (bufferSerial.numberOfPacketsToSend > 0 && bufferSerial.numberOfPacketsSent == 0) {
        // Build byteId
        // char byteIdMake(boolean isStreamPacket, int packetNumber, char *data, int length) {
        int packetNumber = bufferSerial.numberOfPacketsToSend - 1;
        char byteId = byteIdMake(false,packetNumber,bufferSerial.packetBuffer->data + 1, bufferSerial.packetBuffer->positionWrite - 1);
        //
        // // Add the byteId to the packet
        bufferSerial.packetBuffer->data[0] = byteId;

        // Send back some data
        RFduinoGZLL.sendToHost(bufferSerial.packetBuffer->data, bufferSerial.packetBuffer->positionWrite);
        pollRefresh();

        // We know we just sent the first packet
        bufferSerial.numberOfPacketsSent = 1;

        if (verbosePrintouts) {
            Serial.print("Si->"); Serial.print(packetNumber); Serial.print(":"); Serial.println(bufferSerial.packetBuffer->positionWrite);
        }
    }
}

/**
* @description Check to see if there is data in the bufferRadio. This function
*                 is intended to be called in the loop(). check and see if we
*                 are ready to send data from the bufferRadio. We should only
*                 send data when we are sure we got all the packets from
*                 RFduinoGZLL_onReceive()
* @returns: Returns a TRUE if got all the packets and FALSE if not
* @author AJ Keller (@pushtheworldllc)
*/
// boolean OpenBCI_Radio_Class::isTheDevicesRadioBufferFilledWithAllThePacketsFromTheHost(void) {
//     // Check to see if we got all the packets
//     if (bufferPacketsToReceive == bufferPacketsReceived) {
//         return true;
//     } else {
//         return false;
//     }
// }

/**
* @description Called when readRadio returns true. We always write the contents
*                 of bufferRadio over Serial.
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio_Class::writeTheDevicesRadioBufferToThePic(void) {
    if (debugMode) {
        for (int j = 0; j < bufferPositionWriteRadio; j++) {
            Serial.print(bufferRadio[j]);
        }
        Serial.println();
    } else {
        for (int j = 0; j < bufferPositionWriteRadio; j++) {
            Serial.write(bufferRadio[j]);
        }
    }
    bufferCleanRadio();
}

/********************************************/
/********************************************/
/********    COMMON MEHTOD CODE    **********/
/********************************************/
/********************************************/

/**
* @description Writes a buffer to the serial port of a given length
* @param buffer [char *] The buffer you want to write out
* @param length [int] How many bytes to you want to write out?
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio_Class::writeBufferToSerial(char *buffer, int length) {
    // Make sure we don't seg fault
    if (buffer == NULL) return;

    // Loop through all bytes in buffer
    for (int i = 0; i < length; i++) {
        Serial.write(buffer[i]);
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
    temp[0] = (char)OPENBCI_STREAM_BYTE_START;

    // Move the bytes from data to temp
    memcpy(temp + 1, data, OPENBCI_MAX_DATA_BYTES_IN_PACKET);

    // Set stop byte
    temp[OPENBCI_MAX_PACKET_SIZE_STREAM_BYTES - 1] = (char)OPENBCI_STREAM_BYTE_STOP;

    char *buf = temp;
    // Send it!
    writeBufferToSerial(buf,OPENBCI_MAX_PACKET_SIZE_STREAM_BYTES);
}

/**
* @description Private function to clear the given buffer of length
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio_Class::bufferCleanChar(char *buffer, int bufferLength) {
    for (int i = 0; i < bufferLength; i++) {
        buffer[i] = 0;
    }
}

/**
* @description Private function to clean a PacketBuffer.
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio_Class::bufferCleanPacketBuffer(PacketBuffer *packetBuffer, int numberOfPackets) {
    for(int i = 0; i < numberOfPackets; i++) {
        packetBuffer[i].positionRead = 0;
        packetBuffer[i].positionWrite = 1;
        // TODO: Determine if this is really even important... since we are
        //  resetting the read and write positions, i think over writing is ok
        // bufferCleanChar(packetBuffer[i].data, OPENBCI_MAX_PACKET_SIZE_BYTES);
    }
}

/**
* @description Private function to clean (clear/reset) a Buffer.
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio_Class::bufferCleanBuffer(Buffer *buffer, int numberOfPacketsToClean) {
    bufferCleanPacketBuffer(buffer->packetBuffer,numberOfPacketsToClean);
    buffer->numberOfPacketsToSend = 0;
    buffer->numberOfPacketsSent = 0;
}

/**
* @description Private function to clean (clear/reset) the bufferRadio.
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio_Class::bufferCleanRadio(void) {
    bufferCleanChar(bufferRadio,OPENBCI_BUFFER_LENGTH);
    bufferPacketsReceived = 0;
    bufferPacketsToReceive = 0;
    bufferPositionReadRadio = 0;
    bufferPositionWriteRadio = 0;
    isTheDevicesRadioBufferFilledWithAllThePacketsFromTheHost = false;
    isTheHostsRadioBufferFilledWithAllThePacketsFromTheDevice = false;
}

/**
 * @description Private function to clean (clear/reset) the bufferSerial.
 * @param - `numberOfPacketsToClean` - [int] - The number of packets you want to
 *      clean, for example, on init, we would clean all packets, but on cleaning
 *      from the RFduinoGZLL_onReceive() we would only clean the number of
 *      packets acutally used.
 * @author AJ Keller (@pushtheworldllc)
 */
void OpenBCI_Radio_Class::bufferCleanSerial(int numberOfPacketsToClean) {
    bufferCleanBuffer(&bufferSerial, numberOfPacketsToClean);
    currentPacketBufferSerial = bufferSerial.packetBuffer;
    previousPacketNumber = 0;
}

/**
* @description Moves bytes on serial port into bufferSerial
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio_Class::bufferSerialFetch(void) {
    // store the total number of bytes to read, this is faster then calling
    //  Serial.available() every loop
    // int numberOfBytesToRead = Serial.available();

    // Set the number of packets to 1 initally, it will only grow
    if (bufferSerial.numberOfPacketsToSend == 0) {
        bufferSerial.numberOfPacketsToSend = 1;
    }
    //
    // int bytesThisRead = ;

    // We are going to call Serial.read() as many times as there are bytes on the
    //  buffer, on each call we are going to insert into
    // Serial.print("Read "); Serial.print(bytesThisRead); Serial.println(" bytes");
    while (Serial.available()) {

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

            // Serial.print(currentPacketBufferSerial->data[currentPacketBufferSerial->positionWrite]);
            // Increment currentPacketBufferSerial write postion
            currentPacketBufferSerial->positionWrite++;
        } else {
            if (verbosePrintouts) {
                Serial.print("BO:"); Serial.println(Serial.read());
            } else {
                Serial.read();
            }
        }

        if (isDevice) {
            pollRefresh();
        }

        // Save time of last serial read...
        lastTimeNewSerialDataWasAvailable = millis();
    }
}

/**
* @description Strips and gets the packet number from a byteId
* @param byteId [char] a byteId (see ::byteIdMake for description of bits)
* @returns [int] the packetNumber
*/
int OpenBCI_Radio_Class::byteIdGetPacketNumber(char byteId) {
    return (int)((byteId & 0x78) >> 3);
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
* @description Determines if this byteId is a stream byte
* @param byteId [char] a byteId (see ::byteIdMake for description of bits)
* @returns [int] the check sum
*/
boolean OpenBCI_Radio_Class::byteIdGetIsStream(char byteId) {
    return byteId > 0x7F;
}

/**
* @description Creates a byteId for sending data over the RFduinoGZLL
* @param isStreamPacket [boolean] Set true if this is a streaming packet
* @param packetNumber [int] What number packet are you trying to send?
* @param data [char *] The data you want to send. NOTE: Do not send the address
*           of the entire buffer, send this method address of buffer + 1
* @param length [int] The length of the data buffer
* @returns [char] The newly formed byteId where a byteId is defined as
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
* @param data [char *] The data packet from on_recieve
* @param len [int] The length of data packet
* @returns boolean if equal
*/
boolean OpenBCI_Radio_Class::checkSumsAreEqual(char *data, int len) {
    char expectedCheckSum = OpenBCI_Radio.byteIdGetCheckSum(data[0]);

    char calculatedCheckSum = OpenBCI_Radio.checkSumMake(data + 1,len - 1);

    return expectedCheckSum == calculatedCheckSum;
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

    if (data == NULL) return 0x00;

    // initialize count to 0
    count = 0;

    // Use a do-while loop to execute
    do {
        sum = sum + data[count];
        count++;
    } while(--length);

    // Bit smash/smush with unsigned int hack
    sum = -sum;

    return (sum & 0x07);
}

/**
* @description Send a NULL packet to the HOST
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio_Class::pollHost(void) {
    RFduinoGZLL.sendToHost(NULL,0);
    pollRefresh();
    // Serial.println(".");
}

/**
* @description Has enough time passed since the last poll
* @returns [boolean]
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radio_Class::pollNow(void) {
    return millis() - timeOfLastPoll > OPENBCI_POLL_TIME_DURATION_MS;
}

/**
* @description Reset the time since last packent sent to HOST
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radio_Class::pollRefresh(void) {
    timeOfLastPoll = millis();
}

OpenBCI_Radio_Class OpenBCI_Radio;


/********************************************/
/********************************************/
/*******    RFDUINOGZLL DELEGATE    *********/
/********************************************/
/********************************************/
void RFduinoGZLL_onReceive(device_t device, int rssi, char *data, int len) {



    char msg[1]; // the message to ack back
    // will either send above message or data from buffer serial
    boolean goodToAddPacketToRadioBuffer = true;
    boolean willSendDataFromBufferSerial = false;
    boolean immediatelySendStreamPacket = false;
    boolean willSendMsg = false;

    if (OpenBCI_Radio.isHost) {
        OpenBCI_Radio.lastTimeHostHeardFromDevice = millis();
    }

    if (len == 1) { // this is a radio comm packet
        /**************************************/
        // CAN INITIATE DATA SEND HERE
        /**************************************/
        char byte = data[0];
        if (byte == (char)RFDUINOGZLL_PACKET_BAD_CHECK_SUM) {
            // Resend the last sent packet
            OpenBCI_Radio.bufferSerial.numberOfPacketsSent--;

            willSendDataFromBufferSerial = true;
            if (OpenBCI_Radio.verbosePrintouts) {
                Serial.println("R<-B");
            }
        } else if (byte == (char)RFDUINOGZLL_PACKET_MISSED) {
            // Start the page transmission over again
            OpenBCI_Radio.bufferSerial.numberOfPacketsSent = 0;

            willSendDataFromBufferSerial = true;
            if (OpenBCI_Radio.verbosePrintouts) {
                Serial.println("R<-M");
            }
        }
        // TODO: Implement init handshaking
        // else if (byte == (char)RFDUINOGZLL_PACKET_INIT) {
        //     if (OpenBCI_Radio.isHost) {
        //         msg[0] = RFDUINOGZLL_PACKET_INIT & 0xFF;
        //         RFduinoGZLL.sendToDevice(device,msg, 1);
        //     }
        //     if (OpenBCI_Radio.verbosePrintouts) {
        //         Serial.println("R<-I");
        //     }
        // }
    } else if (len > 1) {
        /**************************************/
        // CANN SEND DATA HERE
        /**************************************/
        // is it a stream packet?
        boolean gotLastPacket = false;

        int packetNumber = OpenBCI_Radio.byteIdGetPacketNumber(data[0]);
        if (OpenBCI_Radio.verbosePrintouts) {
            Serial.print("R<-");Serial.println(packetNumber);
        }
        // Verify the checksums are equal
        if (OpenBCI_Radio.checkSumsAreEqual(data,len)) {
            // This first statment asks if this is a last packet and the previous packet was 0 too
            if (packetNumber == 0 && OpenBCI_Radio.previousPacketNumber == 0) {
                // Serial.println("Got final packet.");
                gotLastPacket = true;

            } else {
                if (packetNumber > 0 && OpenBCI_Radio.previousPacketNumber == 0) {
                    // This is the first of multiple packets we are recieving
                    OpenBCI_Radio.previousPacketNumber = packetNumber;

                } else {
                    // This is not the first packet we are reciving of this page
                    if (OpenBCI_Radio.previousPacketNumber - packetNumber == 1) { // Normal...
                        // update
                        OpenBCI_Radio.previousPacketNumber = packetNumber;

                        // Is this the last packet?
                        if (packetNumber == 0) {
                            // Serial.println("Last packet of multiple.");
                            gotLastPacket = true;
                        }

                    } else {
                        goodToAddPacketToRadioBuffer = false;
                        // We missed a packet, send resend message
                        msg[0] = RFDUINOGZLL_PACKET_MISSED & 0xFF;

                        // reset ring buffer to start
                        OpenBCI_Radio.bufferPositionWriteRadio = 0;

                        // Reset the packet state
                        OpenBCI_Radio.previousPacketNumber = 0;
                        if (OpenBCI_Radio.verbosePrintouts) {
                            Serial.println("S->M");
                        }
                    }
                }
            }
        } else {
            goodToAddPacketToRadioBuffer = false;
            msg[0] = RFDUINOGZLL_PACKET_BAD_CHECK_SUM & 0xFF;
            if (OpenBCI_Radio.verbosePrintouts) {
                Serial.println("S->B");
            }
        }

        if (goodToAddPacketToRadioBuffer) {
            if (OpenBCI_Radio.byteIdGetIsStream(data[0])) {
                // blah do stream stuff
            } else {
                for (int i = 1; i < len; i++) { // skip the byteId
                    if (OpenBCI_Radio.bufferPositionWriteRadio < OPENBCI_BUFFER_LENGTH) { // Check for to prevent overflow
                        OpenBCI_Radio.bufferRadio[OpenBCI_Radio.bufferPositionWriteRadio] = data[i];
                        OpenBCI_Radio.bufferPositionWriteRadio++;
                    }
                }
                if (gotLastPacket) {
                    // flag contents of radio buffer to be printed!
                    OpenBCI_Radio.isTheHostsRadioBufferFilledWithAllThePacketsFromTheDevice = true;
                    OpenBCI_Radio.isTheDevicesRadioBufferFilledWithAllThePacketsFromTheHost = true;
                }
            }
            if (OpenBCI_Radio.isDevice) {
                OpenBCI_Radio.pollHost();
            }

            if (OpenBCI_Radio.verbosePrintouts) {
                Serial.println("S->N");
            }
        } else { // We got a problem

            if (OpenBCI_Radio.isHost) {
                RFduinoGZLL.sendToDevice(device,msg,1);
            } else {
                RFduinoGZLL.sendToHost(msg,1);
                OpenBCI_Radio.pollRefresh();
            }

        }
    } else { // This is a NULL byte ack
        /**************************************/
        // CAN SEND DATA HERE ONLY IF FIRST PACKET
        /**************************************/
        // if (OpenBCI_Radio.bufferSerial.numberOfPacketsToSend > 0 && OpenBCI_Radio.bufferSerial.numberOfPacketsSent == 0) {
        //     if (OpenBCI_Radio.theLastTimeNewSerialDataWasAvailableWasLongEnough()) {
        //         willSendDataFromBufferSerial = true;
        //     }
        // }

        // More packets to send?
        if (OpenBCI_Radio.bufferSerial.numberOfPacketsSent < OpenBCI_Radio.bufferSerial.numberOfPacketsToSend) {
            if (OpenBCI_Radio.theLastTimeNewSerialDataWasAvailableWasLongEnough()) {
                willSendDataFromBufferSerial = true;
            }

        } else { // We are done sending packets
            // Clean bufferSerial
            if (OpenBCI_Radio.bufferSerial.numberOfPacketsSent == OpenBCI_Radio.bufferSerial.numberOfPacketsToSend && OpenBCI_Radio.bufferSerial.numberOfPacketsToSend != 0) {
                // Serial.println("Cleaning Hosts's bufferSerial");
                // Clear buffer
                if (OpenBCI_Radio.verbosePrintouts) Serial.print("C");
                OpenBCI_Radio.bufferCleanSerial(OpenBCI_Radio.bufferSerial.numberOfPacketsSent);
            }
        }
    }

    if (willSendDataFromBufferSerial) {
        // Build byteId
        // char byteIdMake(boolean isStreamPacket, int packetNumber, char *data, int length)
        int packetNumber = OpenBCI_Radio.bufferSerial.numberOfPacketsToSend - OpenBCI_Radio.bufferSerial.numberOfPacketsSent - 1;
        char byteId = OpenBCI_Radio.byteIdMake(false,packetNumber,(OpenBCI_Radio.bufferSerial.packetBuffer + OpenBCI_Radio.bufferSerial.numberOfPacketsSent)->data + 1, (OpenBCI_Radio.bufferSerial.packetBuffer + OpenBCI_Radio.bufferSerial.numberOfPacketsSent)->positionWrite - 1);

        // Add the byteId to the packet
        (OpenBCI_Radio.bufferSerial.packetBuffer + OpenBCI_Radio.bufferSerial.numberOfPacketsSent)->data[0] = byteId;

        // Send back some data!
        if (OpenBCI_Radio.isHost) {
            RFduinoGZLL.sendToDevice(device,(OpenBCI_Radio.bufferSerial.packetBuffer + OpenBCI_Radio.bufferSerial.numberOfPacketsSent)->data, (OpenBCI_Radio.bufferSerial.packetBuffer + OpenBCI_Radio.bufferSerial.numberOfPacketsSent)->positionWrite);
        } else { //isDevice
            RFduinoGZLL.sendToHost((OpenBCI_Radio.bufferSerial.packetBuffer + OpenBCI_Radio.bufferSerial.numberOfPacketsSent)->data, (OpenBCI_Radio.bufferSerial.packetBuffer + OpenBCI_Radio.bufferSerial.numberOfPacketsSent)->positionWrite);
            OpenBCI_Radio.pollRefresh();
        }

        if (OpenBCI_Radio.verbosePrintouts) {
            Serial.print("S->"); Serial.println(packetNumber);
        }

        OpenBCI_Radio.bufferSerial.numberOfPacketsSent++;
    }

}
