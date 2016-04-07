#include <Wire.h>

const int kTransducer = A0;

const int kI2cSlaveAddress = 0x77;
const char kResponsePressureNotYetAvailable[] = {(char) 0x23, (char) 0x23, (char) 0x23};
const char kResponsePressureAvailable = (char) 0x24;

void RequestReceived() {
  // Read from the transducer; this is the only value that the master needs.
  int val = analogRead(kTransducer);

  unsigned char to_write[3];
  to_write[0] = kResponsePressureAvailable;
  to_write[1] = (unsigned char) val;
  to_write[2] = (unsigned char) (val >> 8);

  Wire.write(to_write, 3);
}

void setup() {
  // Set up the serial connection.
  Serial.begin(9600);

  // Set up the I2C connection.
  Wire.begin(kI2cSlaveAddress);
  Wire.onRequest(RequestReceived);  
}

void loop() {
  // Unused.
}
