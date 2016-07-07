/**
* Name: OpenBCI_Radio_Definitions.h
* Date: 3/15/2016
* Purpose: This is the header file for the OpenBCI radios definitions.
*
* Author: Push The World LLC (AJ Keller)
*   Much credit must also go to Joel Murphy who with Conor Russomanno and Leif
*     Percifield created the original OpenBCI_32bit_Device.ino and
*     OpenBCI_32bit_Host.ino files in the Summer of 2014. Much of this code base
*     is inspired directly from their work.
*/

#ifndef __OpenBCI_Radio_Definitions__
#define __OpenBCI_Radio_Definitions__

// These are helpful maximums to reference nad use in the code
#define OPENBCI_MAX_DATA_BYTES_IN_PACKET 31
#define OPENBCI_MAX_PACKET_SIZE_BYTES 32
#define OPENBCI_MAX_PACKET_SIZE_STREAM_BYTES 33
#define OPENBCI_MAX_NUMBER_OF_BUFFERS 16


#define OPENBCI_TIMEOUT_PACKET_NRML_uS 3000 // The time to wait before determining a multipart packet is ready to be send
#define OPENBCI_TIMEOUT_PACKET_STREAM_uS 88 // Slightly longer than it takes to send a serial byte at 115200
#define OPENBCI_TIMEOUT_PACKET_POLL_MS 80 // Poll time out length for sending null packet from device to host

// Stream byte stuff
#define OPENBCI_STREAM_BYTE_START 0xA0
#define OPENBCI_STREAM_BYTE_STOP 0xC0

// Max buffer lengths
#define OPENBCI_BUFFER_LENGTH 528

// These are the three different possible configuration modes for this library
#define OPENBCI_MODE_DEVICE 0
#define OPENBCI_MODE_HOST 1
#define OPENBCI_MODE_PASS_THRU 2

// Pins used by the Device
#define OPENBCI_PIN_DEVICE_PCG 5
// Pins used by the Host
#define OPENBCI_PIN_HOST_LED 2
#define OPENBCI_PIN_HOST_RESET 6

// roles for the RFduinoGZLL
#define RFDUINOGZLL_ROLE_HOST HOST
#define RFDUINOGZLL_ROLE_DEVICE DEVICE0

// Channel limits
#define RFDUINOGZLL_CHANNEL_LIMIT_LOWER 0
#define RFDUINOGZLL_CHANNEL_LIMIT_UPPER 25

// flash memory address for RFdunioGZLL
#define RFDUINOGZLL_FLASH_MEM_ADDR 251

// Max number of packets on the TX buffer
#define RFDUINOGZLL_MAX_PACKETS_ON_TX_BUFFER 3

// radio errors
#define ERROR_RADIO_NONE 0x00
#define ERROR_RADIO_RESEND_LAST_PACKET 0x01
#define ERROR_RADIO_RESEND_LAST_PAGE 0x02

// Private Radio communications
//  ORPM --> "OpenBCI Radio Private Message"
#define ORPM_INVALID_CODE_RECEIVED 0x00 // The other radio sent a 1 byte message that does not match any
#define ORPM_PACKET_BAD_CHECK_SUM 0x01 // Bad check sum
#define ORPM_PACKET_MISSED 0x02 // Missed a packet
#define ORPM_PACKET_INIT 0x03 // Init packet
#define ORPM_DEVICE_SERIAL_OVERFLOW 0x04 // The Device is being overflowed by Pic
#define ORPM_CHANGE_CHANNEL_HOST_REQUEST 0x05 // CCHR
#define ORPM_CHANGE_CHANNEL_DEVICE_READY 0x06 //
#define ORPM_CHANGE_POLL_TIME_HOST_REQUEST 0x07 //
#define ORPM_CHANGE_POLL_TIME_DEVICE_READY 0x08 //
#define ORPM_CHANGE_POLL_TIME_GET 0x09 //

// Used to determine what to send after a proccess out bound buffer
#define ACTION_RADIO_SEND_NONE 0x00
#define ACTION_RADIO_SEND_NORMAL 0x01
#define ACTION_RADIO_SEND_SINGLE_CHAR 0x02

// Byte id stuff
#define OPENBCI_BYTE_ID_RESEND 0xFF

// Stream packet EOTs
#define OPENBCI_STREAM_PACKET_HEAD 0x41
#define OPENBCI_STREAM_PACKET_TAIL 0xC0

// Special host codes
#define OPENBCI_HOST_TIME_SYNC '<'
#define OPENBCI_HOST_TIME_SYNC_ACK ','

// Commands
#define OPENBCI_HOST_PRIVATE_CMD_KEY 0xF0
#define OPENBCI_HOST_CMD_CHANNEL_GET 0x00
#define OPENBCI_HOST_CMD_CHANNEL_SET 0x01
#define OPENBCI_HOST_CMD_CHANNEL_SET_OVERIDE 0x02
#define OPENBCI_HOST_CMD_POLL_TIME_GET 0x03
#define OPENBCI_HOST_CMD_POLL_TIME_SET 0x04
#define OPENBCI_HOST_CMD_BAUD_DEFAULT 0x05
#define OPENBCI_HOST_CMD_BAUD_FAST 0x06
#define OPENBCI_HOST_CMD_SYS_UP 0x07

// Raw data packet types/codes
#define OPENBCI_PACKET_TYPE_RAW_AUX      = 3; // 0011
#define OPENBCI_PACKET_TYPE_STANDARD     = 0; // 0000
#define OPENBCI_PACKET_TYPE_TIME_SYNCED  = 1; // 0001
#define OPENBCI_PACKET_TYPE_USER_DEFINED = 2; // 0010

// Possible baud rates
#define OPENBCI_BAUD_RATE_DEFAULT 115200
#define OPENBCI_BAUD_RATE_FAST 230400

#endif
