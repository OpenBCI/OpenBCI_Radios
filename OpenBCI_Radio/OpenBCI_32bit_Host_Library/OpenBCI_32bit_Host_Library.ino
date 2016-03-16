#include "OpenBCI_Radio.h"

OpenBCI_Radio Radio; // use 

void setup() {
  // put your setup code here, to run once:
  Radio.begin(OPENBCI_MODE_HOST,4);
}

void loop() {
  // put your main code here, to run repeatedly:
    
  if (Radio.readSerial()) {
    Radio.writeSerial();
  }
  
  if (Radio.readRadio()) {
    Radio.writeRadio();
  }
}
