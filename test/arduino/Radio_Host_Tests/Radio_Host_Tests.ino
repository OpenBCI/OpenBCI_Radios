#include <RFduinoGZLL.h>
#include "OpenBCI_Radios.h"
#include "PTW-Arduino-Assert.h"

void setup() {
    // Get's serial up and running
    Serial.begin(115200);
    test.setSerial(Serial);
}

void loop() {
    // Start tests by just sending a command
    if (Serial.available()) {
        Serial.read();
        go();
    }
}

void go() {
    // Start the test
    test.begin();

    testProcessOutboundBuffer();

    test.end();
}

void testProcessOutboundBuffer() {
    testProcessOutboundBufferCharSingle();
}

void testProcessOutboundBufferCharSingle() {
    test.describe("processOutboundBufferCharSingle");

    test.assertEqualInt((int)radio.processOutboundBufferCharSingle('<'),ACTION_RADIO_SEND_NORMAL,"A < still results in sending a packet");
    test.assertEqualInt((int)radio.processOutboundBufferCharSingle(0x00),ACTION_RADIO_SEND_NONE,"Host channel query won't send packets");
    test.assertEqualInt((int)radio.processOutboundBufferCharSingle('a'),ACTION_RADIO_SEND_NORMAL,"Sends packet with A");
    test.assertEqualInt((int)radio.processOutboundBufferCharSingle(0xF6),ACTION_RADIO_SEND_NORMAL,"Sends packet with 0xF6");
}
