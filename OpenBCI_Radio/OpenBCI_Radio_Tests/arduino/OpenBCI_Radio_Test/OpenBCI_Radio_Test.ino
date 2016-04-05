#include <RFduinoGZLL.h>
#include "OpenBCI_Radio.h"

boolean verbosePrints = true;

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
  
  Serial.println("#checkSum");
  allPassed = testCheckSumMake() && allPassed;
  allPassed = testCheckSumMake_FundamentalOperation1() && allPassed;
  allPassed = testCheckSumMake_FundamentalOperation2() && allPassed;
  allPassed = testCheckSumsAreEqual_True() && allPassed;
  allPassed = testCheckSumsAreEqual_False() && allPassed;
    
  return allPassed;
}

boolean testCheckSumMake() {
  // Generate test packet
  char temp[3];
  temp[1] = 'a';
  temp[2] = 'j';
  
  char checkSum = OpenBCI_Radio.checkSumMake(temp + 1,2);
  
  boolean result = assertEqualChar(checkSum,5);
  
  if (verbosePrints) {
     verbosePrintResult(result,"CheckSumMake","Get's correct check sum for given char *");
  } 
  
  return result;
}

boolean testCheckSumMake_FundamentalOperation1() {
  // Generate test packet
  char temp1[3];
  temp1[1] = 'a';
  temp1[2] = 'j';
  
  char temp2[3];
  temp2[1] = 'a';
  temp2[2] = 'k';
  
  char checkSum1 = OpenBCI_Radio.checkSumMake(temp1 + 1,2);
  char checkSum2 = OpenBCI_Radio.checkSumMake(temp2 + 1,2);
  
  boolean result = assertGreaterThanChar(checkSum1,checkSum2);
  
  if (verbosePrints) {
     verbosePrintResult(result,"FundamentalOperation1","Data with one byte of 1 less than other has smaller check sum");
  } 
  
  return result;
}

boolean testCheckSumMake_FundamentalOperation2() {
  // Generate test packet
  char temp1[3];
  temp1[1] = 'a';
  temp1[2] = 'j';
  
  char temp2[3];
  temp2[1] = 'a';
  temp2[2] = 'k';
  
  char checkSum1 = OpenBCI_Radio.checkSumMake(temp1 + 1,2);
  char checkSum2 = OpenBCI_Radio.checkSumMake(temp2 + 1,2);
  
  boolean result = assertEqualInt((int)(checkSum1 - checkSum2),1);
  
  if (verbosePrints) {
     verbosePrintResult(result,"FundamentalOperation2","Check sums of two different packets changed by 1 bit shall be one 1 less");
  } 
  
  return result;
}

boolean testCheckSumsAreEqual_True() {
  // Generate test packet
  char temp[3];
  temp[1] = 'a';
  temp[2] = 'j';
  
  // Get byteId for this packet
  char byteId = OpenBCI_Radio.byteIdMake(false, 0, temp + 1, 2);
  // Set byteId for this packet
  temp[0] = byteId;
  
  boolean result = assertEqualBoolean(OpenBCI_Radio.checkSumsAreEqual(temp,3),true);
  
  if (verbosePrints) {
     verbosePrintResult(result,"CheckSumsAreEqual_True","Check sums verification works in good condition");
  } 
  
  return result;
}

boolean testCheckSumsAreEqual_False() {
  // Generate test packet
  char temp[3];
  temp[1] = 'a';
  temp[2] = 'j';
  
  // Get byteId for this packet
  char byteId = OpenBCI_Radio.byteIdMake(false, 0, temp + 1, 2);
  // Set byteId for this packet
  temp[0] = byteId & 0x00;
  
  boolean result = assertEqualBoolean(OpenBCI_Radio.checkSumsAreEqual(temp,3),false);
  
  if (verbosePrints) {
     verbosePrintResult(result,"CheckSumsAreEqual_False","Check sums verification works in bad condition");
  } 
  
  return result;
}

