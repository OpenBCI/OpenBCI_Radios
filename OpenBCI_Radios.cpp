/***************************************************
This is a library for the OpenBCI 32bit RFduinoGZLL Device and host
Let us define two over arching operating modes / paradigms: Host and Device, where:
* Host is connected to PC via USB VCP (FTDI).
* Device is connectedd to uC (PIC32MX250F128B with UDB32-MX2-DIP).

OpenBCI invests time and resources providing this open source code,
please support OpenBCI and open-source hardware by purchasing
products from OpenBCI or donating on our downloads page!

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
* @param: mode {unint8_t} - The mode the radio shall operate in
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::begin(uint8_t mode) {
  // Save global radio mode
  radioMode = mode;

  // configure radio
  configure(mode,radioChannel);
}

/**
* @description The function that the radio will call in setup()
* @param: mode {unint8_t} - The mode the radio shall operate in
* @param: channelNumber {uint32_t} - The channelNumber the RFduinoGZLL will
*           use to communicate with the other RFduinoGZLL.
*           NOTE: Must be from 0 - 25
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::begin(uint8_t mode, uint32_t channelNumber) {
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
* @param: mode {unint8_t} - The mode the radio shall operate in
* @param: channelNumber {int8_t} - The channelNumber the RFduinoGZLL will
*           use to communicate with the other RFduinoGZLL.
*           NOTE: Must be from 0 - 25
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::beginDebug(uint8_t mode, uint32_t channelNumber) {
  debugMode = true;
  begin(mode,channelNumber);
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
    bufferRadioReset(bufferRadio);
    // bufferRadioReset(bufferRadio + 1);
    bufferRadioClean(bufferRadio);
    // bufferRadioClean(bufferRadio + 1);
    streamPacketBufferHead = 0;
    streamPacketBufferTail = 0;
    for (int i = 0; i < OPENBCI_NUMBER_STREAM_BUFFERS; i++) {
      bufferStreamReset(streamPacketBuffer + i);
    }
    currentRadioBuffer = bufferRadio;
    currentRadioBufferNum = 0;
    bufferSerialReset(OPENBCI_NUMBER_SERIAL_BUFFERS);

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

  timeOfLastMultipacketSendToHost = millis();
  sendingMultiPacket = false;
  streamPacketsHaveHeads = true;

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
  // pinMode(OPENBCI_PIN_HOST_TIME,OUTPUT);
  //
  // digitalWrite(OPENBCI_PIN_HOST_TIME,LOW);

  // Turn LED on
  digitalWrite(OPENBCI_PIN_HOST_LED,HIGH);

  // Open the Serial connection
  Serial.begin(OPENBCI_BAUD_RATE_DEFAULT);

  packetInTXRadioBuffer = false;
  sendSerialAck = false;
  channelNumberSaveAttempted = false;
  printMessageToDriverFlag = false;
  systemUp = false;

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
* @description Gets the channel number from non-volatile flash memory
* @returns {uint32_t} - The channel number from non-volatile memory
* @author AJ Keller (@pushtheworldllc)
*/
uint32_t OpenBCI_Radios_Class::getChannelNumber(void) {
  return *ADDRESS_OF_PAGE(RFDUINOGZLL_FLASH_MEM_ADDR);
}

/**
* @description Gets the poll time from non-volatile flash memory
* @returns {uint32_t} - The poll time from non-volatile memory
* @author AJ Keller (@pushtheworldllc)
*/
uint32_t OpenBCI_Radios_Class::getPollTime(void) {
  return *(ADDRESS_OF_PAGE(RFDUINOGZLL_FLASH_MEM_ADDR) + 1);
}

