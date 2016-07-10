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

#include "OpenBCI_Radios.h"

// CONSTRUCTOR
OpenBCI_Radios_Class::OpenBCI_Radios_Class() {
    // Set defaults
    radioMode = OPENBCI_MODE_DEVICE; // Device mode
    radioChannel = 25; // Channel 18
    debugMode = false; // Set true if doing dongle-dongle sim
    ackCounter = 0;
    lastTimeHostHeardFromDevice = 0;
    lastTimeSerialRead = 0;
    systemUp = false;
}

/**
* @description The function that the radio will call in setup()
* @param: mode [unint8_t] - The mode the radio shall operate in
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::begin(uint8_t mode) {
    // Save global radio mode
    radioMode = mode;

    // configure radio
    configure(mode,radioChannel);
}

/**
* @description The function that the radio will call in setup()
* @param: mode [unint8_t] - The mode the radio shall operate in
* @param: channelNumber [int8_t] - The channelNumber the RFduinoGZLL will
*           use to communicate with the other RFduinoGZLL.
*           NOTE: Must be from 0 - 25
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::begin(uint8_t mode, uint32_t channelNumber) {
    // Save global radio mode
    radioMode = mode;
    // Restrict the channel to 0-25 inclusively
    if (channelNumber > RFDUINOGZLL_CHANNEL_LIMIT_UPPER || channelNumber < RFDUINOGZLL_CHANNEL_LIMIT_LOWER) {
        channelNumber = RFDUINOGZLL_CHANNEL_LIMIT_LOWER;
    }

    // configure radio
    configure(mode,channelNumber);
}

/**
* @description Puts into debug mode and then call other function
* @param: mode [unint8_t] - The mode the radio shall operate in
* @param: channelNumber [int8_t] - The channelNumber the RFduinoGZLL will
*           use to communicate with the other RFduinoGZLL.
*           NOTE: Must be from 0 - 25
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::beginDebug(uint8_t mode, uint32_t channelNumber) {
    debugMode = true;
    return begin(mode,channelNumber);
}



/**
* @description Private function to initialize the OpenBCI_Radios_Class object
* @param: mode [unint8_t] - The mode the radio shall operate in
* @param: channelNumber [int8_t] - The channelNumber the RFduinoGZLL will
*           use to communicate with the other RFduinoGZLL
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::configure(uint8_t mode, uint32_t channelNumber) {
    // Quickly check to see if in pass through mode, if so, call and dip out of func
    if (mode == OPENBCI_MODE_PASS_THRU) {
        configurePassThru();
    } else { // we are either dealing with a Host or a Device
        // We give the opportunity to call any 'universal' code, rather code, that
        //    gets set up the same on both Host and Device

        // Check to see if we need to set the channel number
        //  this is only the case on the first run of the program
        if (needToSetChannelNumber()) {
            setChannelNumber(channelNumber);
        }
        RFduinoGZLL.channel = getChannelNumber();
        radioChannel = getChannelNumber();

        // Check to see if we need to set the poll time
        //  this is only the case on the first run of the program
        if (needToSetPollTime()) {
            setPollTime(OPENBCI_TIMEOUT_PACKET_POLL_MS);
        }
        pollTime = getPollTime();

        // get the buffers ready
        bufferRadioReset();
        bufferRadioClean();
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
void OpenBCI_Radios_Class::configureDevice(void) {
    // Start the RFduinoGZLL in DEVICE0 mode
    RFduinoGZLL.begin(RFDUINOGZLL_ROLE_DEVICE);

    // Configure pins

    if (debugMode) { // Dongle to Dongle debug mode
        // BEGIN: To run host as device
        pinMode(OPENBCI_PIN_HOST_RESET,INPUT);
        pinMode(OPENBCI_PIN_HOST_LED,OUTPUT);
        digitalWrite(OPENBCI_PIN_HOST_LED,HIGH);
        Serial.begin(OPENBCI_BAUD_RATE_DEFAULT);
        // END: To run host as device
    } else {
        // BEGIN: To run host normally
        pinMode(OPENBCI_PIN_DEVICE_PCG, INPUT); //feel the state of the PIC with this pin
        // Start the serial connection. On the device we must specify which pins are
        //    rx and tx, where:
        //      rx = GPIO3
        //      tx = GPIO2
        Serial.begin(OPENBCI_BAUD_RATE_DEFAULT, 3, 2);
        // END: To run host normally
    }

    bufferResetStreamPacketBuffer();

    pollRefresh();

}

/**
* @description Private function to initialize the radio in Host mode
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::configureHost(void) {
    // Start the RFduinoGZLL in HOST mode
    RFduinoGZLL.begin(RFDUINOGZLL_ROLE_HOST);

    // Configure pins
    pinMode(OPENBCI_PIN_HOST_RESET,INPUT);
    pinMode(OPENBCI_PIN_HOST_LED,OUTPUT);

    // Turn LED on
    digitalWrite(OPENBCI_PIN_HOST_LED,HIGH);

    // Open the Serial connection
    Serial.begin(OPENBCI_BAUD_RATE_DEFAULT);

    packetInTXRadioBuffer = false;
    ringBufferRead = 0;
    ringBufferWrite = 0;
    ringBufferNumBytes = 0;
    sendSerialAck = false;
    processingSendToDevice = false;
    channelNumberSaveAttempted = false;
    printMessageToDriverFlag = false;

}

/**
* @description Private function to initialize the radio in Pass Through mode
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::configurePassThru(void) {
    // Configure the pins
    pinMode(0,OUTPUT_D0H1);  // output is highZ when logic 0, HIGH when logic 1
    pinMode(1,OUTPUT_D0H1);
    pinMode(OPENBCI_PIN_HOST_LED,OUTPUT);

    digitalWrite(0,LOW);
    digitalWrite(1,LOW);
}

/**
 * @description Returns the channel number from non-volitile flash memory
 */
uint32_t OpenBCI_Radios_Class::getChannelNumber(void) {
    return *ADDRESS_OF_PAGE(RFDUINOGZLL_FLASH_MEM_ADDR);
}

/**
 * @description Returns the poll time from non-volitile flash memory
 */
uint32_t OpenBCI_Radios_Class::getPollTime(void) {
    return *(ADDRESS_OF_PAGE(RFDUINOGZLL_FLASH_MEM_ADDR) + 1);
}

/**
 * @description Reads from memory to see if the channel number needs to be set
 * @return {boolean} True if the channel number needs to be set
 */
boolean OpenBCI_Radios_Class::needToSetChannelNumber(void) {
    return getChannelNumber() == 0xFFFFFFFF;
}

/**
 * @description Reads from memory to see if the poll time needs to be set
 * @return {boolean} True if the poll time needs to be set
 */
boolean OpenBCI_Radios_Class::needToSetPollTime(void) {
    return getPollTime() == 0xFFFFFFFF;
}

void OpenBCI_Radios_Class::revertToPreviousChannelNumber(void) {
    RFduinoGZLL.end();
    RFduinoGZLL.channel = previousRadioChannel;
    RFduinoGZLL.begin(RFDUINOGZLL_ROLE_HOST);
}

/**
 * @description Resets the poll time to the define OPENBCI_TIMEOUT_PACKET_POLL_MS
 * @return {boolean} - see `::setPollTime()`
 */
boolean OpenBCI_Radios_Class::revertToDefaultPollTime(void) {
    return setPollTime((uint32_t)OPENBCI_TIMEOUT_PACKET_POLL_MS);
}

/**
 * @description Store a channel number to memory. Allows for the channel to be
 *      maintained even after power cycle.
 * @param channelNumber {uint32_t} - The new channel to set to. Must be less
 *      than 25.
 * @return {boolean} - If the channel was successfully flashed to memory. False
 *      when the channel number is out of bounds.
 */
