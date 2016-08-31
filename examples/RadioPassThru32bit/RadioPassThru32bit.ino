
/**
 * OpenBCI Dongle Dummy
 *    This code is used to turn the Dongle into a device that will pass FTDI signals
 *    It puts pins 0 and 1 into high Z so they won't interfere with the serial transmission signals.
 *
 * Written by Push The World LLC 2016 inspired by Joel Murphy, Leif Percifield
 *  and Conor Russomanno. You should have recieved a copy of the license when
 *  you downloaded from github. Free to use and share. This code presented for
 *  use as-is.
 */
#include <RFduinoGZLL.h>
#include "OpenBCI_Radios.h"

void setup() {
  // Initialize the radio in pass thru
  radio.begin(OPENBCI_MODE_PASS_THRU);
}

void loop() {
  // Give LED feedback to the user
  radio.ledFeedBackForPassThru();
}
