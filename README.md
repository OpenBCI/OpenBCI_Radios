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

### bufferRadioReset()

Used to reset the flags and positions of the radio buffer.

### bufferResetStreamPacketBuffer()

Resets the stream packet buffer to default settings

### commsFailureTimeout()

The first line of defense against a system that has lost it's device. The timeout is 15ms longer than the longest poll time (255ms) possible.

### didPCSendDataToHost()

Private function to handle a request to read serial as a host

**_Returns_** {boolean}

`true` if there is data to read, `false` if not...

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

Reset the time since the last packet was sent to HOST

### printMessageToDriver            
### processChar                     
### processCommsFailure             
### processRadioCharDevice          
### processRadioCharHost            
### processDeviceRadioCharData      
### processHostRadioCharData        
### resetPic32                      
### sendPacketToDevice              
### sendPacketToHost                
### sendPollMessageToHost           
### sendStreamPacketToTheHost       
### serialWriteTimeOut              
### storeCharToSerialBuffer         
### thereIsDataInSerialBuffer       