boolean OpenBCI_Radios_Class::setChannelNumber(uint32_t channelNumber) {
    if (channelNumber > RFDUINOGZLL_CHANNEL_LIMIT_UPPER) {
        return false;
    }

    uint32_t *p = ADDRESS_OF_PAGE(RFDUINOGZLL_FLASH_MEM_ADDR);
    boolean willSetPollTime = false;
    uint32_t pollTime = 0xFFFFFFFF;
    // If I don't need to set the channel number then grab that channel number
    if (!needToSetPollTime()) {
        pollTime = getPollTime();
        willSetPollTime = true;
    }

    int rc;
    if (flashNonVolatileMemory()) {
        if (willSetPollTime) {
            if (flashWrite(p + 1, pollTime) > 0) {
                return false;
            }
        }
        rc = flashWrite(p, channelNumber);
        if (rc == 0) {
            return true;
        } else if (rc == 1) {
            return false;
        } else if (rc == 2) {
            return false;
        }
    }
    return false;
}

/**
 * @description Store a poll time to memory. Allows for poll time to be retained
 *  after power down
 * @param pollTime {uint32_t} - The new poll time to store to memory
 * @return {boolean} - If the pollTime was successfully set
 */
boolean OpenBCI_Radios_Class::setPollTime(uint32_t pollTime) {

    uint32_t *p = ADDRESS_OF_PAGE(RFDUINOGZLL_FLASH_MEM_ADDR);
    boolean willSetChannel = false;
    uint32_t chan = 0xFFFFFFFF;
    // If I don't need to set the channel number then grab that channel number
    if (!needToSetChannelNumber()) {
        chan = getChannelNumber();
        willSetChannel = true;
    }

    int rc;
    if (flashNonVolatileMemory()) {
        if (willSetChannel) {
            if (flashWrite(p,chan)) {
                return false;
            }
        }
        rc = flashWrite(p + 1, pollTime); // Always stored 1 more than chan
        if (rc == 0) {
            return true;
        } else if (rc == 1) {
            return false;
        } else if (rc == 2) {
            return false;
        }
    }
    return false;
}

/**
 * @description Useful to call if the radios totally need to be reset.
 * @param pollTime {uint32_t} - The new poll time to store to memory
 * @return {boolean} - If the pollTime was successfully set
 */
