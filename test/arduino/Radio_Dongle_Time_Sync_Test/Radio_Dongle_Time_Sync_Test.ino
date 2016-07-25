int ledPin = 2;
char incomingByte = 0;
void setup() {
  // put your setup code here, to run once:
  pinMode(ledPin,OUTPUT);
  Serial.begin(115200);

}

void loop() {
  if (Serial.available()) {
    incomingByte = Serial.read();  // will not be -1
    // actually do something with incomingByte
    if (incomingByte=='0'){
     digitalWrite(ledPin, LOW);
    }
    if (incomingByte=='1'){
     digitalWrite(ledPin, HIGH);
    }
  }
                // wait for a second
}