boolean testByteIdMake() {
  boolean allPassed = true;
  
  Serial.println("#byteId");
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
 
  boolean result = assertGreaterThanChar(byteId,0x7F);
  
  if (verbosePrints) {
     verbosePrintResult(result,"IsStreamPacket","Streaming byteId has 1 in MSB");
  } 
  
  return result;
  
// return assertGreaterThan(byteId,0x7F,"Streaming byteId has 1 in MSB");  
}

boolean testByteIdMake_NotStreamPacket() {
  
  char byteId = OpenBCI_Radio.byteIdMake(false,0,NULL,0);
 
  boolean result = assertLessThanChar(byteId,0x80);
  
  if (verbosePrints) {
     verbosePrintResult(result,"NotStreamPacket","Non streaming byteId has 0 in MSB");
  } 
  
  return result;
  
//  return assertLessThan(byteId,0x80,"Non streaming byteId has 0 in MSB");  
}

boolean testByteIdMake_PacketNumber1() {
  
  char byteId = OpenBCI_Radio.byteIdMake(false,1,NULL,0);
 
  boolean result = assertEqualChar(byteId,0x08);
  
  if (verbosePrints) {
     verbosePrintResult(result,"PacketNumber1","Can set packet number of 1 in byteId");
  } 
  
  return result;
  
// return assertEqualChar(byteId,0x08,"Packet Number of 1 in byteId");  
}

boolean testByteIdMake_PacketNumber9() {
  
  char byteId = OpenBCI_Radio.byteIdMake(false,9,NULL,0);
  
  boolean result = assertEqualChar(byteId,0x48);
  
  if (verbosePrints) {
     verbosePrintResult(result,"PacketNumber9","Can set packet number of 9 in byteId");
  } 
  
  return result;
  
// return assertEqualChar(byteId,0x48,"Packet Number of 9 in byteId");  
}

boolean testByteIdMake_GetPacketNumber() {
  
  int expectedPacketNumber = 10;
  char byteId = 0x50; 
 
  int actualPacketNumber = OpenBCI_Radio.byteIdGetPacketNumber(byteId);
 
  boolean result = assertEqualInt(actualPacketNumber,expectedPacketNumber);
  
  if (verbosePrints) {
     verbosePrintResult(result,"GetPacketNumber","Extracts packet number from byteId");
  } 
  
  return result;
  
// return assertEqualInt(actualPacketNumber,expectedPacketNumber,"Extracts packet number from byteId");  
}

boolean testByteIdMake_GetCheckSum() {
  char checkSum = OpenBCI_Radio.byteIdGetCheckSum(0x1B);
  
  boolean result = assertEqualChar(checkSum,0x03);
  
  if (verbosePrints) {
     verbosePrintResult(result,"GetCheckSum","Extracts check sum from byte id");
  } 
  
  return result;
  
//  return assertEqualChar(checkSum,0x03),"Extracts check sum from byte id");
}

char *testPacket() {
  
 boolean streamPacket = false;
 int packetNumber = 0; 
 char data[3];
 data[1] = 'a';
 data[2] = 'k';
 
 Serial.print("Data in testPacket creation: ");
 for (int i = 1; i < 3; i++) {
    Serial.println(data[i], HEX);
 }
  
 return data;
}

boolean assertEqualBoolean(boolean a, boolean b) {
  return a == b;  
}

boolean assertEqualChar(char a, char b) {
  return a == b;  
}

boolean assertGreaterThanChar(char a, char b) {
  return a > b; 
}

boolean assertLessThanChar(char a, char b) {
  return a < b; 
}

boolean assertEqualInt(int a, int b) {
  return a == b;  
}

void verbosePrintResult(boolean testPassed, char *testName, char *msg) {
   if (testPassed) {
     Serial.print("  Passed - "); 
     Serial.print(testName);
     Serial.print(" - ");
     Serial.println(msg);
   } else  {
     Serial.print("  ****Failed - ");
     Serial.print(testName);
     Serial.print(" - ");
     Serial.println(msg);  
   } 
}
