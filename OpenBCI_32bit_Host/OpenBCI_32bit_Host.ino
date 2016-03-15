/*
This sketch is OpenBCI_32bit specific and sets up a HOST module
You need to add cusom files to your application software (sorry)
If you want to modify this firmware, go to RFduino and download their latest release
Install RFduino libraries as instructed from their website
go here (there) to download the adjusted library components,
and follow the instalation instructions to use the custom tools for OpenBC

Since the Device must initiate communication, the
device "polls" the Host evey 50mS when not sending packets.
Host is connected to PC via USB VCP (FTDI).
Device is connectedd to uC (PIC32MX250F128B with UDB32-MX2-DIP).
The code switches between 'normal' mode and 'streamingData' mode.
Normal mode expects a call-response protocol between the Host PC and Device uC.
Normal mode supports avrdude and will allow over-air upload to Device-connected uC.

StreamingData mode expects a continuous Serial stream of data from the uC.
In streamingData mode, Host insterts a pre-fix and post-fix to the data for PC coordination.

Single byte serial messages sent from PC are modified by the Host to include a '+' before and after
This is to avoid an error experienced when the uC gets a 'ghost' command byte during streamData mode

Made by Joel Murphy with Leif Percifield and Conor Russomanno, Summer 2014
Free to use and share. This code presented for use as-is. wysiwyg.

* ASCII commands are received on the serial port to configure and control
* Serial protocol uses '+' immediately before and after the command character
* We call this the 'burger' protocol. the '+' are the buns. Example:
* To begin streaming data, send '+b+'
* This software is provided as-is with no promise of workability
* Use at your own risk, wysiwyg.
*/

#include <RFduinoGZLL.h>  // using the Gazelle Stack

device_t role = HOST;  // This is the HOST code!
int LED = 2;           // blue LED on GPIO2

const int numBuffers = 20;              // buffer depth
char serialBuffer[numBuffers] [32];  	// buffers to hold serial data
int bufferLevel = 0;                    // counts which buffer array we are using
int serialIndex[numBuffers];            // Buffer position counter
int serialBuffCounter = 0;
unsigned long serialTimer;              // used to time end of serial message
boolean serialToSend = false;           // set when serial data is ready to go to serial
boolean serialTiming = false;           // set to start serial timer

const int radioBuffMax = 512;
char radioBuffer[radioBuffMax];		// buffer to hold radio data
int radioIndex = 0;                  	// used in sendToHost to protect len value
int packetCount = 0;                    // used to keep track of packets in received radio message
int packetsReceived = 0;                // used to count incoming packets
boolean radioToSend = false;            // set to send data from Device
int resetPin = 6;                       // GPIO6 connected to Arduino MCLR pin through 0.1uF
int resetPinValue;                      // this is not used
char RFmessage[1];                      // can't get on the radio without an array
boolean sendRFmessage = false;          // flag to send radio to radio message

boolean streamingData = false;          // flag to get into streamingData mode
int numBytes;                           // counter for receiving/sending stream
int tail = 0;                           // used when streaming to make ring buffer
boolean streamToSend = false;           // send radio data to serial port using ring buffer and pre/post-fix
unsigned long totalPacketsReceived = 0; // used for verbose
unsigned long streamingIdleTimer;	// used to escape streamingData mode


void setup(){
  RFduinoGZLL.channel = 18;  // use 2 to 25
  RFduinoGZLL.begin(role);   // start the GZLL stack
  Serial.begin(115200);      // start the serial port

  initBuffer();  // prime the serialBuffer

  pinMode(resetPin,INPUT);  // DTR from FTDI routed to GPIO6 through slide switch
  pinMode(LED,OUTPUT);    // blue LED on GPIO2
  digitalWrite(LED,HIGH); // trun on blue LED!

}



