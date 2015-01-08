/* 

  OpenBCI Dongle Dummy
    This code is used to turn the Dongle into a device that will pass FTDI signals
    It puts pins 0 and 1 into high Z so they won't interfere with the serial transmission signals.
    
    Made by Joel Murphy, Summer 2014
    
*/

int LED = 2;

void setup() {
  pinMode(0,OUTPUT_D0H1);  // output is highZ when logic 0, HIGH when logic 1
  pinMode(1,OUTPUT_D0H1);
  digitalWrite(0,LOW);
  digitalWrite(1,LOW);
  
  pinMode(LED,OUTPUT);
}

void loop() {
  
  digitalWrite(LED,HIGH);
  delay(600);
  digitalWrite(LED,LOW);
  delay(200); 
  
}
