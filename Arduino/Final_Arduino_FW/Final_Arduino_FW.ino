#include <Wire.h>

// Pump Motor Parameters
const int kMotorDirectionPin = 12;
const int kMotorBrakePin = 9;
const int kMotorSpeedPin = 3;

// Solenoid Valve Parameters
const int kSolenoidValvePin = 7;

// Pressure Transducer Parameters
const int kTransducerPin = A3;

// initialize Pressure variables
unsigned int pending_target_pressure = 0;
unsigned int target_pressure = 0;
unsigned int actual_pressure = 0;
bool pressure_changed = false;
unsigned char send_actual_pressure[3];

const int kPressureReleaseBuffer = 10;
const unsigned char kResponsePressureAvailable = (unsigned char) 0x24;
const unsigned char kRequestSetTargetPressure = (unsigned char) 0x42;

void receivePressure(int pending_target_pressure){
  // Receive target pressure from aducm 350
  Wire.read(); // Eat the 0 value (the data "address", perhaps).
  unsigned char command = (unsigned char) Wire.read();
  if (command == kRequestSetTargetPressure) {
    unsigned int c1 = (unsigned int) Wire.read();
    unsigned int c2 = (unsigned int) Wire.read();
    pending_target_pressure = c1 | (c2 << 8);
    Serial.print("Setting target to ");
    Serial.println(pending_target_pressure);
  } else {
    Serial.println("Unknown command: ");
    Serial.println(command);
    while (Wire.available()) {
      Wire.read();
    }
  }
}

void sendPressure() {
  // Send actual pressure as a response to the aducm 350
  int val = analogRead(kTransducerPin);

  // Structure actual pressure data to appropriate bytes
  //to send over the i2c bus
  send_actual_pressure[0] = kResponsePressureAvailable;
  send_actual_pressure[1] = (unsigned char) val;
  send_actual_pressure[2] = (unsigned char) (val >> 8);
  
  // Send structured actual pressure to the aducm 350
  Wire.write(send_actual_pressure, 3);
  
}


void setup() {

  Serial.begin(9600);

  // i2c Configurations
  Wire.begin(0x77);
  Wire.onReceive(receivePressure);
  Wire.onRequest(sendPressure);
  
  // Motor Pump Configurations
  pinMode(kMotorDirectionPin, OUTPUT);
  pinMode(kMotorBrakePin, OUTPUT);
  //Configure Pump in Forward Direction
  digitalWrite(kMotorDirectionPin, HIGH);
  // Configure Pump Brake on
  digitalWrite(kMotorBrakePin, HIGH);
  // Configure Pump Speed
  analogWrite(kMotorSpeedPin, 75);

  // Solenoid Valve Configurations.
  pinMode(kSolenoidValvePin, OUTPUT);
  // Solenoid Set to Open (opens valve).
  digitalWrite(kSolenoidValvePin, LOW);
  
  // Read initial actual pressure from Transducer.
  actual_pressure = analogRead(kTransducerPin);

  Serial.println("Ready");
}

void loop() {
  // Set a target pressure from pending valve
  // received from aducm350.
  pressure_changed = target_pressure != pending_target_pressure;
  if (!pressure_changed) {
    return;
  }

  target_pressure = pending_target_pressure;

  // Control loop to set the actual pressure to target pressure.
  if (actual_pressure < target_pressure) {
    // Motor on, solenoid on.
    // Disengage brake for motor (motor starts).
    digitalWrite(kMotorBrakePin, LOW);
    // Turn on solenoid (shuts valve). 
    digitalWrite(kSolenoidValvePin, HIGH);

    while (actual_pressure < target_pressure) {
      // Wait for the motor to inflate the cuff...
      actual_pressure = analogRead(kTransducerPin);
    }

    // Engage brake for motor (motor stops).
    digitalWrite(kMotorBrakePin, HIGH);

  } else if (actual_pressure > target_pressure + kPressureReleaseBuffer) {
    // Turn off solenoid (open valve). 
    digitalWrite(kSolenoidValvePin, HIGH); 
    while (actual_pressure > target_pressure + kPressureReleaseBuffer) {
      // Wait for the cuff to drop down to the target pressure...
      actual_pressure = analogRead(kTransducerPin);
    }
    // Turn on solenoid (close valve). 
    digitalWrite(kSolenoidValvePin, HIGH); 
  }
}