boolean OpenBCI_Radios_Class::flashNonVolatileMemory(void) {

    uint32_t *p = ADDRESS_OF_PAGE(RFDUINOGZLL_FLASH_MEM_ADDR);

    int rc = flashPageErase(PAGE_FROM_ADDRESS(p)); // erases 1k of flash
    if (rc == 1) {
        return false;
    } else if (rc == 2) {
        return false;
    }
    return true;
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
boolean OpenBCI_Radios_Class::didPCSendDataToHost(void) {
    if (Serial.available() > 0) {
        return true;
    } else {
        return false;
    }
}

/**
* @description Called when readRadio returns true. We always write the contents
*                 of bufferRadio over Serial.
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::writeTheHostsRadioBufferToThePC(void) {
    if (debugMode) {
        for (int j = 0; j < bufferRadio.positionWrite; j++) {
            Serial.print(bufferRadio.data[j]);
        }
        Serial.println();
    } else {
        for (int j = 0; j < bufferRadio.positionWrite; j++) {
            Serial.write(bufferRadio.data[j]);
        }
    }
    bufferRadioReset();
}

/**
 * @description The first line of defense against a system that has lost it's device
 */
boolean OpenBCI_Radios_Class::commsFailureTimeout(void) {
    return millis() > (lastTimeHostHeardFromDevice + 500);
}

/**
 * @description If the Host received a stream packet, then this will have data in it
 *      and number of packets to send will be greater than 0
 * @return {boolean} - If there are packets to send
 */
// boolean OpenBCI_Radios_Class::hasStreamPacket(void) {
//     return bufferStreamPackets.numberOfPacketsToSend > 0;
// }

/**
 * @descirption Answers the question of if a packet is ready to be sent. need
 *  to check and there is no packet in the TX Radio Buffer, there are in fact
 *  packets to send and enough time has passed.
 */
boolean OpenBCI_Radios_Class::hostPacketToSend(void) {
    return packetToSend() && (packetInTXRadioBuffer == false);
}

void OpenBCI_Radios_Class::printChannelNumber(char c) {
    Serial.print("Channel number: "); Serial.print((int)c); Serial.write(c);
}

void OpenBCI_Radios_Class::printChannelNumberVerify(void) {
    Serial.print("Verify channel number is less than 25");
}

void OpenBCI_Radios_Class::printBaudRateChangeTo(int b) {
    Serial.print("Switch your baud rate to ");
    Serial.print(b);
};

void OpenBCI_Radios_Class::printCommsTimeout(void) {
    Serial.print("Communications timeout - Device failed to poll Host");
}

void OpenBCI_Radios_Class::printEOT(void) {
    Serial.print("$$$");
}

void OpenBCI_Radios_Class::printFailure(void) {
    Serial.print("Failure: ");
}

void OpenBCI_Radios_Class::printPollTime(char p) {
    Serial.print("Poll time: "); Serial.print((int)p); Serial.write(p);
}

void OpenBCI_Radios_Class::printSuccess(void) {
    Serial.print("Success: ");
}

void OpenBCI_Radios_Class::printValidatedCommsTimeout(void) {
    printFailure();
    printCommsTimeout();
    printEOT();
}

void OpenBCI_Radios_Class::printMessageToDriver(uint8_t code) {
    switch (code) {
        case OPENBCI_HOST_MSG_COMMS_DOWN:
            printValidatedCommsTimeout();
            break;
        case OPENBCI_HOST_MSG_SYS_UP:
            printSuccess();
            Serial.print("System is Up");
            printEOT();
            break;
        case OPENBCI_HOST_MSG_SYS_DOWN:
            printFailure();
            Serial.print("System is Down");
            printEOT();
            break;
        case OPENBCI_HOST_MSG_BAUD_FAST:
            printSuccess();
            printBaudRateChangeTo((int)OPENBCI_BAUD_RATE_FAST);
            printEOT();
            delay(2);
            // Close the current serial connection
            Serial.end();
            // Open the Serial connection
            Serial.begin(OPENBCI_BAUD_RATE_FAST);
            break;
        case OPENBCI_HOST_MSG_BAUD_DEFAULT:
            printSuccess();
            printBaudRateChangeTo((int)OPENBCI_BAUD_RATE_DEFAULT);
            printEOT();
            delay(2);
            // Close the current serial connection
            Serial.end();
            // Open the Serial connection
            Serial.begin(OPENBCI_BAUD_RATE_DEFAULT);
            break;
        case OPENBCI_HOST_MSG_CHAN:
            printValidatedCommsTimeout();
            break;
        case OPENBCI_HOST_MSG_CHAN_OVERRIDE:
            radioChannel = getChannelNumber();
            RFduinoGZLL.end();
            RFduinoGZLL.channel = getChannelNumber();
            RFduinoGZLL.begin(RFDUINOGZLL_ROLE_HOST);
            printSuccess();
            Serial.print("Host override - ");
            printChannelNumber(getChannelNumber());
            printEOT();
            break;
        case OPENBCI_HOST_MSG_CHAN_VERIFY:
            printFailure();
            printChannelNumberVerify();
            printEOT();
            break;
        case OPENBCI_HOST_MSG_CHAN_GET_FAILURE:
            printFailure();
            Serial.print("Host on ");
            printChannelNumber(getChannelNumber());
            printEOT();
            break;
        case OPENBCI_HOST_MSG_CHAN_GET_SUCCESS:
            printSuccess();
            Serial.print("Host and Device on ");
            printChannelNumber(getChannelNumber());
            printEOT();
            break;
        case OPENBCI_HOST_MSG_POLL_TIME:
            radio.printSuccess();
            radio.printPollTime(radio.getPollTime());
            radio.printEOT();
            break;
        default:
            break;
    }
}

/**
 * Used to process the the serial buffer if the device fails to poll the host
 *  more than 3 * pollTime.
 */
void OpenBCI_Radios_Class::processCommsFailure(void) {
    // Serial.println("processCommsFailure");
    if (isWaitingForNewChannelNumberConfirmation) {
        isWaitingForNewChannelNumberConfirmation = false;
        revertToPreviousChannelNumber();
        printValidatedCommsTimeout();
    } else if (isWaitingForNewPollTimeConfirmation) {
        isWaitingForNewPollTimeConfirmation = false;
        printValidatedCommsTimeout();
    } else {
        switch (bufferSerial.numberOfPacketsToSend) {
            case 0:
                break; // Do nothing
            case 1:
                processCommsFailureSinglePacket();
                break;
            default: // numberOfPacketsToSend > 1
                bufferCleanSerial(bufferSerial.numberOfPacketsToSend);
                printValidatedCommsTimeout();
                break;
        }
    }
}

/**
 * Used to process the the serial buffer if the device fails to poll the host
 *  more than 3 * pollTime. The single packet condition should be parsed because
 *  it may contain actionable queries to the OpenBCI Radio system.
 */
void OpenBCI_Radios_Class::processCommsFailureSinglePacket(void) {
    // The first byte needs to match the command key to act on it
    if (bufferSerial.packetBuffer->data[OPENBCI_HOST_PRIVATE_POS_KEY] == OPENBCI_HOST_PRIVATE_CMD_KEY) {
        // Switch on the first byte of the first packet.
        switch (bufferSerial.packetBuffer->data[OPENBCI_HOST_PRIVATE_POS_CODE]) {
            case OPENBCI_HOST_CMD_CHANNEL_SET:
                printMessageToDriver(OPENBCI_HOST_MSG_COMMS_DOWN);
                break;
            case OPENBCI_HOST_CMD_CHANNEL_SET_OVERIDE:
                    if (setChannelNumber((uint32_t)bufferSerial.packetBuffer->data[OPENBCI_HOST_PRIVATE_POS_PAYLOAD])) {
                        msgToPrint = OPENBCI_HOST_MSG_CHAN_OVERRIDE;
                        printMessageToDriverFlag = true;
                        systemUp = false;
                    } else {
                        printMessageToDriver(OPENBCI_HOST_MSG_CHAN_VERIFY);
                    }
                break;
            case OPENBCI_HOST_CMD_CHANNEL_GET:
                printMessageToDriver(OPENBCI_HOST_MSG_CHAN_GET_FAILURE);
                break;
            case OPENBCI_HOST_CMD_BAUD_DEFAULT:
                msgToPrint = OPENBCI_HOST_MSG_BAUD_DEFAULT;
                printMessageToDriverFlag = true;
                break;
            case OPENBCI_HOST_CMD_BAUD_FAST:
                msgToPrint = OPENBCI_HOST_MSG_BAUD_FAST;
                printMessageToDriverFlag = true;
                break;
            case OPENBCI_HOST_CMD_POLL_TIME_GET:
                printMessageToDriver(OPENBCI_HOST_MSG_COMMS_DOWN);
                break;
            case OPENBCI_HOST_CMD_SYS_UP:
                // We were not able to get polled by the Device
                printMessageToDriver(OPENBCI_HOST_MSG_SYS_DOWN);
                break;
            default:
                printMessageToDriver(OPENBCI_HOST_MSG_COMMS_DOWN);
                break;
        }
    } else {
        printMessageToDriver(OPENBCI_HOST_MSG_COMMS_DOWN);
    }
    // Always clear the serial buffer
    bufferCleanSerial(1);
}


/**
 * @description The host can recieve messaged from the PC/Driver that should
 *  never be sent out. So we process an outbound packet for this!
 * @return {byte} - The action to be taken after exit:
 *                      ACTION_RADIO_SEND_NORMAL - Send a packet like normal
 *                      ACTION_RADIO_SEND_NONE - Take no action
 *                      ACTION_RADIO_SEND_SINGLE_CHAR - Send a secret radio message from singleCharMsg buffer
 */
byte OpenBCI_Radios_Class::processOutboundBuffer(volatile PacketBuffer *currentPacketBuffer) {
    if (currentPacketBuffer->positionWrite == 2) {
        return processOutboundBufferCharSingle(currentPacketBuffer->data[1]);
    } else if (currentPacketBuffer->positionWrite == 3) {
        return processOutboundBufferCharDouble(currentPacketBuffer->data);
    } else if (currentPacketBuffer->positionWrite == 4) {
        return processOutboundBufferCharTriple(currentPacketBuffer->data);
    } else {
        return ACTION_RADIO_SEND_NORMAL;
    }
}

/**
 * @description Called by the Host's on_recieve function if the out bound buffer
 *      has a single char in it.
 * @param c {char} - The char in the outbound buffer.
 * @return {byte} - The action to be taken after exit:
 *                      ACTION_RADIO_SEND_NORMAL - Send a packet like normal
 *                      ACTION_RADIO_SEND_NONE - Take no action
 *                      ACTION_RADIO_SEND_SINGLE_CHAR - Send a secret radio message from singleCharMsg buffer
 */
byte OpenBCI_Radios_Class::processOutboundBufferCharSingle(char c) {
    switch (c) {
        case OPENBCI_HOST_TIME_SYNC:
            // Send a comma back to the PC/Driver
            sendSerialAck = true;
            return ACTION_RADIO_SEND_NORMAL;
        default:
            return ACTION_RADIO_SEND_NORMAL;
    }
}

/**
 * @description Called by the Host's on_recieve function if the out bound buffer
 *      has two chars in it.
 * @param buffer {char *} - The char buffer
 * @return {byte} - The action to be taken after exit:
 *                      ACTION_RADIO_SEND_NORMAL - Send a packet like normal
 *                      ACTION_RADIO_SEND_NONE - Take no action
 *                      ACTION_RADIO_SEND_SINGLE_CHAR - Send a secret radio message from singleCharMsg buffer
 */
byte OpenBCI_Radios_Class::processOutboundBufferCharDouble(volatile char *buffer) {
    // The first byte needs to match the command key to act on it
    if (buffer[1] == OPENBCI_HOST_PRIVATE_CMD_KEY) {
        // Decode the char
        switch (buffer[2]) {
            // Is the byte the command for time sync set?
            case OPENBCI_HOST_TIME_SYNC:
                // Send a comma back to the PC/Driver
                Serial.println("yo");
                sendSerialAck = true;
                return ACTION_RADIO_SEND_NORMAL;
            // Is the byte the command for a host channel number?
            case OPENBCI_HOST_CMD_CHANNEL_GET:
                if (systemUp) {
                    // Send the channel number back to the driver
                    msgToPrint = OPENBCI_HOST_MSG_CHAN_GET_SUCCESS;
                    printMessageToDriverFlag = true;
                } else {
                    // Send the channel number back to the driver
                    msgToPrint = OPENBCI_HOST_MSG_CHAN_GET_FAILURE;
                    printMessageToDriverFlag = true;
                }
                // Clear the serial buffer
                bufferCleanSerial(1);
                return ACTION_RADIO_SEND_NONE;
            case OPENBCI_HOST_CMD_BAUD_DEFAULT:
                msgToPrint = OPENBCI_HOST_MSG_BAUD_DEFAULT;
                printMessageToDriverFlag = true;
                // Clear the serial buffer
                bufferCleanSerial(1);
                return ACTION_RADIO_SEND_NONE;
            case OPENBCI_HOST_CMD_BAUD_FAST:
                msgToPrint = OPENBCI_HOST_MSG_BAUD_FAST;
                printMessageToDriverFlag = true;
                // Clear the serial buffer
                bufferCleanSerial(1);
                return ACTION_RADIO_SEND_NONE;
            case OPENBCI_HOST_CMD_SYS_UP:
                if (systemUp) {
                    msgToPrint = OPENBCI_HOST_MSG_SYS_UP;
                    printMessageToDriverFlag = true;
                } else {
                    msgToPrint = OPENBCI_HOST_MSG_SYS_DOWN;
                    printMessageToDriverFlag = true;
                }
                // Clear the serial buffer
                bufferCleanSerial(1);
                return ACTION_RADIO_SEND_NONE;
            case OPENBCI_HOST_CMD_POLL_TIME_GET:
                // Send a time change request to the device
                singleCharMsg[0] = (char)ORPM_CHANGE_POLL_TIME_GET;
                // Clean the serial buffer
                bufferCleanSerial(1);
                return ACTION_RADIO_SEND_SINGLE_CHAR;
            default:
                return ACTION_RADIO_SEND_NORMAL;
        }
    } else {
        return ACTION_RADIO_SEND_NORMAL;
    }
}

/**
 * @description Called by the Host's on_recieve function if the out bound buffer
 *      has three chars in it.
 * @param buffer {char *} - The char buffer
 * @return {byte} - The action to be taken after exit:
 *                      ACTION_RADIO_SEND_NORMAL - Send a packet like normal
 *                      ACTION_RADIO_SEND_NONE - Take no action
 *                      ACTION_RADIO_SEND_SINGLE_CHAR - Send a secret radio message from singleCharMsg buffer
 */
byte OpenBCI_Radios_Class::processOutboundBufferCharTriple(volatile char *buffer) {
    // The first byte needs to match the command key to act on it
    if (buffer[1] == OPENBCI_HOST_PRIVATE_CMD_KEY) {
        switch (buffer[2]) {
            // Is the first byte equal to the channel change request?
            case OPENBCI_HOST_CMD_CHANNEL_SET:
                // Make sure the channel is within bounds (<25)
                if (buffer[3] <= RFDUINOGZLL_CHANNEL_LIMIT_UPPER) {
                    // Save requested new channel number
                    radioChannel = (uint32_t)buffer[3];
                    // Save the previous channel number
                    previousRadioChannel = getChannelNumber();
                    // Send a channel change request to the device
                    singleCharMsg[0] = (char)ORPM_CHANGE_CHANNEL_HOST_REQUEST;
                    // Clear the serial buffer
                    bufferCleanSerial(1);
                    // Send a single char message
                    return ACTION_RADIO_SEND_SINGLE_CHAR;
                } else {
                    // Clear the serial buffer
                    bufferCleanSerial(1);
                    // Send back error message to the PC/Driver
                    msgToPrint = OPENBCI_HOST_MSG_CHAN_VERIFY;
                    printMessageToDriverFlag = true;
                    // Don't send a single char message
                    return ACTION_RADIO_SEND_NONE;
                }
            case OPENBCI_HOST_CMD_POLL_TIME_SET:
                // Save the new poll time
                pollTime = (uint32_t)buffer[OPENBCI_HOST_PRIVATE_POS_PAYLOAD];
                // Send a time change request to the device
                singleCharMsg[0] = (char)ORPM_CHANGE_POLL_TIME_HOST_REQUEST;
                // Clear the serial buffer
                bufferCleanSerial(1);
                return ACTION_RADIO_SEND_SINGLE_CHAR;
            case OPENBCI_HOST_CMD_CHANNEL_SET_OVERIDE:
                if (setChannelNumber((uint32_t)buffer[OPENBCI_HOST_PRIVATE_POS_PAYLOAD])) {
                    msgToPrint = OPENBCI_HOST_MSG_CHAN_OVERRIDE;
                    printMessageToDriverFlag = true;
                    systemUp = false;
                } else {
                    msgToPrint = OPENBCI_HOST_MSG_CHAN_VERIFY;
                    printMessageToDriverFlag = true;
                }
                bufferCleanSerial(1);
                return ACTION_RADIO_SEND_NONE;
            default:
                return ACTION_RADIO_SEND_NORMAL;
        }
    } else {
        return ACTION_RADIO_SEND_NORMAL;
    }
}

/**
 * @description Called from Host's on_recieve if a packet will be sent.
 */
void OpenBCI_Radios_Class::sendPacketToDevice(device_t device) {
    processingSendToDevice = true;
    // Build byteId
    int packetNumber = bufferSerial.numberOfPacketsToSend - bufferSerial.numberOfPacketsSent - 1;

    byte radioAction = ACTION_RADIO_SEND_NORMAL;

    // Is there only one packet to send and it's the first packet?
    if (bufferSerial.numberOfPacketsToSend == 1 && packetNumber == 0) {
        // Enter the process outbound buffer subroutine
        radioAction = processOutboundBuffer(bufferSerial.packetBuffer);
    }

    // Make the byte id
    char byteId;

    switch (radioAction) {
        case ACTION_RADIO_SEND_SINGLE_CHAR:
            RFduinoGZLL.sendToDevice(device,singleCharMsg,1);
            // Set flag
            packetInTXRadioBuffer = true;
            break;
        case ACTION_RADIO_SEND_NORMAL:
            // Save the byteId
            byteId = byteIdMake(false,packetNumber,(bufferSerial.packetBuffer + bufferSerial.numberOfPacketsSent)->data + 1, (bufferSerial.packetBuffer + bufferSerial.numberOfPacketsSent)->positionWrite - 1);
            // Add the byteId to the packet
            (bufferSerial.packetBuffer + bufferSerial.numberOfPacketsSent)->data[0] = byteId;
            // Serial.print("Sending "); Serial.print((bufferSerial.packetBuffer + bufferSerial.numberOfPacketsSent)->positionWrite); Serial.println(" bytes");
            RFduinoGZLL.sendToDevice(device,(char *)(bufferSerial.packetBuffer + bufferSerial.numberOfPacketsSent)->data, (bufferSerial.packetBuffer + bufferSerial.numberOfPacketsSent)->positionWrite);
            // Increment number of bytes sent
            bufferSerial.numberOfPacketsSent++;
            // Set flag
            packetInTXRadioBuffer = true;
            break;
        default: // do nothing
            break;
    }
    processingSendToDevice = false;
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
boolean OpenBCI_Radios_Class::didPicSendDeviceSerialData(void) {
    return Serial.available() > 0;
}

/**
 * @description If there are
 */
boolean OpenBCI_Radios_Class::thereIsDataInSerialBuffer(void) {
    return bufferSerial.numberOfPacketsSent < bufferSerial.numberOfPacketsToSend;
}

/**
 * @description Sends a null byte to the host
 */
void OpenBCI_Radios_Class::sendPollMessageToHost(void) {
    RFduinoGZLL.sendToHost(NULL,0);
}

/**
 * @description Sends a one byte long message to the host
 * @param msg {byte} - A single byte to send to the host
 */
void OpenBCI_Radios_Class::sendRadioMessageToHost(byte msg) {
    RFduinoGZLL.sendToHost((const char*)msg,1);
}

/**
* @description Private function to take data from the serial port and send it
*                 to the HOST
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::sendTheDevicesFirstPacketToTheHost(void) {
    // Is there data in bufferSerial?
    if (bufferSerial.numberOfPacketsToSend > 0 && bufferSerial.numberOfPacketsSent == 0) {
        // Get the packet number or type, based on streaming or not
        int packetNumber = bufferSerial.numberOfPacketsToSend - 1;

        char byteId = byteIdMake(false,packetNumber,bufferSerial.packetBuffer->data + 1, bufferSerial.packetBuffer->positionWrite - 1);

        // Add the byteId to the packet
        bufferSerial.packetBuffer->data[0] = byteId;

        // Send back some data
        RFduinoGZLL.sendToHost((char *)bufferSerial.packetBuffer->data, bufferSerial.packetBuffer->positionWrite);

        // We know we just sent the first packet
        bufferSerial.numberOfPacketsSent = 1;

        pollRefresh();

        bufferResetStreamPacketBuffer();
    }
}

void OpenBCI_Radios_Class::setByteIdForPacketBuffer(int packetNumber) {
    char byteId = byteIdMake(false,packetNumber,(bufferSerial.packetBuffer + bufferSerial.numberOfPacketsSent)->data + 1, (bufferSerial.packetBuffer + bufferSerial.numberOfPacketsSent)->positionWrite - 1);

    // Add the byteId to the packet
    (bufferSerial.packetBuffer + bufferSerial.numberOfPacketsSent)->data[0] = byteId;
}

void OpenBCI_Radios_Class::sendPacketToHost(void) {

    int packetNumber = bufferSerial.numberOfPacketsToSend - bufferSerial.numberOfPacketsSent - 1;

    // Make the byteId
    char byteId = byteIdMake(false,packetNumber,(bufferSerial.packetBuffer + bufferSerial.numberOfPacketsSent)->data + 1, (bufferSerial.packetBuffer + bufferSerial.numberOfPacketsSent)->positionWrite - 1);

    // Add the byteId to the packet
    (bufferSerial.packetBuffer + bufferSerial.numberOfPacketsSent)->data[0] = byteId;

    // (Host sends Payload ACK, TX Fifo: 1)
    RFduinoGZLL.sendToHost((char *)(bufferSerial.packetBuffer + bufferSerial.numberOfPacketsSent)->data, (bufferSerial.packetBuffer + bufferSerial.numberOfPacketsSent)->positionWrite);
    // onReceive called with Payload ACK
    pollRefresh();

    bufferSerial.numberOfPacketsSent++;
}

/**
 * @description Checks to see if we have a packet waiting in the streamPacketBuffer
 *  waiting to be sent out
 * @return {bool} if we have a packet waiting
 */
boolean OpenBCI_Radios_Class::isAStreamPacketWaitingForLaunch(void) {
    return curStreamState == STREAM_STATE_READY;
}

/**
 * @description Test to see if a char follows the stream tail byte format
 */
boolean OpenBCI_Radios_Class::isATailByteChar(char newChar) {
    return (newChar >> 4) == 0xC;
}

/**
 * @description Send one char from the Serial port and this function will implement
 *  the serial read subroutine
 * @param newChar {char} - A new char to process
 * @param {char} - The char that was read in
 */
char OpenBCI_Radios_Class::processChar(char newChar) {
    // Always store to serial buffer
    boolean success = storeCharToSerialBuffer(newChar);
    // Verify we have not over flowed
    if (!success) return newChar;

    // Process the new char
    switch (curStreamState) {
        case STREAM_STATE_TAIL:
            // Is the current char equal to 0xCX where X is 0-F?
            if (isATailByteChar(newChar)) {
                // Set the type byte
                streamPacketBuffer.typeByte = newChar;
                // Change the state to ready
                curStreamState = STREAM_STATE_READY;
            } else {
                // Reset the state machine
                curStreamState = STREAM_STATE_INIT;
                // Set bytes in to 0
                streamPacketBuffer.bytesIn = 0;
                // Test to see if this byte is a head byte, maybe if it's not a
                //  tail byte then that's because a byte was dropped on the way
                //  over from the Pic.
                if (newChar == OPENBCI_STREAM_PACKET_HEAD) {
                    // Move the state
                    curStreamState = STREAM_STATE_STORING;
                    // Store to the streamPacketBuffer
                    streamPacketBuffer.data[0] = newChar;
                    // Set to 1
                    streamPacketBuffer.bytesIn = 1;
                }
            }
            break;
        case STREAM_STATE_STORING:
            // Store to the stream packet buffer
            streamPacketBuffer.data[streamPacketBuffer.bytesIn] = newChar;
            // Increment the number of bytes read in
            streamPacketBuffer.bytesIn++;

            if (streamPacketBuffer.bytesIn == 32) {
                curStreamState = STREAM_STATE_TAIL;
                // Serial.println("SST");
            }

            break;
        // We have called the function before we were able to send the stream
        //  packet which means this is not a stream packet, it's part of a
        //  bigger message
        case STREAM_STATE_READY:
            // Got a 34th byte, go back to start
            curStreamState = STREAM_STATE_INIT;
            // Serial.println("SSI");
            // Set bytes in to 0
            streamPacketBuffer.bytesIn = 0;

            break;
        case STREAM_STATE_INIT:
            if (newChar == OPENBCI_STREAM_PACKET_HEAD) {
                // Move the state
                curStreamState = STREAM_STATE_STORING;
                // Store to the streamPacketBuffer
                streamPacketBuffer.data[0] = newChar;
                // Set to 1
                streamPacketBuffer.bytesIn = 1;
            }
            break;
        default:
            // Reset the state
            curStreamState = STREAM_STATE_INIT;

    }
    return newChar;
}

/**
 * @description Sends a soft reset command to the Pic 32 incase of an emergency.
 */
void OpenBCI_Radios_Class::resetPic32(void) {
    Serial.write('v');
}

/**
 * @description Sends the contents of the `streamPacketBuffer`
 *                 to the HOST, sends as stream
 * @return {boolean} true when the packet has been sent
 * @author AJ Keller (@pushtheworldllc)
 */
boolean OpenBCI_Radios_Class::sendStreamPacketToTheHost(void) {

    byte packetType = byteIdMakeStreamPacketType();
    // Serial.print("Packet type: "); Serial.println(packetType);

    char byteId = byteIdMake(true,packetType,streamPacketBuffer.data + 1, OPENBCI_MAX_DATA_BYTES_IN_PACKET); // 31 bytes

    // Add the byteId to the packet
    streamPacketBuffer.data[0] = byteId;

    // Clean the serial buffer (because these bytes got added to it too)
    bufferCleanSerial(bufferSerial.numberOfPacketsToSend);
    // Serial.println("#c4\n");

    // Clean the stream packet buffer
    bufferResetStreamPacketBuffer();

    // Refresh the poll timeout timer because we just polled the Host by sending
    //  that last packet
    pollRefresh();

    // Send the packet to the host...
    RFduinoGZLL.sendToHost((char *)streamPacketBuffer.data, OPENBCI_MAX_PACKET_SIZE_BYTES); // 32 bytes
    return true;
}

/**
 * @description Stores a char to the serial buffer
 * @param newChar {char} - The new char to store to the buffer
 * @return {boolean} - If the new char was added to the serial buffer
 */
boolean OpenBCI_Radios_Class::storeCharToSerialBuffer(char newChar) {
    // Is the serial buffer overflowed?
    if (bufferSerial.overflowed) {
        // End the subroutine
        // Serial.println("OVR");
        return false;
    } else {
        // Is the current buffer's write position less than max size of 32?
        if (currentPacketBufferSerial->positionWrite < OPENBCI_MAX_PACKET_SIZE_BYTES) {
            // Store the char into the serial buffer at the write position
            currentPacketBufferSerial->data[currentPacketBufferSerial->positionWrite] = newChar;
            // Increment the write position
            currentPacketBufferSerial->positionWrite++;
            // Set the number of packets to 1 initally, it will only grow
            if (bufferSerial.numberOfPacketsToSend == 0) {
                bufferSerial.numberOfPacketsToSend = 1;
            }
            // end successful subroutine read
            return true;

        } else {
            // Are we out of serial buffers?
            if (bufferSerial.numberOfPacketsToSend == OPENBCI_MAX_NUMBER_OF_BUFFERS) {
                // Set the overflowed flag equal to true
                bufferSerial.overflowed = true;
                // Serial.println("OVR");
                // End the subroutine with a failure
                return false;

            } else {
                // Increment the current buffer pointer to the next one
                currentPacketBufferSerial++;
                // Increment the number of packets to send
                bufferSerial.numberOfPacketsToSend++;
                // Serial.print("#p2s1: "); Serial.println(bufferSerial.numberOfPacketsToSend);

                // Store the char into the serial buffer at the write position
                currentPacketBufferSerial->data[currentPacketBufferSerial->positionWrite] = newChar;
                // Increment the write position
                currentPacketBufferSerial->positionWrite++;
                // End the subroutine with success
                return true;
            }
        }
    }
}

/********************************************/
/********************************************/
/*************    PASS THRU    **************/
/********************************************/
/********************************************/

/**
 * @description used to flash the led to indicate to the user the device is in pass through mode.
 */
void OpenBCI_Radios_Class::ledFeedBackForPassThru(void) {
    digitalWrite(OPENBCI_PIN_HOST_LED,HIGH);
    delay(600);
    digitalWrite(OPENBCI_PIN_HOST_LED,LOW);
    delay(200);
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
void OpenBCI_Radios_Class::writeBufferToSerial(char *buffer, int length) {
    // Make sure we don't seg fault
    if (buffer == NULL) return;

    // Loop through all bytes in buffer
    for (int i = 0; i < length; i++) {
        Serial.write(buffer[i]);
    }
}

/**
 * @description Moves bytes into bufferStreamPackets from on_recieve
 * @param `data` - {char *} - Normally a buffer to read into bufferStreamPackets
 * @param `length` - {int} - Normally 32, but you know, who wants to read what we shouldnt be...
 * @author AJ Keller (@pushtheworldllc)
 */
void OpenBCI_Radios_Class::bufferAddStreamPacket(volatile char *data, int length) {

    ringBuffer[ringBufferWrite] = 0xA0;
    ringBufferWrite++;
    int count = 1;
    while (count < length) {
        if (ringBufferWrite >= OPENBCI_BUFFER_LENGTH) {
            ringBufferWrite = 0;
        }
        ringBuffer[ringBufferWrite] = data[count];
        ringBufferWrite++;
        count++;
    }
    ringBuffer[ringBufferWrite] = outputGetStopByteFromByteId(data[0]);
    ringBufferWrite++;
    ringBufferNumBytes += OPENBCI_MAX_PACKET_SIZE_STREAM_BYTES;

}

/**
 * @description Moves bytes into bufferStreamPackets from on_recieve
 * @param `data` - {char *} - Normally a buffer to read into bufferStreamPackets
 * @param `length` - {int} - Normally 32, but you know, who wants to read what we shouldnt be...
 * @author AJ Keller (@pushtheworldllc)
 */
void OpenBCI_Radios_Class::bufferAddTimeSyncSentAck(void) {

    if (ringBufferWrite >= OPENBCI_BUFFER_LENGTH) {
        ringBufferWrite = 0;
    }
    ringBuffer[ringBufferWrite] = (char)OPENBCI_HOST_TIME_SYNC_ACK;
    ringBufferWrite++;
    ringBufferNumBytes++;
    sendSerialAck = false;
}

/**
* @description Private function to clear the given buffer of length
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::bufferCleanChar(volatile char *buffer, int bufferLength) {
    for (int i = 0; i < bufferLength; i++) {
        buffer[i] = 0;
    }
}

/**
* @description Private function to clean a PacketBuffer.
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::bufferCleanPacketBuffer(volatile PacketBuffer *packetBuffer, int numberOfPackets) {
    for(int i = 0; i < numberOfPackets; i++) {
        packetBuffer[i].positionRead = 0;
        packetBuffer[i].positionWrite = 1;
    }
}

/**
* @description Private function to clean a PacketBuffer.
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::bufferCleanCompletePacketBuffer(volatile PacketBuffer *packetBuffer, int numberOfPackets) {
    for(int i = 0; i < numberOfPackets; i++) {
        packetBuffer[i].positionRead = 0;
        packetBuffer[i].positionWrite = 0;
    }
}

/**
* @description Private function to clean (clear/reset) a Buffer.
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::bufferCleanBuffer(volatile Buffer *buffer, int numberOfPacketsToClean) {
    bufferCleanPacketBuffer(buffer->packetBuffer,numberOfPacketsToClean);
    buffer->numberOfPacketsToSend = 0;
    buffer->numberOfPacketsSent = 0;
    buffer->overflowed = false;
}

/**
* @description Private function to clean (clear/reset) a Buffer.
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::bufferCleanCompleteBuffer(volatile Buffer *buffer, int numberOfPacketsToClean) {
    bufferCleanCompletePacketBuffer(buffer->packetBuffer,numberOfPacketsToClean);
    buffer->numberOfPacketsToSend = 0;
    buffer->numberOfPacketsSent = 0;
    // Serial.print("#p2s5: "); Serial.println(buffer->numberOfPacketsToSend);

    buffer->overflowed = false;
}

/**
 * @description Private function to clean (clear/reset) the bufferSerial.
 * @param - `numberOfPacketsToClean` - [int] - The number of packets you want to
 *      clean, for example, on init, we would clean all packets, but on cleaning
 *      from the RFduinoGZLL_onReceive() we would only clean the number of
 *      packets acutally used.
 * @author AJ Keller (@pushtheworldllc)
 */
void OpenBCI_Radios_Class::bufferCleanSerial(int numberOfPacketsToClean) {
    bufferCleanBuffer(&bufferSerial, numberOfPacketsToClean);
    currentPacketBufferSerial = bufferSerial.packetBuffer;
    // previousPacketNumber = 0;
}

/**
 * @description Used to add a char data array to the the radio buffer. Always
 *      skips the fist
 * @param data {char *} - An array from RFduinoGZLL_onReceive
 * @param len {int} - Length of array from RFduinoGZLL_onReceive
 * @param clearBuffer {boolean} - If true then will reset the flags on the radio
 *      buffer.
 * @return {boolean} - True if the data was added to the buffer, false if the
 *      buffer was overflowed.
 */
boolean OpenBCI_Radios_Class::bufferRadioAddData(volatile char *data, int len, boolean clearBuffer) {
    if (clearBuffer) {
        bufferRadioReset();
    }

    for (int i = 0; i < len; i++) {
        if (bufferRadio.positionWrite < OPENBCI_BUFFER_LENGTH) { // Check for to prevent overflow
            bufferRadio.data[bufferRadio.positionWrite] = data[i];
            bufferRadio.positionWrite++;
        } else { // We overflowed, need to return false.
            return false;
        }
    }
    return true;
}

/**
 * @description Used to fill the buffer with all zeros. Should be used as
 *      frequently as possible. This is very useful if you need to ensure that
 *      no bad data is sent over the serial port.
 */
void OpenBCI_Radios_Class::bufferRadioClean(void) {
    bufferCleanChar(bufferRadio.data,OPENBCI_BUFFER_LENGTH);
}

/**
 * @description Called when all the packets have been recieved to flush the
 *       contents of the radio buffer to the serial port.
 * @author AJ Keller (@pushtheworldllc)
 */
void OpenBCI_Radios_Class::bufferRadioFlush(void) {
    if (debugMode) {
        for (int j = 0; j < bufferRadio.positionWrite; j++) {
            Serial.print(bufferRadio.data[j]);
        }
        Serial.println();
    } else {
        for (int j = 0; j < bufferRadio.positionWrite; j++) {
            Serial.write(bufferRadio.data[j]);
        }
    }
}

/**
 * @description Used to reset the flags and positions of the radio buffer.
 */
void OpenBCI_Radios_Class::bufferRadioReset(void) {
    bufferRadio.positionWrite = 0;
    bufferRadio.previousPacketNumber = 0;
    gotAllRadioPackets = false;
}

/**
 * @description Resets the stream packet buffer to default settings
 */
void OpenBCI_Radios_Class::bufferResetStreamPacketBuffer(void) {
    streamPacketBuffer.bytesIn = 0;
    curStreamState = STREAM_STATE_INIT;
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
char OpenBCI_Radios_Class::byteIdMake(boolean isStreamPacket, int packetNumber, volatile char *data, int length) {
    // Set output initially equal to 0
    char output = 0x00;

    // Set first bit if this is a streaming packet
    if  (isStreamPacket) output = output | 0x80;

    // Set packet count bits Bits[6:3] NOTE: 0xFF is error
    // convert int to char then shift then or
    output = output | ((packetNumber & 0x0F) << 3);

    // Set the check sum bits Bits[2:0]
    // TODO: Remove all traces of checksum
    // output = output | checkSumMake(data,length);

    return output;
}

/**
* @description Determines if this byteId is a stream byte
* @param byteId [char] a byteId (see ::byteIdMake for description of bits)
* @returns [int] the check sum
*/
boolean OpenBCI_Radios_Class::byteIdGetIsStream(char byteId) {
    return byteId > 0x7F;
}

/**
* @description Strips and gets the packet number from a byteId
* @param byteId [char] a byteId (see ::byteIdMake for description of bits)
* @returns [int] the packetNumber
*/
int OpenBCI_Radios_Class::byteIdGetPacketNumber(char byteId) {
    return (int)((byteId & 0x78) >> 3);
}

/**
* @description Strips and gets the packet number from a byteId
* @param byteId [char] a byteId (see ::byteIdMake for description of bits)
* @returns [byte] the packet type
*/
byte OpenBCI_Radios_Class::byteIdGetStreamPacketType(char byteId) {
    return (byte)((byteId & 0x78) >> 3);
}

/**
* @description Strips and gets the packet number from a byteId
* @returns [byte] the packet type
*/
byte OpenBCI_Radios_Class::byteIdMakeStreamPacketType(void) {
    return (byte)(streamPacketBuffer.typeByte & 0x0F);
}

/**
* @description Send a NULL packet to the HOST
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::pollHost(void) {
    RFduinoGZLL.sendToHost(NULL,0);
    pollRefresh();
}

/**
* @description Has enough time passed since the last poll
* @return [boolean]
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::pollNow(void) {
    return millis() - timeOfLastPoll > pollTime;
}

/**
* @description Reset the time since last packent sent to HOST
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::pollRefresh(void) {
    timeOfLastPoll = millis();
}

/**
 * @description Takes a byteId and converts to a Stop Byte for a streaming packet
 * @param `byteId` - [byte] - A byteId with packet type in bits 6-3
 * @return - [byte] - A stop byte with 1100 as the MSBs with packet type in the
 *          four LSBs
 * @example byteId == 0b10111000 returns 0b11000111
 */
byte OpenBCI_Radios_Class::outputGetStopByteFromByteId(char byteId) {
    return byteIdGetStreamPacketType(byteId) | 0xC0;
}

OpenBCI_Radios_Class radio;


/********************************************/
/********************************************/
/*******    RFDUINOGZLL DELEGATE    *********/
/********************************************/
/********************************************/

/**
 * @description Used to process a single char message recieved on the host
 *      radio.
 * @param newChar {char} - The char to be read in
 * @return {boolean} - True if a packet should be sent from the serial buffer
 */
boolean OpenBCI_Radios_Class::processRadioCharHost(device_t device, char newChar) {

    switch (newChar) {
        case ORPM_PACKET_BAD_CHECK_SUM:
            // Resend the last sent packet
            bufferSerial.numberOfPacketsSent--;

            return true;

        case ORPM_PACKET_MISSED:
            // Start the page transmission over again
            bufferSerial.numberOfPacketsSent = 0;

            return true;

        case ORPM_CHANGE_CHANNEL_DEVICE_READY:
            // We are the Host, and the device is ready to change it's channel number to what every we want
            if (setChannelNumber(radioChannel)) { // Returns true if successful
                // send back the radio channel
                singleCharMsg[0] = (char)radioChannel;
                isWaitingForNewChannelNumberConfirmation = true;
                channelNumberSaveAttempted = false;
                RFduinoGZLL.sendToDevice(device,singleCharMsg,1);
                packetInTXRadioBuffer = true;

            } else {
                // Tell device to switch to the previous channel number
                radioChannel = getChannelNumber();
                singleCharMsg[0] = (char)radioChannel;
                RFduinoGZLL.sendToDevice(device,singleCharMsg,1);
                packetInTXRadioBuffer = true;
            }
            return false;

        case ORPM_CHANGE_POLL_TIME_DEVICE_READY:
            // Get the poll time from memory... should have been stored here before
            singleCharMsg[0] = (char)pollTime;
            setPollTime(pollTime);
            RFduinoGZLL.sendToDevice(device,singleCharMsg,1);
            packetInTXRadioBuffer = true;
            isWaitingForNewPollTimeConfirmation = true;
            return false;

        case ORPM_DEVICE_SERIAL_OVERFLOW:
            Serial.print("Failure: Board RFduino buffer overflowed. Soft reset command sent to Board.$$$");
            // TODO : Decide if this is a good idea
            // singleCharMsg[0] = 'v';
            // RFduinoGZLL.sendToDevice(device,singleCharMsg,1);
            packetInTXRadioBuffer = true;
            return false;

        case ORPM_INVALID_CODE_RECEIVED:
            // Working theory
            return false;

        default:
            singleCharMsg[0] = (char)ORPM_INVALID_CODE_RECEIVED;
            RFduinoGZLL.sendToDevice(device,singleCharMsg,1);
            packetInTXRadioBuffer = true;
            return false;
    }

}

/**
 * @description Used to process a single char message recieved on the device
 *      radio.
 * @param newChar {char} - The char to be read in
 * @return {boolean} - True if a packet should be sent from the serial buffer
 */
boolean OpenBCI_Radios_Class::processRadioCharDevice(char newChar) {
    if (isWaitingForNewChannelNumber) {
        isWaitingForNewChannelNumber = false;
        // Refresh poll
        pollRefresh();
        // Set the new channel number
        boolean success = setChannelNumber((uint32_t)newChar);
        if (success) {
            // Poll the host, which will swap after this...
            pollHost();
            delay(30);
            // Change Device radio channel
            RFduinoGZLL.end();
            RFduinoGZLL.channel = (uint32_t)newChar;
            RFduinoGZLL.begin(RFDUINOGZLL_ROLE_DEVICE);

        }
        return false;

    } else if (isWaitingForNewPollTime) {
        isWaitingForNewPollTime = false;
        // Refresh poll
        pollRefresh();
        // Set the new poll time
        boolean success = setPollTime((uint32_t)newChar);
        if (success) {
            // Change Device poll time
            pollTime = getPollTime();
            // Poll the host
            pollHost();
        }
        return false;

    } else {
        switch (newChar) {
            case ORPM_PACKET_BAD_CHECK_SUM:
                // Resend the last sent packet
                bufferSerial.numberOfPacketsSent--;
                return true;

            case ORPM_PACKET_MISSED:
                // Start the page transmission over again
                bufferSerial.numberOfPacketsSent = 0;
                return true;

            case ORPM_CHANGE_CHANNEL_HOST_REQUEST:
                // The host want to change the channel!
                // We need to tell the Host we acknoledge his request and are
                // Patiently waiting for the channel he wants to change to

                // Tell the Host we are ready to change channels
                isWaitingForNewChannelNumber = true;
                singleCharMsg[0] = (char)ORPM_CHANGE_CHANNEL_DEVICE_READY;
                RFduinoGZLL.sendToHost(singleCharMsg,1);
                pollRefresh();
                return false;

            case ORPM_CHANGE_POLL_TIME_HOST_REQUEST:
                // We are the device and we just got asked if we want to change
                //  our poll time
                // Now we have to wait for the new poll time
                isWaitingForNewPollTime = true;
                singleCharMsg[0] = (char)ORPM_CHANGE_POLL_TIME_DEVICE_READY;
                RFduinoGZLL.sendToHost(singleCharMsg,1);
                pollRefresh();
                return false;

            case ORPM_CHANGE_POLL_TIME_GET:
                // If there are no packets to send
                storeCharToSerialBuffer('S');
                storeCharToSerialBuffer('u');
                storeCharToSerialBuffer('c');
                storeCharToSerialBuffer('c');
                storeCharToSerialBuffer('e');
                storeCharToSerialBuffer('s');
                storeCharToSerialBuffer('s');
                storeCharToSerialBuffer(':');
                storeCharToSerialBuffer(' ');
                storeCharToSerialBuffer('0');
                storeCharToSerialBuffer('x');
                storeCharToSerialBuffer((char)getPollTime());
                storeCharToSerialBuffer('$');
                storeCharToSerialBuffer('$');
                storeCharToSerialBuffer('$');
                pollRefresh();
                return true;

            case ORPM_INVALID_CODE_RECEIVED:
                // Working theory
                return false;

            default:
                // Send the invalid code recieved message
                singleCharMsg[0] = (char)ORPM_INVALID_CODE_RECEIVED;
                RFduinoGZLL.sendToHost(singleCharMsg,1);
                pollRefresh();
                return false; // Don't send a packet
        }
    }
}

/**
 * Used to determine if there are packets in the serial buffer to be sent.
 * @returns {boolean} - True if there are packets in the buffer and enough time
 *  has passed
 */
boolean OpenBCI_Radios_Class::packetToSend(void) {
    return packetsInSerialBuffer() && serialWriteTimeOut();
}

/**
 * Used to determine if there are packets in the serial buffer to be sent.
 * @returns {boolean} - True if there are packets in the buffer;
 */
boolean OpenBCI_Radios_Class::packetsInSerialBuffer(void) {
    return bufferSerial.numberOfPacketsSent < bufferSerial.numberOfPacketsToSend;
}

/**
 * Used to see if enough time has passed since the last serial read. Useful to
 *  if a serial transmission from the PC/Driver has concluded
 * @returns {boolean} - True if enough time has passed
 */
boolean OpenBCI_Radios_Class::serialWriteTimeOut(void) {
    return micros() > (lastTimeSerialRead + OPENBCI_TIMEOUT_PACKET_NRML_uS);
}

boolean OpenBCI_Radios_Class::processDeviceRadioCharData(volatile char *data, int len) {
    // We enter this if statement if we got a packet with length greater than
    //  1. If we recieve a packet with packetNumber equal to 0, then we can set
    //  a flag to write the radio buffer.

    boolean gotLastPacket = false;
    boolean goodToAddPacketToRadioBuffer = true;
    boolean firstPacket = false;

    // The packetNumber is embedded in the first byte, the byteId
    int packetNumber = byteIdGetPacketNumber(data[0]);

    // When in debug mode, state which packetNumber we just recieved
    if (byteIdGetIsStream(data[0])) {
        // Serial.println("Got stream packet!");
        // Check to see if there is a packet to send back
        return packetToSend();
    }

    // This first statment asks if this is a last packet and the previous
    //  packet was 0 too, this is in an effort to get to the point in the
    //  program where we ask if this packet is a stream packet
    if (packetNumber == 0 && bufferRadio.previousPacketNumber == 0) {
        // This is a one packet message
        gotLastPacket = true;
        // Mark this as the first packet
        firstPacket = true;

    } else {
        if (packetNumber > 0 && bufferRadio.previousPacketNumber == 0) {
            // This is the first of multiple packets we are recieving
            bufferRadio.previousPacketNumber = packetNumber;
            // Mark as the first packet
            // firstPacket = true;

        } else {
            // This is not the first packet we are reciving of this page
            if (bufferRadio.previousPacketNumber - packetNumber == 1) { // Normal...
                // Update the global var
                bufferRadio.previousPacketNumber = packetNumber;

                // Is this the last packet?
                if (packetNumber == 0) {
                    // Serial.println("Last packet of multiple.");
                    gotLastPacket = true;
                }

            } else {
                goodToAddPacketToRadioBuffer = false;
                // We missed a packet, send resend message
                singleCharMsg[0] = ORPM_PACKET_MISSED & 0xFF;

                // Clean the radio buffer. Resets the flags.
                bufferRadioReset();
                bufferRadioClean();

            }
        }
    }

    // goodToAddPacketToRadioBuffer is true if we have not recieved an error
    //  message and this packet should be routed to either the Pic (if we're
    //  the Device) or the Driver (if we are the Host)
    if (goodToAddPacketToRadioBuffer) {
        // This is not a stream packet and, be default, we will store it
        //  into a buffer called bufferRadio that is a 1 dimensional array
        bufferRadioAddData(data+1,len-1,false);
        // If this is the last packet then we need to set a flag to empty
        //  the buffer
        if (gotLastPacket) {
            // flag contents of radio buffer to be printed!
            gotAllRadioPackets = true;
            // Serial.print("len: "); Serial.print(len); Serial.println("|~|");
        }

        if (packetToSend()) {
            return true;
        } else if (bufferSerial.numberOfPacketsSent == bufferSerial.numberOfPacketsToSend && bufferSerial.numberOfPacketsToSend != 0) {
            // Clear buffer
            bufferCleanSerial(bufferSerial.numberOfPacketsSent);
            return false;
        } else {
            pollHost();
            return false;
        }



    } else { // We got a problem
        RFduinoGZLL.sendToHost(singleCharMsg,1);
        pollRefresh();
        return false;
    }
}

boolean OpenBCI_Radios_Class::processHostRadioCharData(device_t device, volatile char *data, int len) {
    // We enter this if statement if we got a packet with length greater than one... it's important to note this is for both the Host and for the Device.
    // A general rule of this system is that if we recieve a packet with a packetNumber of 0 that signifies an actionable end of transmission
    boolean gotLastPacket = false;
    boolean goodToAddPacketToRadioBuffer = true;
    boolean firstPacket = false;

    // The packetNumber is embedded in the first byte, the byteId
    int packetNumber = byteIdGetPacketNumber(data[0]);

    if (byteIdGetIsStream(data[0])) {
        // Serial.println("Got stream packet!");
        // We don't actually read to serial port yet, we simply move it
        //  into a buffer in an effort to not write to the Serial port
        //  from an ISR.
        bufferAddStreamPacket(data,len);
        // Check to see if there is a packet to send back
        return hostPacketToSend();
    }

    // This first statment asks if this is a last packet and the previous
    //  packet was 0 too, this is in an effort to get to the point in the
    //  program where we ask if this packet is a stream packet
    if (packetNumber == 0 && bufferRadio.previousPacketNumber == 0) {
        // This is a one packet message
        gotLastPacket = true;
        // Mark this as the first packet
        // firstPacket = true;

    } else {
        if (packetNumber > 0 && bufferRadio.previousPacketNumber == 0) {
            // This is the first of multiple packets we are recieving
            bufferRadio.previousPacketNumber = packetNumber;
            // Mark this as the first packet
            // firstPacket = true;
        } else {
            // This is not the first packet we are reciving of this page
            if (bufferRadio.previousPacketNumber - packetNumber == 1) { // Normal...
                // update
                bufferRadio.previousPacketNumber = packetNumber;

                // Is this the last packet?
                if (packetNumber == 0) {
                    gotLastPacket = true;
                }

            } else {
                goodToAddPacketToRadioBuffer = false;
                // We missed a packet, send resend message
                singleCharMsg[0] = (char)ORPM_PACKET_MISSED;

                // reset ring buffer to start
                // Reset the packet state
                bufferRadioReset();
                RFduinoGZLL.sendToDevice(device,singleCharMsg,1);
                return false;
            }
        }
    }

    // goodToAddPacketToRadioBuffer is true if we have not recieved an error
    //  message and this packet should be routed to either the Pic (if we're
    //  the Device) or the Driver (if we are the Host)
    if (goodToAddPacketToRadioBuffer) {
        // Check to see if this is a stream packet... This the case then the
        //  Device has sent a stream packet to the Host

        // This is not a stream packet and, be default, we will store it
        //  into a buffer called bufferRadio that is a 1 dimensional array
        bufferRadioAddData(data+1,len-1,false);
        // If this is the last packet then we need to set a flag to empty
        //  the buffer
        if (gotLastPacket) {
            // flag contents of radio buffer to be printed!
            gotAllRadioPackets = true;
        }
    }

    if (hostPacketToSend()) {
        return true;
    } else if (bufferSerial.numberOfPacketsSent == bufferSerial.numberOfPacketsToSend && bufferSerial.numberOfPacketsToSend != 0) {
        // Serial.println("Cleaning Hosts's bufferSerial");
        // Clear buffer
        bufferCleanSerial(bufferSerial.numberOfPacketsSent);
        return false;
    }

}
