[![Stories in Ready](https://badge.waffle.io/OpenBCI/OpenBCI_Radios.png?label=ready&title=Ready)](https://waffle.io/OpenBCI/OpenBCI_Radios)
[![Join the chat at https://gitter.im/OpenBCI/openbci-js-sdk](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/OpenBCI/openbci-js-sdk?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

# OpenBCI_Radios

Libraries and firmware for OpenBCI Radio Modules.

## General Overview

The OpenBCI board has an on-board RFDuino radio module acting as a "Device". The OpenBCI system includes a USB dongle for the PC, which acts as the RFDuino "Host". The format of the OpenBCI data as seen on the PC is defined by a combination of the Arduino code on the OpenBCI board and of the RFDuino code running on the Host.

For a general discussion of the OpenBCI Radio firmware please refer to software section on the learning pages on [openbci.com](http://www.openbci.com).

**IMPORTANT:** The `channelNumber` will be _stored_ into memory the first and only time from the `begin()` function into memory. Why? As of firmware `v2` channels can be changed over the air, that required us to retain the channel number during power cycle periods, i.e. turning the boards on and off. See our [learning docs](http://docs.openbci.com/software/01-OpenBCI_SDK#openbci-firmware-sdk-radio-configuration-commands) for the commands on how to change the channels over the air. Our main [Processing GUI](http://openbci.com/index.php/downloads) also contains tools to change channels along with a config utility just for this purpose.

## Installation

For [detailed installation and upload instructions](http://docs.openbci.com/tutorials/03-Upload_Code_to_OpenBCI_Dongle) please refer to our learning section on [openbci.com](http://www.openbci.com).

# Reference Guide

## Functions

### begin(mode)

The function that the radio will call in `setup()`

**_mode_** - `unint8_t`

The mode the radio shall operate in, can be:

* `OPENBCI_MODE_DEVICE` - Radio operates in `DEVICE` mode. Intended to be a RFduino on the OpenBCI Board when not in debug mode.
* `OPENBCI_MODE_HOST` - Radio operates in `HOST` mode. Intended to be an RFduino on an OpenBCI dongle.
* `OPENBCI_MODE_PASS_THRU` - Pass through FTDI driver from Host to Device

### begin(mode, channelNumber)

The function that the radio will call in `setup()` with radio `channelNumber`. **NOTE** `channelNumber` is stored to non-volatile memory! More about this in a couple lines.

**_mode_** - `unint8_t`

The mode the radio shall operate in, can be:

* `OPENBCI_MODE_DEVICE` - Radio operates in `DEVICE` mode. Intended to be a RFduino on the OpenBCI Board when not in debug mode.
* `OPENBCI_MODE_HOST` - Radio operates in `HOST` mode. Intended to be an RFduino on an OpenBCI dongle.
* `OPENBCI_MODE_PASS_THRU` - Pass through FTDI driver from Host to Device

**_channelNumber_** - `uint32_t`

The channelNumber the RFduinoGZLL will use to communicate with the other RFduinoGZLL. **NOTE** can only be from `0` to `25` inclusive.  Further, and this is really important `channelNumber` will only be _stored_ into memory on the first time this function is executed.

### beginDebug(mode, channelNumber)

The function that the radio will call in `setup()` with radio `channelNumber`. This puts the system in dongle to dongle _debug_ mode. During the development of this  **NOTE** `channelNumber` is stored to non-volatile memory! More about this in a couple lines.

**_mode_** - `unint8_t`

The mode the radio shall operate in, can be:

* `OPENBCI_MODE_DEVICE` - Radio operates in `DEVICE` mode. Intended to be a RFduino on the OpenBCI Board when not in debug mode.
* `OPENBCI_MODE_HOST` - Radio operates in `HOST` mode. Intended to be an RFduino on an OpenBCI dongle.
* `OPENBCI_MODE_PASS_THRU` - Pass through FTDI driver from Host to Device

**_channelNumber_** - `uint32_t`

The channelNumber the RFduinoGZLL will use to communicate with the other RFduinoGZLL. **NOTE** can only be from `0` to `25` inclusive.  Further, and this is really important `channelNumber` will only be _stored_ into memory on the first time this function is executed.                       
### bufferAddStreamPacket(buf)

Moves bytes from StreamPacketBuffers to the main radio buffer.

**_buf_** - `StreamPacketBuffer *`

A buffer to read into the ring buffer

### bufferAddTimeSyncSentAck()

Adds a `,` to the main ring buffer. Used to ack that a time sync set command was sent.

### bufferCleanSerial(numberOfPacketsToClean)

Function to clean (clear/reset) the bufferSerial.

**_numberOfPacketsToClean_** - `int`

The number of packets you want to clean, for example, on init, we would clean all packets, but on cleaning from the RFduinoGZLL_onReceive() we would only clean the number of packets actually used.

### bufferRadioClean()

Used to fill the buffer with all zeros. Should be used as frequently as possible. This is very useful if you need to ensure that no bad data is sent over the serial port.

### bufferRadioFlush()

Called when all the packets have been received to flush the contents of the radio buffer to the serial port.

### bufferRadioFlushBuffers()

Used to flush any radio buffer that is ready to be flushed to the serial port. For now flushes just bufferRadio and bufferRadioBackUp on the Host.

### bufferRadioHasData()

Used to determine if there is data in the radio buffer. Most likely this data needs to be cleared.

**_Returns_** {boolean}

`true` if the radio buffer has data, `false` if not...

### bufferRadioReset()

Used to reset the flags and positions of the radio buffer.

### bufferResetStreamPacketBuffer()

Resets the stream packet buffer to default settings

### commsFailureTimeout()

The first line of defense against a system that has lost it's device. The timeout is 15ms longer than the longest poll time (255ms) possible.

**_Returns_** {boolean}

`true` if enough time has passed since last poll, `false` if not...

### didPCSendDataToHost()

Private function to handle a request to read serial as a host

**_Returns_** {boolean}

`true` if there is data to read, `false` if not...

### flashNonVolatileMemory()

Used to reset the non-volatile memory back to it's factory state so the parameters in `begin()` will be accepted.

**_Returns_** {boolean}

`true` if the memory was successfully reset, `false` if not...

### getChannelNumber()

Gets the channel number from non-volatile flash memory

**_Returns_** {uint32_t}

The channel number from non-volatile memory

### getPollTime()

Gets the poll time from non-volatile flash memory

**_Returns_** {uint32_t}

The poll time from non-volatile memory

### hostPacketToSend()

Answers the question of if a packet is ready to be sent. need to check and there is no packet in the TX Radio Buffer, there are in fact packets to send and enough time has passed.       

**_Returns_** {boolean}

`true` if there is a packet ready to send on the Host

### isAStreamPacketWaitingForLaunch()

Checks to see if the stream packet parser is in the STREAM_STATE_READY which means that a stream packet is ready to be sent to the Host.

**_Returns_** {boolean}

`true` if there is a stream packet ready to send the Host

### ledFeedBackForPassThru()

Used to flash the led to indicate to the user the device is in pass through mode.

### packetToSend()

Used to determine if there are packets in the serial buffer to be sent.

**_Returns_** {boolean}

`true` if there are packets in the buffer and enough time has passed     

### packetsInSerialBuffer()

Used to determine if there are packets in the serial buffer to be sent.

**_Returns_** {boolean}

`true` if there are packets in the buffer

### pollRefresh()

Reset the time since the last packet was sent to HOST. Very important with polling.

### printMessageToDriver(code)

Writes to the serial port a message that matches a specific code.

**_code_**

* `_code_` {uint8_t} - The code to Serial.write().
  * `OPENBCI_HOST_MSG_COMMS_DOWN` - Print the comms down message
  * `OPENBCI_HOST_MSG_BAUD_FAST` - Baud rate swtiched to 230400
  * `OPENBCI_HOST_MSG_BAUD_DEFAULT` - Baud rate swtiched to 115200
  * `OPENBCI_HOST_MSG_SYS_UP` - Print the system up message
  * `OPENBCI_HOST_MSG_SYS_DOWN` - Print the system down message
  * `OPENBCI_HOST_MSG_CHAN` - Print the channel number message
  * `OPENBCI_HOST_MSG_CHAN_OVERRIDE` - Print the host over ride message
  * `OPENBCI_HOST_MSG_CHAN_VERIFY` - Print the need to verify the channel number you inputed message
  * `OPENBCI_HOST_MSG_CHAN_GET_FAILURE` - The message to print when there is a comms timeout and to print just the Host channel number.
  * `OPENBCI_HOST_MSG_CHAN_GET_SUCCESS` - The message to print when the Host and Device are communicating.
  * `OPENBCI_HOST_MSG_POLL_TIME` - Prints the poll time when there is no comms.

### processCommsFailure()

Used to process the the serial buffer if the device fails to poll the host more than 3 * pollTime.

### processDeviceRadioCharData(data, len)

Entered from RFduinoGZLL_onReceive if the Device receives a packet of length greater than 1.

**_data_** - {volatile char * }

The data buffer to process.

**_len_** - {int}

The length of `data`

**_Returns_** - {boolean}

`true` if there is a packet to send to the Host.

### processHostRadioCharData(device, data, len)

Entered from `RFduinoGZLL_onReceive` if the Host receives a packet of length greater than 1.

**_device_** - {device_t}

The device that sent a packet to the Host.

**_data_** - {volatile char * }

The data buffer to process.

**_len_** - {int}

The length of `data`

**_Returns_** - {boolean}

`true` if there is a packet to send to the Device.

### processRadioCharDevice(newChar)

Used to process a single char message received on the Device radio aka a private radio message. See `OpenBCI_Radios_Definitions.h` for a full list of `ORPM`s.

**_newChar_** - {char}

The char to be read in.

**_Returns_** - {boolean}

`true` if a packet should be sent from the serial buffer.

### processRadioCharHost(newChar)

Used to process a single char message received on the Host radio aka a private radio message. See `OpenBCI_Radios_Definitions.h` for a full list of `ORPM`s.

**_newChar_** - {char}

The char to be read in.

**_Returns_** - {boolean}

`true` if a packet should be sent from the serial buffer.            


### processSerialCharDevice(newChar)

Process a char from the serial port on the Device. Stores the char not only to the serial buffer but also tries enters the char into the stream state machine.

**_newChar_** - {char}

A new char to process

**_Returns_** - {char}

Passes `newChar` back out.

### resetPic32()

Sends a soft reset command to the Pic 32 incase of an emergency.

### sendPacketToDevice(device)

Called from Host's `RFduinoGZLL_onReceive` if a packet will be sent.

**_device_** - {device_t}

The device to send the packet to.

### sendPacketToHost()

Called from Devices to send a packet to Host. Uses global variables to send the correct packet.

**_Returns_** - {int}

The packet number sent.               

### sendPollMessageToHost()

Sends a null byte to the host.

### sendStreamPacketToTheHost()

Sends the contents of the `streamPacketBuffer` to the HOST, sends as stream packet with the proper byteId.     

**_Returns_** - {boolean}

`true` when the packet has been added to the TX buffer.

### serialWriteTimeOut()

Used to see if enough time has passed since the last serial read. Useful to if a serial transmission from the PC/Driver has concluded.   

**_Returns_** - {boolean}

`true` if enough time has passed.

### storeCharToSerialBuffer(newChar)

Stores a char to the serial buffer. Used by both the Device and the Host. Protects the system from buffer overflow.

**_newChar_** - {char}

The new char to store to the serial buffer.

**_Returns_** - {boolean}

`true` if the new char was added to the serial buffer,

### thereIsDataInSerialBuffer()

If there are packets to be sent in the serial buffer.

**_Returns_** - {boolean}

`true` if there are packets waiting to be sent from the serial buffer, `false` if not...       