void loop(){

  if(serialTiming){                   // if the serial port is active
    if(millis() - serialTimer > 3){   // check for idle time out
      serialTiming = false;	      // clear serialTiming flag
      if(serialIndex[0] == 2){      // single byte messages from uC are special we need to burgerize them
        testSerialByte(serialBuffer[0][1]);       // could be command to streamData, go sniff it
        serialBuffer[0][2] = serialBuffer[0][1];  // move the command byte into patty position
        serialBuffer[0][1] = serialBuffer[0][3] = '+';  // put the buns around the patty
        serialIndex[0] = 4;                       // now we're sending the burger protocol
      }                                           // if we started a buffer and didn't add any bytes,
      if(serialIndex[bufferLevel] == 0){bufferLevel--;}   // don't send more buffers than we have!
      serialBuffer[0][0] = bufferLevel +1;  // drop the packet count into zero position (Device knows where to find it)
      serialBuffCounter = 0;              // get ready to count the number of packets we send
      serialToSend = true;                // set serialToSend flag. transmission starts on next packet from Device
    }
  }

  if(streamingData && (millis() - streamingIdleTimer) > 1000){
    streamingData = false;  // if for some reason the data stream fails
    streamToSend = false;   // time-out of streamingData mode and reset the resetables
    radioIndex = 0;
    Serial.println("stream timed out");
  }

  if(streamToSend){       // when we are streaming radio data
    Serial.write(0xA0);                // send the pre-fix
    for(int i=0; i<numBytes; i++){
      Serial.write(radioBuffer[tail]); // using ring buffer
      tail++; if(tail == radioBuffMax){tail = 0;}
    }
    Serial.write(0xC0);              // send the post-fix
    streamToSend = false;
    streamingIdleTimer = millis();   // reset streaming idle timer
  }

  if(radioToSend){  // when radio data is ready to send
    for(int i=0; i<radioIndex; i++){
      Serial.write(radioBuffer[i]);  // send it out the serial port
    }
    radioIndex = 0;
    radioToSend = false;    // reset radioToSend flag
  }




  if(Serial.available()){   // when the serial port is acive
    while(Serial.available() > 0){    // collect the bytes in 2D array
      serialBuffer[bufferLevel][serialIndex[bufferLevel]] = Serial.read();
      serialIndex[bufferLevel]++;           // count up the buffer size
      if(serialIndex[bufferLevel] == 32){	  // when the buffer is full,
        bufferLevel++;			  // next buffer please
      }
    }
    serialTiming = true;      // turn on the serial idle timer
    serialTimer = millis();   // set the serial idle clock
  }

}

void RFduinoGZLL_onReceive(device_t device, int rssi, char *data, int len){

  if(serialToSend){
    // .sentToDevice(device_t device, char buffer, int length)
    RFduinoGZLL.sendToDevice(device,serialBuffer[serialBuffCounter], serialIndex[serialBuffCounter]);
    serialBuffCounter++;	// get ready for next buffered packet
    if(serialBuffCounter == bufferLevel +1){// when we send all the packets
      serialToSend = false; 		    // put down bufferToSend flag
      bufferLevel = 0;			// initialize bufferLevel
      initBuffer();         // initialize serialBuffer
    }
  }

  if(len > 0){
    int startIndex = 0;	// get ready to read this packet
    if(packetCount == 0){	// if this is the first packet in transaction
      packetCount = data[0];	// get the number of packets to expect in message
      startIndex = 1;	// skip the first byte of the first packet when retrieving radio data
    }
    for(int i = startIndex; i < len; i++){
      radioBuffer[radioIndex] = data[i];  // retrieve the packet
      radioIndex++; if(radioIndex == radioBuffMax){radioIndex = 0;}// ring
    }
    packetsReceived++;
    if(packetsReceived == packetCount){	// we got all the packets
      packetsReceived = 0;
      packetCount = 0;
      // totalPacketsReceived++;          // this is used for verbose feedback
      if(streamingData){
        numBytes = len-1;                 // set the output byte count based on len
        streamToSend = true;              // set the stream packet flag
      }else{
        radioToSend = true;               // set serial pass flag
      }
    }
  }


}// end of onReceive

// sniff the serial command sent from PC to uC
void testSerialByte(char z){
  switch(z){
    case 'b':  // PC wants to stream data
    streamingData = true;  // enter streaimingData mode
    streamingIdleTimer = millis();
    radioIndex = 0;
    tail = 0;
    break;

    case 's':  // PC sends 's' to stop streaming data
    streamingData = false;  // get out of streamingData mode
    radioIndex = 0;         // reset radioBuffer index
    break;

    default:
    break;
  }
}

void initBuffer(){
  serialIndex[0] = 1;     // make room for the packet checkSum!
  for(int i=1; i<numBuffers; i++){
    serialIndex[i] = 0;   // initialize indexes to 0
  }
}



// end
