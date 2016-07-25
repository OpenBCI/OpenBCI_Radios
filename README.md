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
### bufferAddStreamPacket           
### bufferAddTimeSyncSentAck        
### bufferCleanSerial               
### bufferRadioClean                
### bufferRadioFlush                
### bufferRadioReset                
### bufferResetStreamPacketBuffer   
### commsFailureTimeout             
### didPCSendDataToHost             
### getChannelNumber                
### gotAllRadioPackets              
### hostPacketToSend                
### isAStreamPacketWaitingForLaunch
### ledFeedBackForPassThru          
### packetToSend                    
### packetsInSerialBuffer           
### pollRefresh                     
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