/**
* @description Reads from memory to see if the channel number needs to be set
* @return {boolean} True if the channel number needs to be set
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::needToSetChannelNumber(void) {
  return getChannelNumber() == 0xFFFFFFFF;
}

/**
* @description Reads from memory to see if the poll time needs to be set
* @return {boolean} True if the poll time needs to be set
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::needToSetPollTime(void) {
  return getPollTime() == 0xFFFFFFFF;
}

void OpenBCI_Radios_Class::revertToPreviousChannelNumber(void) {
  RFduinoGZLL.end();
  RFduinoGZLL.channel = previousRadioChannel;
  RFduinoGZLL.begin(RFDUINOGZLL_ROLE_HOST);
  radioChannel = previousRadioChannel;
}

/**
* @description Resets the poll time to the define OPENBCI_TIMEOUT_PACKET_POLL_MS
* @return {boolean} - see `::setPollTime()`
* @author AJ Keller (@pushtheworldllc)
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
* @author AJ Keller (@pushtheworldllc)
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
* @author AJ Keller (@pushtheworldllc)
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
* @description Used to reset the non-volatile memory back to it's factory state so
*  the parameters in `begin()` will be accepted.
* @return {boolean} - `true` if the memory was successfully reset, `false` if not...
* @author AJ Keller (@pushtheworldllc)
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
* @return {Boolean} - TRUE if there is data to read! FALSE if not...
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
* @description The first line of defense against a system that has lost it's
*  device. The timeout is 15ms longer than the longest polltime (255) possible.
* @returns {boolean} - `true` if enough time has passed since last poll.
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::commsFailureTimeout(void) {
  return millis() > (lastTimeHostHeardFromDevice + OPENBCI_TIMEOUT_COMMS_MS);
}

/**
* @descirption Answers the question of if a packet is ready to be sent. need
*  to check and there is no packet in the TX Radio Buffer, there are in fact
*  packets to send and enough time has passed.
* @returns {boolean} - True if there is a packet ready to send on the host
* @author AJ Keller (@pushtheworldllc)
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

/**
* @description Writes to the serial port a message that matches a specific code.
* @param {uint8_t} - The code to print Serial.write()
*  Possible options:
*  `HOST_MESSAGE_COMMS_DOWN` - Print the comms down message
*  `HOST_MESSAGE_COMMS_DOWN_CHAN` - Print the message when the comms when down trying to change channels.
*  `HOST_MESSAGE_COMMS_DOWN_POLL_TIME` - Print the messafe when the comms go down trying to change poll times.
*  `HOST_MESSAGE_BAUD_FAST` - Baud rate swtiched to 230400
*  `HOST_MESSAGE_BAUD_DEFAULT` - Baud rate swtiched to 115200
*  `HOST_MESSAGE_BAUD_HYPER` - Baud rate swtiched to 921600
*  `HOST_MESSAGE_SYS_UP` - Print the system up message
*  `HOST_MESSAGE_SYS_DOWN` - Print the system down message
*  `HOST_MESSAGE_CHAN` - Print the channel number message
*  `HOST_MESSAGE_CHAN_OVERRIDE` - Print the host over ride message
*  `HOST_MESSAGE_CHAN_VERIFY` - Print the need to verify the channel number you inputed message
*  `HOST_MESSAGE_CHAN_GET_FAILURE` - The message to print when there is a comms timeout and to print just the Host channel number.
*  `HOST_MESSAGE_CHAN_GET_SUCCESS` - The message to print when the Host and Device are communicating.
*  `HOST_MESSAGE_POLL_TIME` - Prints the poll time when there is no comms.
*  `HOST_MESSAGE_SERIAL_ACK` - Writes a serial ack (',') to the Driver/PC
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::printMessageToDriver(uint8_t code) {
  switch (code) {
    case HOST_MESSAGE_COMMS_DOWN:
    printValidatedCommsTimeout();
    break;
    case HOST_MESSAGE_COMMS_DOWN_CHAN:
    printFailure();
    Serial.print("Channel Change Request");
    printCommsTimeout();
    printEOT();
    break;
    case HOST_MESSAGE_COMMS_DOWN_POLL_TIME:
    printFailure();
    Serial.print("Poll Time Change Request");
    printCommsTimeout();
    printEOT();
    break;
    case HOST_MESSAGE_SYS_UP:
    printSuccess();
    Serial.print("System is Up");
    printEOT();
    break;
    case HOST_MESSAGE_SYS_DOWN:
    printFailure();
    Serial.print("System is Down");
    printEOT();
    break;
    case HOST_MESSAGE_BAUD_FAST:
    printSuccess();
    printBaudRateChangeTo((int)OPENBCI_BAUD_RATE_FAST);
    printEOT();
    delay(2);
    // Close the current serial connection
    Serial.end();
    // Open the Serial connection
    Serial.begin(OPENBCI_BAUD_RATE_FAST);
    break;
    case HOST_MESSAGE_BAUD_DEFAULT:
    printSuccess();
    printBaudRateChangeTo((int)OPENBCI_BAUD_RATE_DEFAULT);
    printEOT();
    delay(2);
    // Close the current serial connection
    Serial.end();
    // Open the Serial connection
    Serial.begin(OPENBCI_BAUD_RATE_DEFAULT);
    break;
    case HOST_MESSAGE_BAUD_HYPER:
    printSuccess();
    printBaudRateChangeTo((int)OPENBCI_BAUD_RATE_HYPER);
    printEOT();
    delay(2);
    // Close the current serial connection
    Serial.end();
    // Open the Serial connection
    Serial.begin(OPENBCI_BAUD_RATE_HYPER);
    break;
    case HOST_MESSAGE_CHAN:
    printValidatedCommsTimeout();
    break;
    case HOST_MESSAGE_CHAN_OVERRIDE:
    radioChannel = getChannelNumber();
    RFduinoGZLL.end();
    RFduinoGZLL.channel = getChannelNumber();
    RFduinoGZLL.begin(RFDUINOGZLL_ROLE_HOST);
    printSuccess();
    Serial.print("Host override - ");
    printChannelNumber(getChannelNumber());
    printEOT();
    systemUp = false;
    break;
    case HOST_MESSAGE_CHAN_VERIFY:
    printFailure();
    printChannelNumberVerify();
    printEOT();
    break;
    case HOST_MESSAGE_CHAN_GET_FAILURE:
    printFailure();
    Serial.print("Host on ");
    printChannelNumber(getChannelNumber());
    printEOT();
    systemUp = false;
    break;
    case HOST_MESSAGE_CHAN_GET_SUCCESS:
    printSuccess();
    Serial.print("Host and Device on ");
    printChannelNumber(getChannelNumber());
    printEOT();
    break;
    case HOST_MESSAGE_POLL_TIME:
    printSuccess();
    printPollTime(radio.getPollTime());
    printEOT();
    break;
    case HOST_MESSAGE_SERIAL_ACK:
    // Messages to print
    Serial.write(',');
    break;
    default:
    break;
  }
}

/**
* @description Used to process the the serial buffer if the device fails to poll the host
*  more than 3 * pollTime.
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::bufferSerialProcessCommsFailure(void) {
  systemUp = false;
  if (isWaitingForNewChannelNumberConfirmation) {
    isWaitingForNewChannelNumberConfirmation = false;
    revertToPreviousChannelNumber();
    msgToPrint = HOST_MESSAGE_COMMS_DOWN_CHAN;
    printMessageToDriverFlag = true;
  } else if (isWaitingForNewPollTimeConfirmation) {
    isWaitingForNewPollTimeConfirmation = false;
    msgToPrint = HOST_MESSAGE_COMMS_DOWN_POLL_TIME;
    printMessageToDriverFlag = true;
  } else {
    if (bufferSerialHasData()) {
      byte action = processOutboundBuffer(bufferSerial.packetBuffer);
      if (action == ACTION_RADIO_SEND_NORMAL) {
        msgToPrint = HOST_MESSAGE_COMMS_DOWN;
        printMessageToDriverFlag = true;
      }
      bufferSerialReset(bufferSerial.numberOfPacketsToSend);
    }
  }
}

/**
* @description Used to process the the serial buffer if the device fails to poll the host
*  more than 3 * pollTime. The single packet condition should be parsed because
*  it may contain actionable queries to the OpenBCI Radio system.
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::processCommsFailureSinglePacket(void) {
  // The first byte needs to match the command key to act on it
  if (bufferSerial.packetBuffer->data[OPENBCI_HOST_PRIVATE_POS_KEY] == OPENBCI_HOST_PRIVATE_CMD_KEY) {
    // Switch on the first byte of the first packet.
    switch (bufferSerial.packetBuffer->data[OPENBCI_HOST_PRIVATE_POS_CODE]) {
      case OPENBCI_HOST_CMD_CHANNEL_SET:
      printMessageToDriver(HOST_MESSAGE_COMMS_DOWN);
      break;
      case OPENBCI_HOST_CMD_CHANNEL_SET_OVERIDE:
      if (setChannelNumber((uint32_t)bufferSerial.packetBuffer->data[OPENBCI_HOST_PRIVATE_POS_PAYLOAD])) {
        msgToPrint = HOST_MESSAGE_CHAN_OVERRIDE;
        printMessageToDriverFlag = true;
      } else {
        printMessageToDriver(HOST_MESSAGE_CHAN_VERIFY);
      }
      break;
      case OPENBCI_HOST_CMD_CHANNEL_GET:
      printMessageToDriver(HOST_MESSAGE_CHAN_GET_FAILURE);
      break;
      case OPENBCI_HOST_CMD_BAUD_DEFAULT:
      msgToPrint = HOST_MESSAGE_BAUD_DEFAULT;
      printMessageToDriverFlag = true;
      break;
      case OPENBCI_HOST_CMD_BAUD_FAST:
      msgToPrint = HOST_MESSAGE_BAUD_FAST;
      printMessageToDriverFlag = true;
      break;
      case OPENBCI_HOST_CMD_BAUD_HYPER:
      msgToPrint = HOST_MESSAGE_BAUD_HYPER;
      printMessageToDriverFlag = true;
      break;
      case OPENBCI_HOST_CMD_POLL_TIME_GET:
      printMessageToDriver(HOST_MESSAGE_COMMS_DOWN);
      break;
      case OPENBCI_HOST_CMD_SYS_UP:
      // We were not able to get polled by the Device
      printMessageToDriver(HOST_MESSAGE_SYS_DOWN);
      break;
      default:
      printMessageToDriver(HOST_MESSAGE_COMMS_DOWN);
      break;
    }
  } else {
    printMessageToDriver(HOST_MESSAGE_COMMS_DOWN);
  }
  // Always clear the serial buffer
  bufferSerialReset(1);
}


/**
* @description The host can recieve messaged from the PC/Driver that should
*  never be sent out. So we process an outbound packet for this!
* @return {byte} - The action to be taken after exit:
*                      ACTION_RADIO_SEND_NORMAL - Send a packet like normal
*                      ACTION_RADIO_SEND_NONE - Take no action
*                      ACTION_RADIO_SEND_SINGLE_CHAR - Send a secret radio message from singleCharMsg buffer
* @author AJ Keller (@pushtheworldllc)
*/
byte OpenBCI_Radios_Class::processOutboundBuffer(PacketBuffer *buf) {
  if (buf->positionWrite == 3) {
    return processOutboundBufferCharDouble(buf->data);
  } else if (buf->positionWrite == 4) {
    return processOutboundBufferCharTriple(buf->data);
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
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::processOutboundBufferForTimeSync(void) {
  if (bufferSerial.packetBuffer->positionWrite == 2) {
    if ((char)bufferSerial.packetBuffer->data[1] == (char)OPENBCI_HOST_TIME_SYNC) {
      if (systemUp) {
        // Send a comma back to the PC/Driver
        sendSerialAck = true;
        // Add the byteId to the packet
        bufferSerial.packetBuffer->data[0] = byteIdMake(false,0,bufferSerial.packetBuffer->data + 1, bufferSerial.packetBuffer->positionWrite - 1);
        // Serial.print("Sending "); Serial.print((bufferSerial.packetBuffer + bufferSerial.numberOfPacketsSent)->positionWrite); Serial.println(" bytes");
        RFduinoGZLL.sendToDevice(DEVICE0,(char *)bufferSerial.packetBuffer->data, bufferSerial.packetBuffer->positionWrite);
        // Set flag
        packetInTXRadioBuffer = true;
        // Clear the buffer // TODO: Don't clear buffer here
        bufferSerialReset(1);
        return true;
      } else {
        msgToPrint = HOST_MESSAGE_COMMS_DOWN;
        printMessageToDriverFlag = true;
        // Clean the serial buffer
        bufferSerialReset(1);
        return false;
      }
    }
  }
  return false;
}

/**
* @description Called by the Host's on_recieve function if the out bound buffer
*      has two chars in it.
* @param buffer {char *} - The char buffer
* @return {byte} - The action to be taken after exit:
*                      ACTION_RADIO_SEND_NORMAL - Send a packet like normal
*                      ACTION_RADIO_SEND_NONE - Take no action
*                      ACTION_RADIO_SEND_SINGLE_CHAR - Send a secret radio message from singleCharMsg buffer
* @author AJ Keller (@pushtheworldllc)
*/
byte OpenBCI_Radios_Class::processOutboundBufferCharDouble(char *buffer) {
  // The first byte needs to match the command key to act on it
  if (buffer[OPENBCI_HOST_PRIVATE_POS_KEY] == OPENBCI_HOST_PRIVATE_CMD_KEY) {
    // Decode the char
    switch (buffer[OPENBCI_HOST_PRIVATE_POS_CODE]) {
      // Is the byte the command for a host channel number?
      case OPENBCI_HOST_CMD_CHANNEL_GET:
      if (systemUp) {
        // Send the channel number back to the driver
        msgToPrint = HOST_MESSAGE_CHAN_GET_SUCCESS;
        printMessageToDriverFlag = true;
      } else {
        // Send the channel number back to the driver
        msgToPrint = HOST_MESSAGE_CHAN_GET_FAILURE;
        printMessageToDriverFlag = true;
      }
      // Clear the serial buffer
      bufferSerialReset(1);
      return ACTION_RADIO_SEND_NONE;
      case OPENBCI_HOST_CMD_BAUD_DEFAULT:
      msgToPrint = HOST_MESSAGE_BAUD_DEFAULT;
      printMessageToDriverFlag = true;
      // Clear the serial buffer
      bufferSerialReset(1);
      return ACTION_RADIO_SEND_NONE;
      case OPENBCI_HOST_CMD_BAUD_FAST:
      msgToPrint = HOST_MESSAGE_BAUD_FAST;
      printMessageToDriverFlag = true;
      // Clear the serial buffer
      bufferSerialReset(1);
      return ACTION_RADIO_SEND_NONE;
      case OPENBCI_HOST_CMD_BAUD_HYPER:
      msgToPrint = HOST_MESSAGE_BAUD_HYPER;
      printMessageToDriverFlag = true;
      // Clear the serial buffer
      bufferSerialReset(1);
      return ACTION_RADIO_SEND_NONE;
      case OPENBCI_HOST_CMD_SYS_UP:
      if (systemUp) {
        msgToPrint = HOST_MESSAGE_SYS_UP;
        printMessageToDriverFlag = true;
      } else {
        msgToPrint = HOST_MESSAGE_SYS_DOWN;
        printMessageToDriverFlag = true;
      }
      // Clear the serial buffer
      bufferSerialReset(1);
      return ACTION_RADIO_SEND_NONE;
      case OPENBCI_HOST_CMD_POLL_TIME_GET:
      if (systemUp) {
        // Send a time change request to the device
        singleCharMsg[0] = (char)ORPM_GET_POLL_TIME;
        // Clean the serial buffer
        bufferSerialReset(1);
        return ACTION_RADIO_SEND_SINGLE_CHAR;
      } else {
        msgToPrint = HOST_MESSAGE_COMMS_DOWN;
        printMessageToDriverFlag = true;
        // Clean the serial buffer
        bufferSerialReset(1);
        return ACTION_RADIO_SEND_NONE;
      }
      default:
      if (systemUp) {
        return ACTION_RADIO_SEND_NORMAL;
      } else {
        msgToPrint = HOST_MESSAGE_COMMS_DOWN;
        printMessageToDriverFlag = true;
        // Clean the serial buffer
        bufferSerialReset(1);
        return ACTION_RADIO_SEND_NONE;
      }
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
* @author AJ Keller (@pushtheworldllc)
*/
byte OpenBCI_Radios_Class::processOutboundBufferCharTriple(char *buffer) {
  // The first byte needs to match the command key to act on it
  if (buffer[OPENBCI_HOST_PRIVATE_POS_KEY] == OPENBCI_HOST_PRIVATE_CMD_KEY) {
    switch (buffer[OPENBCI_HOST_PRIVATE_POS_CODE]) {
      // Is the first byte equal to the channel change request?
      case OPENBCI_HOST_CMD_CHANNEL_SET:
      if (!systemUp) {
        msgToPrint = HOST_MESSAGE_COMMS_DOWN;
        printMessageToDriverFlag = true;
        // Clean the serial buffer
        bufferSerialReset(1);
        return ACTION_RADIO_SEND_NONE;
      }
      // Make sure the channel is within bounds (<25)
      if (buffer[3] <= RFDUINOGZLL_CHANNEL_LIMIT_UPPER) {
        // Save requested new channel number
        radioChannel = (uint32_t)buffer[OPENBCI_HOST_PRIVATE_POS_PAYLOAD];
        // Save the previous channel number
        previousRadioChannel = getChannelNumber();
        // Send a channel change request to the device
        singleCharMsg[0] = (char)ORPM_CHANGE_CHANNEL_HOST_REQUEST;
        // Clear the serial buffer
        bufferSerialReset(1);
        // Send a single char message
        return ACTION_RADIO_SEND_SINGLE_CHAR;
      } else {
        // Clear the serial buffer
        bufferSerialReset(1);
        // Send back error message to the PC/Driver
        msgToPrint = HOST_MESSAGE_CHAN_VERIFY;
        printMessageToDriverFlag = true;
        // Don't send a single char message
        return ACTION_RADIO_SEND_NONE;
      }
      case OPENBCI_HOST_CMD_POLL_TIME_SET:
      if (systemUp) {
        // Save the new poll time
        pollTime = (uint32_t)buffer[OPENBCI_HOST_PRIVATE_POS_PAYLOAD];
        // Send a time change request to the device
        singleCharMsg[0] = (char)ORPM_CHANGE_POLL_TIME_HOST_REQUEST;
        // Clear the serial buffer
        bufferSerialReset(1);
        return ACTION_RADIO_SEND_SINGLE_CHAR;
      } else {
        msgToPrint = HOST_MESSAGE_COMMS_DOWN;
        printMessageToDriverFlag = true;
        // Clean the serial buffer
        bufferSerialReset(1);
        return ACTION_RADIO_SEND_NONE;
      }
      case OPENBCI_HOST_CMD_CHANNEL_SET_OVERIDE:
      if (setChannelNumber((uint32_t)buffer[OPENBCI_HOST_PRIVATE_POS_PAYLOAD])) {
        radioChannel = (uint32_t)buffer[OPENBCI_HOST_PRIVATE_POS_PAYLOAD];
        msgToPrint = HOST_MESSAGE_CHAN_OVERRIDE;
        printMessageToDriverFlag = true;
      } else {
        msgToPrint = HOST_MESSAGE_CHAN_VERIFY;
        printMessageToDriverFlag = true;
      }
      bufferSerialReset(1);
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
* @param `device` {device_t} - The device to send the packet to.
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::sendPacketToDevice(device_t device, boolean lockPacketSend) {
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
}

/********************************************/
/********************************************/
/***********    DEVICE CODE    **************/
/********************************************/
/********************************************/

/**
* @description Private function to handle a request to read serial as a device
* @return {boolean} - `true` if there is data to read, `false` if not...
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::didPicSendDeviceSerialData(void) {
  return Serial.available() > 0;
}

/**
* @description Sends a null byte to the host
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::sendPollMessageToHost(void) {
  RFduinoGZLL.sendToHost(NULL,0);
}

/**
* @description Sends a one byte long message to the host
* @param msg {byte} - A single byte to send to the host
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::sendRadioMessageToHost(byte msg) {
  RFduinoGZLL.sendToHost((const char*)msg,1);
}

void OpenBCI_Radios_Class::setByteIdForPacketBuffer(int packetNumber) {
  char byteId = byteIdMake(false,packetNumber,(bufferSerial.packetBuffer + bufferSerial.numberOfPacketsSent)->data + 1, (bufferSerial.packetBuffer + bufferSerial.numberOfPacketsSent)->positionWrite - 1);

  // Add the byteId to the packet
  (bufferSerial.packetBuffer + bufferSerial.numberOfPacketsSent)->data[0] = byteId;
}

/**
* @description Called from Devices to send a packet to Host. Uses global
*  variables to send the correct packet.
* @returns {boolean} - The packet number sent.
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::sendPacketToHost(void) {

  // Reset the stream buffers
  bufferStreamReset();

  int packetNumber = bufferSerial.numberOfPacketsToSend - bufferSerial.numberOfPacketsSent - 1;

  // Make the byteId
  char byteId = byteIdMake(false,packetNumber,(bufferSerial.packetBuffer + bufferSerial.numberOfPacketsSent)->data + 1, (bufferSerial.packetBuffer + bufferSerial.numberOfPacketsSent)->positionWrite - 1);

  // Add the byteId to the packet
  (bufferSerial.packetBuffer + bufferSerial.numberOfPacketsSent)->data[0] = byteId;

  if (RFduinoGZLL.sendToHost((char *)(bufferSerial.packetBuffer + bufferSerial.numberOfPacketsSent)->data, (bufferSerial.packetBuffer + bufferSerial.numberOfPacketsSent)->positionWrite)) {
    pollRefresh();

    bufferSerial.numberOfPacketsSent++;

    return true;
  }

  return false;
}

/**
* @description Test to see if a char follows the stream tail byte format
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::isATailByte(uint8_t newChar) {
  return (newChar >> 4) == 0xC;
}

/**
* @description Sends a soft reset command to the Pic 32 incase of an emergency.
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::resetPic32(void) {
  Serial.write('v');
}

/********************************************/
/********************************************/
/*************    PASS THRU    **************/
/********************************************/
/********************************************/

/**
* @description Used to flash the led to indicate to the user the device is in pass through mode.
* @author AJ Keller (@pushtheworldllc)
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
* @description Private function to clear the given buffer of length
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::bufferCleanChar(char *buffer, int bufferLength) {
  for (int i = 0; i < bufferLength; i++) {
    buffer[i] = 0;
  }
}

/**
* @description Private function to clean a PacketBuffer.
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::bufferCleanPacketBuffer(PacketBuffer *packetBuffer, int numberOfPackets) {
  for(int i = 0; i < numberOfPackets; i++) {
    packetBuffer[i].positionRead = 0;
    packetBuffer[i].positionWrite = 1;
  }
}

/**
* @description Private function to clean a PacketBuffer.
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::bufferCleanCompletePacketBuffer(PacketBuffer *packetBuffer, int numberOfPackets) {
  for(int i = 0; i < numberOfPackets; i++) {
    packetBuffer[i].positionRead = 0;
    packetBuffer[i].positionWrite = 0;
  }
}

/**
* @description Private function to clean (clear/reset) a Buffer.
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::bufferCleanBuffer(Buffer *buffer, int numberOfPacketsToClean) {
  bufferCleanPacketBuffer(buffer->packetBuffer,numberOfPacketsToClean);
  buffer->numberOfPacketsToSend = 0;
  buffer->numberOfPacketsSent = 0;
  buffer->overflowed = false;
}

/**
* @description Private function to clean (clear/reset) a Buffer.
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::bufferCleanCompleteBuffer(Buffer *buffer, int numberOfPacketsToClean) {
  bufferCleanCompletePacketBuffer(buffer->packetBuffer,numberOfPacketsToClean);
  buffer->numberOfPacketsToSend = 0;
  buffer->numberOfPacketsSent = 0;
  // Serial.print("#p2s5: "); Serial.println(buffer->numberOfPacketsToSend);

  buffer->overflowed = false;
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
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::bufferRadioAddData(BufferRadio *buf, char *data, int len, boolean lastPacket) {
  if (lastPacket) {
    buf->gotAllPackets = true;
  }
  // Serial.print("Pos write "); Serial.println(currentRadioBuffer->positionWrite);
  for (int i = 0; i < len; i++) {
    if (buf->positionWrite < OPENBCI_BUFFER_LENGTH_MULTI) { // Check for to prevent overflow
      buf->data[buf->positionWrite] = data[i];
      buf->positionWrite++;
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
* @param `buf` {BufferRadio *} - The buffer to clean.
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::bufferRadioClean(BufferRadio *buf) {
  bufferCleanChar(buf->data,OPENBCI_BUFFER_LENGTH_MULTI);
}

/**
* @description Called when all the packets have been recieved to flush the
*       contents of the radio buffer to the serial port.
* @param `buf` {BufferRadio *} - The buffer to flush.
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::bufferRadioFlush(BufferRadio *buf) {
  // Lock this buffer down!
  buf->flushing = true;
  if (debugMode) {
    for (int j = 0; j < buf->positionWrite; j++) {
      Serial.print(buf->data[j]);
    }
    Serial.println();
  } else {
    for (int j = 0; j < buf->positionWrite; j++) {
      Serial.write(buf->data[j]);
    }
  }
  buf->flushing = false;
}

/**
* @description Used to flush any radio buffer that is ready to be flushed to
*  the serial port.
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::bufferRadioFlushBuffers(void) {
  for (int i = 0; i < OPENBCI_NUMBER_RADIO_BUFFERS; i++) {
    bufferRadioProcessSingle(bufferRadio + i);
  }
}

/**
* @description Used to determine if there is data in the radio buffer. Most
*  likely this data needs to be cleared.
* @param `buf` {BufferRadio *} - The buffer to examine.
* @returns {boolean} - `true` if the radio buffer has data, `false` if not...
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::bufferRadioHasData(BufferRadio *buf) {
  return buf->positionWrite > 0;
}

byte OpenBCI_Radios_Class::bufferRadioProcessPacket(char *data, int len) {
  // The packetNumber is embedded in the first byte, the byteId
  int packetNumber = byteIdGetPacketNumber(data[0]);
  // Last packet
  if (packetNumber == 0) {
    // Current buffer has no data
    if (bufferRadioReadyForNewPage(currentRadioBuffer)) {
      // Take it! Mark Last
      bufferRadioAddData(currentRadioBuffer,data+1,len-1,true);
      // Return that this last packet was added
      return OPENBCI_PROCESS_RADIO_PASS_LAST_SINGLE;

      // Current buffer has data
    } else {
      // Current buffer has all packets or is flushing
      if (currentRadioBuffer->gotAllPackets || currentRadioBuffer->flushing) {
        // Can swtich to other buffer
        if (bufferRadioSwitchToOtherBuffer()) {
          // Take it! Mark Last
          bufferRadioAddData(currentRadioBuffer,data+1,len-1,true);
          // Return that this last packet was added
          return OPENBCI_PROCESS_RADIO_PASS_SWITCH_LAST;

          // Cannot switch to other buffer
        } else {
          // Serial.println("Last packet / Current buffer has data / Current buffer has all packets / Cannot switch to other buffer");
          // Reject it!
          return OPENBCI_PROCESS_RADIO_FAIL_SWITCH_LAST;
        }
        // Current buffer does not have all packets
      } else {
        // Previous packet number == packetNumber + 1
        if (currentRadioBuffer->previousPacketNumber - packetNumber == 1) {
          // Serial.println("Last packet / Current buffer has data / Current buffer does not have all packets / Previous packet number == packetNumber + 1");
          // Take it! Mark last.
          bufferRadioAddData(currentRadioBuffer,data+1,len-1,true);
          // Return that this last packet was added
          return OPENBCI_PROCESS_RADIO_PASS_LAST_MULTI;

          // Missed a packet
        } else {
          // Reject it!
          return OPENBCI_PROCESS_RADIO_FAIL_MISSED_LAST;
        }
      }
    }
    // Not last packet
  } else {
    // Current buffer has no data
    if (bufferRadioReadyForNewPage(currentRadioBuffer)) {
      // Serial.println("Not last packet / Current buffer has no data");
      // Take it, not last
      bufferRadioAddData(currentRadioBuffer,data+1,len-1,false);

      // Update the previous packet number
      currentRadioBuffer->previousPacketNumber = packetNumber;

      // Return that a packet that was not last was added
      return OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_FIRST;

      // Current buffer has data
    } else {
      // Current buffer has all packets
      if (currentRadioBuffer->gotAllPackets) {
        // Can switch to other buffer
        if (bufferRadioSwitchToOtherBuffer()) {
          // Take it! Not last
          bufferRadioAddData(currentRadioBuffer,data+1,len-1,false);

          // Update the previous packet number
          currentRadioBuffer->previousPacketNumber = packetNumber;

          // Return that a packet that was not last was added
          return OPENBCI_PROCESS_RADIO_PASS_SWITCH_NOT_LAST;

          // Cannot switch to other buffer
        } else {
          // Reject it!
          return OPENBCI_PROCESS_RADIO_FAIL_SWITCH_NOT_LAST;
        }
        // Current buffer does not have all packets
      } else {
        // Previous packet number == packetNumber + 1
        if (currentRadioBuffer->previousPacketNumber - packetNumber == 1) {
          // Take it! Not last.
          bufferRadioAddData(currentRadioBuffer,data+1,len-1,false);

          // Update the previous packet number
          currentRadioBuffer->previousPacketNumber = packetNumber;

          // Return that a packet that was not last was added
          return OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_MIDDLE;

          // Missed a packet
        } else {
          // Reject it! Reset current buffer
          return OPENBCI_PROCESS_RADIO_FAIL_MISSED_NOT_LAST;
        }
      }
    }
  }
}

/**
* @description Should only flush a buffer if it has data in it and has gotten all
*  of it's packets. This function will be called every loop so it's important to
*  make sure we don't flush a buffer unless it's really ready!
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::bufferRadioProcessSingle(BufferRadio *buf) {
  if (bufferRadioHasData(buf) && buf->gotAllPackets) {
    // Flush radio buffer to the driver
    bufferRadioFlush(buf);
    // Reset the radio buffer flags
    bufferRadioReset(buf);
  }
}

/**
* @description Used to determing if the buffer radio `buf` is in a locked state.
* @param `buf` {BufferRadio *} - The buffer to examine.
* @returns {boolen} - `true` if there is no lock on `buf`
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::bufferRadioReadyForNewPage(BufferRadio *buf) {
  return !buf->flushing && !bufferRadioHasData(buf);
}

/**
* @description Used to reset the flags and positions of the radio buffer.
* @param `buf` {BufferRadio *} - The buffer to reset.
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::bufferRadioReset(BufferRadio *buf) {
  buf->flushing = false;
  buf->gotAllPackets = false;
  buf->positionWrite = 0;
  buf->previousPacketNumber = 0;
}

/**
* @description Used to safely swap the global buffers!
* @returns {boolean} - `true` if the current radio buffer has been swapped,
*  `false` if the swap was not able to occur.
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::bufferRadioSwitchToOtherBuffer(void) {
  if (OPENBCI_NUMBER_RADIO_BUFFERS == 2) {
    // current radio buffer is set to the first one
    if (currentRadioBuffer == bufferRadio) {
      if (bufferRadioReadyForNewPage(bufferRadio + 1)) {
        currentRadioBuffer++;
        return true;
      }
      // current radio buffer is set to the second one
    } else {
      if (bufferRadioReadyForNewPage(bufferRadio)) {
        currentRadioBuffer--;
        return true;
      }
    }
  }
  return false;
}

/**
* @description Stores a char to the serial buffer. Used by both the Device and
*  the Host.
* @param newChar {char} - The new char to store to the serial buffer.
* @return {boolean} - `true` if the new char was added to the serial buffer,
*  `false` on serial buffer overflow.
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::bufferSerialAddChar(char newChar) {
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
      if (bufferSerial.numberOfPacketsToSend == OPENBCI_NUMBER_SERIAL_BUFFERS) {
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

/**
* @description If there are packets to be sent in the serial buffer.
* @return {boolean} - `true` if there are packets waiting to be sent from the
*  serial buffer, `false` if not...
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::bufferSerialHasData(void) {
  return bufferSerial.numberOfPacketsSent < bufferSerial.numberOfPacketsToSend;
}

/**
* @description Function to clean (clear/reset) the bufferSerial.
* @param - `n` - {uint8_t} - The number of packets you want to
*      clean, for example, on init, we would clean all packets, but on cleaning
*      from the RFduinoGZLL_onReceive() we would only clean the number of
*      packets actually used.
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::bufferSerialReset(uint8_t n) {
  bufferCleanBuffer(&bufferSerial, n);
  currentPacketBufferSerial = bufferSerial.packetBuffer;
  // previousPacketNumber = 0;
}

/**
* @description Based off the last time the serial port was read from, Determines
*  if enough time has passed to qualify this data as a full serial page.
* @returns {boolean} - `true` if enough time has passed, `false` if not.
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::bufferSerialTimeout(void) {
  return micros() > (lastTimeSerialRead + OPENBCI_TIMEOUT_PACKET_NRML_uS);
}

/**
* @description Process a char from the serial port on the Device. Enters the char
*  into the stream state machine.
* @param `buf` {StreamPacketBuffer *} - The stream packet buffer to add the char to.
* @param `newChar` {char} - A new char to process
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::bufferStreamAddChar(StreamPacketBuffer *buf, char newChar) {
  // Process the new char
  switch (buf->state) {
    case STREAM_STATE_TAIL:
    // Is the current char equal to 0xCX where X is 0-F?
    if (isATailByte(newChar)) {
      // Set the type byte
      buf->typeByte = newChar;
      // Change the state to ready
      buf->state = STREAM_STATE_READY;
      // Serial.print(33); Serial.print(" state: "); Serial.print("READY-");
      // Serial.println((streamPacketBuffer + streamPacketBufferHead)->state);
    } else {
      // Reset the state machine
      buf->state = STREAM_STATE_INIT;
      // Set bytes in to 0
      buf->bytesIn = 0;
      // Test to see if this byte is a head byte, maybe if it's not a
      //  tail byte then that's because a byte was dropped on the way
      //  over from the Pic.
      if (newChar == OPENBCI_STREAM_PACKET_HEAD) {
        // Move the state
        buf->state = STREAM_STATE_STORING;
        // Store to the streamPacketBuffer
        buf->data[0] = newChar;
        // Set to 1
        buf->bytesIn = 1;
      }
    }
    break;
    case STREAM_STATE_STORING:
    // Store to the stream packet buffer
    buf->data[buf->bytesIn] = newChar;
    // Increment the number of bytes read in
    buf->bytesIn++;

    if (buf->bytesIn == OPENBCI_MAX_PACKET_SIZE_BYTES) {
      buf->state = STREAM_STATE_TAIL;
    }

    break;
    // We have called the function before we were able to send the stream
    //  packet which means this is not a stream packet, it's part of a
    //  bigger message
    case STREAM_STATE_READY:
    // Got a 34th byte, go back to start
    buf->state = STREAM_STATE_INIT;
    // Set bytes in to 0
    buf->bytesIn = 0;

    break;
    case STREAM_STATE_INIT:
    if (newChar == OPENBCI_STREAM_PACKET_HEAD) {
      // Move the state
      buf->state = STREAM_STATE_STORING;
      // Store to the streamPacketBuffer
      buf->data[0] = newChar;
      // Set to 1
      buf->bytesIn = 1;
    }
    break;
    default:
    // // Reset the state
    buf->state = STREAM_STATE_INIT;
    break;

  }
}

/**
* @description Used to add a packet to the of steaming data to the current
*  `streamPacketBufferHead` and then increment the head. Will wrap around if
*  need be to avoid moving the head past `OPENBCI_NUMBER_STREAM_BUFFERS`.
* @param `data` {char *} - The data packet you want to add of length
*  `OPENBCI_MAX_PACKET_SIZE_BYTES` (32)
* @returns {boolean} - `true` if able to add it. Currently this func will always
*  return `true`, however this allows for greater flexiblity in the future.
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::bufferStreamAddData(char *data) {

  bufferStreamStoreData(streamPacketBuffer + streamPacketBufferHead, data);

  streamPacketBufferHead++;
  if (streamPacketBufferHead > (OPENBCI_NUMBER_STREAM_BUFFERS - 1)) {
    streamPacketBufferHead = 0;
  }

  return true;
}

/**
* @description Used to flush a StreamPacketBuffer to the serial port with a
*  head byte and a formated tail byte based off the `typeByte`.
* @param `buf` {StreamPacketBuffer *} - The stream packet buffer to add the char to.
* @author AJ Keller (@pushtheworldllc)
**/
void OpenBCI_Radios_Class::bufferStreamFlush(StreamPacketBuffer *buf) {
  buf->flushing = true;
  Serial.write(0xA0);
  for (int i = 0; i < OPENBCI_MAX_DATA_BYTES_IN_PACKET; i++) {
    Serial.write(buf->data[i]);
  }
  Serial.write(buf->typeByte);
  buf->flushing = false;
}

/**
* @description Used to flush a StreamPacketBuffer if the `streamPacketBufferTail`
*  is not equal to the `streamPacketBufferHead`. This function will also reset
*  the buffer after the buffer is flushed. Further it will increment `streamPacketBufferTail`
*  and wrap that around if necessary.
* @author AJ Keller (@pushtheworldllc)
**/
void OpenBCI_Radios_Class::bufferStreamFlushBuffers(void) {
  if (streamPacketBufferTail != streamPacketBufferHead) {
    bufferStreamFlush(streamPacketBuffer + streamPacketBufferTail);
    bufferStreamReset(streamPacketBuffer + streamPacketBufferTail);
    streamPacketBufferTail++;
    if (streamPacketBufferTail > (OPENBCI_NUMBER_STREAM_BUFFERS - 1)) {
      streamPacketBufferTail = 0;
    }
  }
}

/**
* @description Used to determine if a stream packet buffer is ready for a new packet
*  this function is no longer being used with the head/tail system. Will look to
*  deprecate it soon.
* @param `buf` {StreamPacketBuffer *} - The stream packet buffer to add the char to.
* @author AJ Keller (@pushtheworldllc)
**/
boolean OpenBCI_Radios_Class::bufferStreamReadyForNewPacket(StreamPacketBuffer *buf) {
  return buf->bytesIn == 0 && !buf->flushing;
}

/**
* @description Utility function to return `true` if the the streamPacketBuffer
*   is in the STREAM_STATE_READY. Normally used for determining if a stream
*   packet is ready to be sent.
* @param `buf` {StreamPacketBuffer *} - The stream packet buffer to send to the Host.
* @returns {boolean} - `true` is the `buf` is in the ready state, `false` otherwise.
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::bufferStreamReadyToSendToHost(StreamPacketBuffer *buf) {
  return radio.streamPacketBuffer->state == radio.STREAM_STATE_READY;
}

/**
* @description Resets the stream packet buffer to default settings
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::bufferStreamReset(void) {
  for (int i = 0; i < OPENBCI_NUMBER_STREAM_BUFFERS; i++) {
    bufferStreamReset(streamPacketBuffer + i);
  }
  streamPacketBufferHead = 0;
  streamPacketBufferTail = 0;
}

/**
* @description Resets the stream packet buffer to default settings
* @param `buf` {StreamPacketBuffer *} - Pointer to a stream packet buffer to reset
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Radios_Class::bufferStreamReset(StreamPacketBuffer *buf) {
  buf->flushing = false;
  buf->bytesIn = 0;
  buf->typeByte = 0;
  buf->state = STREAM_STATE_INIT;
}

/**
* @description Sends the contents of the `streamPacketBuffer` to the HOST,
*  sends as stream packet with the proper byteId.
* @returns {boolean} - `true` when the packet has been added to the TX buffer
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::bufferStreamSendToHost(StreamPacketBuffer *buf) {

  byte packetType = byteIdMakeStreamPacketType(buf->typeByte);

  char byteId = byteIdMake(true,packetType,buf->data + 1, OPENBCI_MAX_DATA_BYTES_IN_PACKET); // 31 bytes

  // Add the byteId to the packet
  buf->data[0] = byteId;

  // Clean the serial buffer (because these bytes got added to it too)
  bufferSerialReset(bufferSerial.numberOfPacketsToSend);

  if (RFduinoGZLL.sendToHost((char *)buf->data, OPENBCI_MAX_PACKET_SIZE_BYTES)) {
    // Refresh the poll timeout timer because we just polled the Host by sending
    //  that last packet
    pollRefresh();

    // Clean the stream packet buffer
    bufferStreamReset(buf);

    return true;
  }

  return false;
}

/**
* @description Used to flush a StreamPacketBuffer to the serial port with a
*  head byte and a formated tail byte based off the `typeByte`.
* @param `buf` {StreamPacketBuffer *} - The stream packet buffer to add the char to.
* @param `data` {char *} - A stream packet buffer in raw char buffer form fresh
*   from the radio.
* @author AJ Keller (@pushtheworldllc)
**/
void OpenBCI_Radios_Class::bufferStreamStoreData(StreamPacketBuffer *buf, char *data) {
  buf->bytesIn = OPENBCI_MAX_DATA_BYTES_IN_PACKET;
  buf->typeByte = outputGetStopByteFromByteId(data[0]);
  for (int i = 0; i < OPENBCI_MAX_DATA_BYTES_IN_PACKET; i++) {
    buf->data[i] = data[i+1];
  }
}

/**
* @description Based off the last time the serial port was read from, determines
*  if enough time has passed to qualify this data as a stream packet.
* @returns {boolean} - `true` if enough time has passed, `false` if not.
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::bufferStreamTimeout(void) {
  return micros() > (lastTimeSerialRead + OPENBCI_TIMEOUT_PACKET_STREAM_uS);
}

/**
* @description Creates a byteId for sending data over the RFduinoGZLL
* @param isStreamPacket [boolean] Set true if this is a streaming packet
* @param packetNumber {uint8_t} What number packet are you trying to send?
* @param data [char *] The data you want to send. NOTE: Do not send the address
*           of the entire buffer, send this method address of buffer + 1
* @param length [int] The length of the data buffer
* @returns [char] The newly formed byteId where a byteId is defined as
*           Bit 7 - Streaming byte packet
*           Bits[6:3] - Packet count
*           Bits[2:0] - The check sum
* @author AJ Keller (@pushtheworldllc)
*/
char OpenBCI_Radios_Class::byteIdMake(boolean isStreamPacket, uint8_t packetNumber, char *data, uint8_t length) {
  // Set output initially equal to 0
  char output = 0x00;

  // Set first bit if this is a streaming packet
  if  (isStreamPacket) output = output | 0x80;

  // Set packet count bits Bits[6:3] NOTE: 0xFF is error
  // convert int to char then shift then or
  output = output | ((packetNumber & 0x0F) << 3);

  return output;
}

/**
* @description Determines if this byteId is a stream byte
* @param byteId [char] a byteId (see ::byteIdMake for description of bits)
* @returns [int] the check sum
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::byteIdGetIsStream(uint8_t byteId) {
  return byteId > 0x7F;
}

/**
* @description Strips and gets the packet number from a byteId
* @param byteId [char] a byteId (see ::byteIdMake for description of bits)
* @returns [int] the packetNumber
* @author AJ Keller (@pushtheworldllc)
*/
int OpenBCI_Radios_Class::byteIdGetPacketNumber(uint8_t byteId) {
  return (int)((byteId & 0x78) >> 3);
}

/**
* @description Strips and gets the packet number from a byteId
* @param byteId [char] a byteId (see ::byteIdMake for description of bits)
* @returns [byte] the packet type
* @author AJ Keller (@pushtheworldllc)
*/
byte OpenBCI_Radios_Class::byteIdGetStreamPacketType(uint8_t byteId) {
  return (byte)((byteId & 0x78) >> 3);
}

/**
* @description Strips and gets the packet number from a byteId
* @returns [byte] the packet type
* @author AJ Keller (@pushtheworldllc)
*/
byte OpenBCI_Radios_Class::byteIdMakeStreamPacketType(uint8_t typeByte) {
  return typeByte & 0x0F;
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
* @description Reset the time since last packent sent to HOST. Very important with polling.
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
* @author AJ Keller (@pushtheworldllc)
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
* @description Used to process a single char message recieved on the Host
*      radio aka a private radio message.
* @param newChar {char} - The char to be read in
* @return {boolean} - True if a packet should be sent from the serial buffer
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::processRadioCharHost(device_t device, char newChar) {

  switch (newChar) {
    case ORPM_PACKET_PAGE_REJECT:
    // Start the page transmission over again
    bufferSerial.numberOfPacketsSent = 0;
    // Wait a little bit to let the Device finish
    delay(10);

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
* @description Used to process a single char message received on the Device
*      radio aka a private radio message.
* @param newChar {char} - The char to be read in
* @return {boolean} - `true` if a packet should be sent from the serial buffer
* @author AJ Keller (@pushtheworldllc)
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
      case ORPM_PACKET_PAGE_REJECT:
      // Start the page transmission over again
      bufferSerial.numberOfPacketsSent = 0;
      // Wait a little bit to let the Host finish
      delay(10);

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

      case ORPM_GET_POLL_TIME:
      // If there are no packets to send
      bufferSerialAddChar('S');
      bufferSerialAddChar('u');
      bufferSerialAddChar('c');
      bufferSerialAddChar('c');
      bufferSerialAddChar('e');
      bufferSerialAddChar('s');
      bufferSerialAddChar('s');
      bufferSerialAddChar(':');
      bufferSerialAddChar(' ');
      bufferSerialAddChar('0');
      bufferSerialAddChar('x');
      bufferSerialAddChar((char)getPollTime());
      bufferSerialAddChar('$');
      bufferSerialAddChar('$');
      bufferSerialAddChar('$');
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
* @description Used to determine if there are packets in the serial buffer to be sent.
* @returns {boolean} - True if there are packets in the buffer and enough time
*  has passed
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::packetToSend(void) {
  return packetsInSerialBuffer() && serialWriteTimeOut();
}

/**
* @description Used to determine if there are packets in the serial buffer to be sent.
* @returns {boolean} - True if there are packets in the buffer
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::packetsInSerialBuffer(void) {
  return bufferSerial.numberOfPacketsSent < bufferSerial.numberOfPacketsToSend;
}

/**
* @description Used to see if enough time has passed since the last serial read. Useful to
*  if a serial transmission from the PC/Driver has concluded
* @returns {boolean} - `true` if enough time has passed
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::serialWriteTimeOut(void) {
  return micros() > (lastTimeSerialRead + OPENBCI_TIMEOUT_PACKET_NRML_uS);
}

/**
* @description Entered from RFduinoGZLL_onReceive if the Device receives a
*  packet of length greater than 1.
* @param `data` {volatile char *} - The data buffer to process.
* @param `len` {int} - The length of `data`
* @returns {boolean} - `true` if there is a packet to send to the Host.
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::processDeviceRadioCharData(char *data, int len) {
  // We enter this if statement if we got a packet with length greater than
  //  1. If we recieve a packet with packetNumber equal to 0, then we can set
  //  a flag to write the radio buffer.

  // The packetNumber is embedded in the first byte, the byteId
  int packetNumber = byteIdGetPacketNumber(data[0]);

  if (byteIdGetIsStream(data[0])) {
    // Send any stream packet that comes back, back!
    // RFduinoGZLL.sendToHost((const char*)data,len);
    // Check to see if there is a packet to send back
    return packetToSend();
  }

  switch (bufferRadioProcessPacket(data,len)) {
    case OPENBCI_PROCESS_RADIO_FAIL_SWITCH_LAST:
    case OPENBCI_PROCESS_RADIO_FAIL_SWITCH_NOT_LAST:
    singleCharMsg[0] = (char)ORPM_PACKET_PAGE_REJECT;
    RFduinoGZLL.sendToHost(singleCharMsg,1);
    return false;

    case OPENBCI_PROCESS_RADIO_FAIL_MISSED_LAST:
    case OPENBCI_PROCESS_RADIO_FAIL_MISSED_NOT_LAST:
    // Not able to process the packet
    singleCharMsg[0] = (char)ORPM_PACKET_MISSED;
    RFduinoGZLL.sendToHost(singleCharMsg,1);
    bufferRadioReset(currentRadioBuffer);
    return false;

    default:
    if (packetToSend()) {
      return true;
    } else if (bufferSerial.numberOfPacketsSent == bufferSerial.numberOfPacketsToSend && bufferSerial.numberOfPacketsToSend != 0) {
      // Clear buffer
      bufferSerialReset(bufferSerial.numberOfPacketsSent);
      return false;
    } else {
      pollHost();
      return false;
    }
    break;
  }
}

/**
* @description Entered from RFduinoGZLL_onReceive if the Host receives a
*  packet of length greater than 1.
* @param `device` {device_t} - The device that sent a packet to the Host.
* @param `data` {volatile char *} - The data buffer to process.
* @param `len` {int} - The length of `data`
* @returns {boolean} - `true` if there is a packet to send to the Device.
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Radios_Class::processHostRadioCharData(device_t device, char *data, int len) {

  if (byteIdGetIsStream(data[0])) {
    // We don't actually read to serial port yet, we simply move it
    //  into a buffer in an effort to not write to the Serial port
    //  from an ISR.
    bufferStreamAddData(data);
    // Check to see if there is a packet to send back
    return hostPacketToSend();
  }

  switch (bufferRadioProcessPacket(data,len)) {
    case OPENBCI_PROCESS_RADIO_FAIL_SWITCH_LAST:
    case OPENBCI_PROCESS_RADIO_FAIL_SWITCH_NOT_LAST:
    singleCharMsg[0] = (char)ORPM_PACKET_PAGE_REJECT;
    RFduinoGZLL.sendToDevice(device,singleCharMsg,1);
    return false;

    case OPENBCI_PROCESS_RADIO_FAIL_MISSED_LAST:
    case OPENBCI_PROCESS_RADIO_FAIL_MISSED_NOT_LAST:
    // Not able to process the packet
    singleCharMsg[0] = (char)ORPM_PACKET_MISSED;
    RFduinoGZLL.sendToDevice(device,singleCharMsg,1);
    bufferRadioReset(currentRadioBuffer);
    return false;

    default:
    break;
  }

  if (hostPacketToSend()) {
    return true;
  } else if (bufferSerial.numberOfPacketsSent == bufferSerial.numberOfPacketsToSend && bufferSerial.numberOfPacketsToSend != 0) {
    // Serial.println("Cleaning Hosts's bufferSerial");
    // Clear buffer
    bufferSerialReset(bufferSerial.numberOfPacketsSent);
    return false;
  }

}
