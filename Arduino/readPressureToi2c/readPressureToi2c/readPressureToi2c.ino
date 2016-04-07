#include <Wire.h>

const int kTransducer = A0;

const int kMaxPressure = 634;

const int kI2cSlaveAddress = 0x77;
const char kResponsePressureNotYetAvailable[] = {(char) 0x23, (char) 0x23, (char) 0x23};
const char kResponsePressureAvailable = (char) 0x24;
const int kReadPressureCommand = 0x42;

unsigned int pressureValue;
bool measure_requested = false;
bool pressure_measured = false;

void CommandReceived(int numBytes) {
  //Serial.println(F("Received command from ADUCM350!"));
  while (Wire.available()) {
    unsigned int num = (unsigned int) Wire.read();
    if (num == kReadPressureCommand) {
      //Serial.println(F("ADUCM350 requested cuff pressure."));
      measure_requested = true;
    }
  }
  //Serial.println("CommandReceived");
}



void RequestReceived() {
  //if (!pressure_measured) {
  //  Wire.write(kResponsePressureNotYetAvailable, 3);
  //} else {
    char to_write[3];
    to_write[0] = kResponsePressureAvailable;
    to_write[1] = (char) analogRead(kTransducer);
    to_write[2] = (char) (analogRead(kTransducer) >> 8);

    Wire.write(to_write, 3);
  //}
  //Serial.println("RequestReceived");
  //Serial.println(analogRead(kTransducer));
}



void setup() {
  // Set up the serial connection.
  Serial.begin(9600);

  // Set up the I2C connection.
  Wire.begin(kI2cSlaveAddress);
  Wire.onReceive(CommandReceived);
  Wire.onRequest(RequestReceived);
  
}

void loop() {
  // put your main code here, to run repeatedly:
   if (!measure_requested) {
    return;
  }
  //Serial.println("measure requested");
  

}
