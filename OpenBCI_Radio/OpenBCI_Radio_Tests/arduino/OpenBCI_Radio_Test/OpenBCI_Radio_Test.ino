#include <RFduinoGZLL.h>
#include "OpenBCI_Radio.h"


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  
}

void byteIdMakeTest() {
  char *data;
  data = testPacket();
  
  char byteId = OpenBCI_Radio.byteIdMake(false,0,
}

char *testPacket() {
  
  boolean streamPacket = false;
  int packetNumber = 0; 
  char data[1];
  data[0] = 1;
  
  char *output;
  
  output = data;
  
  return output;
}
