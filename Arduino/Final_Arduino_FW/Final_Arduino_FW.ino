#include <Wire.h>

// Pump motor parameters.
const int kMotorDirectionPin = 12;
const int kMotorBrakePin = 9;
const int kMotorSpeedPin = 3;
const int kMotorSpeed = 200;

// Solenoid valve parameters.
const int kSolenoidValvePin = 7;

// Pressure transducer parameters.
const int kTransducerPin = A3;
const unsigned int kMinimumPressure = 100;

// Initialize Pressure variables
volatile unsigned int pending_target_pressure = 0;
volatile bool reached_target_pressure = false;
unsigned int target_pressure = 0;
unsigned int actual_pressure = 0;
bool pressure_changed;

// I2C.
const unsigned char kResponseStillInflating = (unsigned char) 0x23;
const unsigned char kResponsePressureAvailable = (unsigned char) 0x24;
const unsigned char kRequestSetTargetPressure = (unsigned char) 0x42;
unsigned char actual_pressure_tx[3];

void receivePressure(int numBytes) {
  if (numBytes == 1) {
    // The ADUCM350 I2C implementation sends an address before a request;
    // ignore these bytes.
    Wire.read();
    return;
  }

  // Receive target pressure from ADUCM350.
  // First, eat the 0 value (the data address).
  Wire.read();

  unsigned char command = (unsigned char) Wire.read();
  if (command == kRequestSetTargetPressure) {
    unsigned int c1 = (unsigned int) Wire.read();
    unsigned int c2 = (unsigned int) Wire.read();
    pending_target_pressure = c1 | (c2 << 8);
  } else {
    Serial.println("Unknown command: ");
    Serial.println(command);
    while (Wire.available()) {
      Serial.println(Wire.read());
    }
  }
}

void sendPressure() {
  // Send actual pressure as a response to the ADUCM350.
  int val = analogRead(kTransducerPin);

  // Structure actual pressure data to appropriate bytes
  // to send over the I2C bus.
  if (reached_target_pressure) {
    actual_pressure_tx[0] = kResponsePressureAvailable;
  } else {
    actual_pressure_tx[0] = kResponseStillInflating;
  }
  actual_pressure_tx[1] = (unsigned char) val;
  actual_pressure_tx[2] = (unsigned char) (val >> 8);
  
  // Send response to the ADUCM350.
  Wire.write(actual_pressure_tx, 3);
}


void setup() {
  Serial.begin(9600);

  // Configure the I2C.
  Wire.begin(0x77);
  Wire.onRequest(sendPressure);
  Wire.onReceive(receivePressure);

  // Configure the motor pump.
  pinMode(kMotorDirectionPin, OUTPUT);
  pinMode(kMotorBrakePin, OUTPUT);
  // Set the motor to run in the forward direction.
  digitalWrite(kMotorDirectionPin, HIGH);
  // Set the motor's speed.
  analogWrite(kMotorSpeedPin, kMotorSpeed);
  // Set the motor to stopped.
  digitalWrite(kMotorBrakePin, HIGH);

  // Configure the solenoid and open its valve.
  pinMode(kSolenoidValvePin, OUTPUT);
  digitalWrite(kSolenoidValvePin, LOW);

  Serial.println("Ready!");
}

void loop() {
  // Set target pressure from pending value received from aducm350.
  pressure_changed = target_pressure != pending_target_pressure;
  if (!pressure_changed) {
    return;
  }
  target_pressure = pending_target_pressure;

  // Read the pressure from the transducer.
  actual_pressure = analogRead(kTransducerPin);

  if (target_pressure == 0) {
    Serial.println("Deflating cuff to 0...");
    reached_target_pressure = true;
    // If the target pressure is set to 0, open the solenoid and let
    // all the air out.
    digitalWrite(kMotorBrakePin, HIGH);
    // Turn on solenoid (shuts valve). 
    digitalWrite(kSolenoidValvePin, LOW);
  } else if (target_pressure > actual_pressure) {
    reached_target_pressure = false;

    Serial.print("Inflating cuff to ");
    Serial.print(target_pressure);
    Serial.println("...");

    // Shut the valve and engage the motor.
    digitalWrite(kSolenoidValvePin, HIGH);
    digitalWrite(kMotorBrakePin, LOW);

    // Over-inflate the cuff a bit because the pressure drops a bit more right after.
    int target_pressure_extra = (int) (target_pressure * 1.1);

    while (actual_pressure < target_pressure_extra) {
      // Wait for the motor to inflate the cuff...
      actual_pressure = analogRead(kTransducerPin);
    }

    // Engage the motor's brake so that it stops.
    digitalWrite(kMotorBrakePin, HIGH);
    Serial.println("Done.");
    reached_target_pressure = true;
  } else {
    // Just let the air leak out slowly.
    reached_target_pressure = true;
  }
}
