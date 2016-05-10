/**
 * OpenBCI Dongle Dummy
 *    This code is used to turn the Dongle into a device that will pass FTDI signals
 *    It puts pins 0 and 1 into high Z so they won't interfere with the serial transmission signals.
 *
 *    Made by Joel Murphy, Summer 2014
 *    Refactored by AJ Keller (@pushtheworldllc), Spring 2016
 */
void setup() {
  // put your setup code here, to run once:
  OpenBCI_Radio.begin(OPENBCI_MODE_PASS_THRU);
}

void loop() {
  OpenBCI_Radio.ledFeedBackForPassThru();
}
