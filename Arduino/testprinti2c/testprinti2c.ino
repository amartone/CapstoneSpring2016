#include <Wire.h>

void setup() {
  Wire.begin(0x77);
  Serial.begin(9600);
  Wire.onReceive(printReceived);
  Serial.println(F("Ready!"));
}

void printReceived(int numBytes) {
  Serial.println(F("Received data from I2C!"));
  while (Wire.available()) {
    unsigned int num = (unsigned int) Wire.read();
    Serial.println(String(num));
  }
}

void loop() {
}

