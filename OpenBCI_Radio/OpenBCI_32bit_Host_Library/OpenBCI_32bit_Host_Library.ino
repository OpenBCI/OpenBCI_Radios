#include <RFduinoGZLL.h>
#include "OpenBCI_Radio.h"

void setup() {
  // put your setup code here, to run once:
  OpenBCI_Radio.begin(OPENBCI_MODE_HOST,4);
}

void loop() {
  // put your main code here, to run repeatedly:
    
  if (OpenBCI_Radio.readSerial()) {
    OpenBCI_Radio.writeSerial();
  }
  
  if (OpenBCI_Radio.readRadio()) {
    OpenBCI_Radio.writeRadio();
  }
}
