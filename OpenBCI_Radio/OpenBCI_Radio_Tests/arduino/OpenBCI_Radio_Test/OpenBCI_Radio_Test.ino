#include <RFduinoGZLL.h>
#include "OpenBCI_Radio.h"

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); 
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available()) {
    Serial.read();
    test(); 
  }
}

void test() {
  boolean allTestsPassed = true;
  Serial.println("\nTests Begin\n");
  
  allTestsPassed = testCheckSum() && allTestsPassed;
  allTestsPassed = testByteIdMake() && allTestsPassed;
  
  if (allTestsPassed) {
    Serial.println("\nAll tests passed!");
  }
  
  Serial.println("\nTests End\n");
}

boolean testCheckSum() {
  boolean allPassed = true;
  
  allPassed = testCheckSumMake() && allPassed;
    
  return allPassed;
}

boolean testCheckSumMake() {
  char temp[2];
  temp[0] = 'a';
  temp[1] = 'j';  
  
  char checkSum = OpenBCI_Radio.checkSumMake(temp,2);
  
  return assertEqualChar(checkSum,6,"Get's correct check sum for given char *");
}

boolean testByteIdMake() {
  boolean allPassed = true;
  
  allPassed = testByteIdMake_IsStreamPacket() && allPassed;
  allPassed = testByteIdMake_NotStreamPacket() && allPassed;
  allPassed = testByteIdMake_PacketNumber1() && allPassed;
  allPassed = testByteIdMake_PacketNumber9() && allPassed;
  allPassed = testByteIdMake_GetPacketNumber() && allPassed;
  allPassed = testByteIdMake_GetCheckSum() && allPassed;
    
  return allPassed;
}

boolean testByteIdMake_IsStreamPacket() {
  
 char byteId = OpenBCI_Radio.byteIdMake(true,0,NULL,0);
  
 return assertGreaterThan(byteId,0x7F,"Streaming byteId has 1 in MSB");  
}

boolean testByteIdMake_NotStreamPacket() {
  
 char byteId = OpenBCI_Radio.byteIdMake(false,0,NULL,0);
  
 return assertLessThan(byteId,0x80,"Non streaming byteId has 0 in MSB");  
}

boolean testByteIdMake_PacketNumber1() {
  
 char byteId = OpenBCI_Radio.byteIdMake(false,1,NULL,0);
  
 return assertEqualChar(byteId,0x08,"Packet Number of 1 in byteId");  
}

boolean testByteIdMake_PacketNumber9() {
  
 char byteId = OpenBCI_Radio.byteIdMake(false,9,NULL,0);
  
 return assertEqualChar(byteId,0x48,"Packet Number of 9 in byteId");  
}

boolean testByteIdMake_GetPacketNumber() {
  
 
 int expectedPacketNumber = 10;
 char byteId = 0x50; 
 
 int actualPacketNumber = OpenBCI_Radio.byteIdGetPacketNumber(byteId);
  
 return assertEqualInt(actualPacketNumber,expectedPacketNumber,"Extracts packet number from byteId");  
}

boolean testByteIdMake_GetCheckSum() {
  char checkSum = OpenBCI_Radio.byteIdGetCheckSum(0x1B);
  
  return assertEqualChar(checkSum,0x03,"Extracts check sum from byte id");
}

char *testPacket() {
  
 boolean streamPacket = false;
 int packetNumber = 0; 
 char data[2];
 data[0] = 'a';
 data[1] = 'j';
  
 char *output;
  
 output = data;
  
 return output;
}

boolean assertEqualChar(char a, char b, char *msg) {
  return verbosePrintResult(a == b,msg);  
}

boolean assertGreaterThan(char a, char b, char *msg) {
  return verbosePrintResult(a > b,msg); 
}

boolean assertLessThan(char a, char b, char *msg) {
  return verbosePrintResult(a < b,msg); 
}

boolean assertEqualInt(int a, int b, char *msg) {
  return verbosePrintResult(a == b,msg);  
}

boolean verbosePrintResult(boolean testPassed, char *msg) {
   if (testPassed) {
     Serial.print("Passed - "); 
     Serial.println(msg);
     return true;
   } else  {
     Serial.print("****Failed - ");
     Serial.println(msg);  
     return false;
   } 
}
