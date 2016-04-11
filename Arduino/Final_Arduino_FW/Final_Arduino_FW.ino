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
int pending_target_pressure = 0;
int target_pressure = 0;
int actual_pressure = 0;
unsigned char send_actual_pressure[3];
const char kResponsePressureAvailable = (char) 0x24;

void receivePressure(int pending_target_pressure){
  // Receive target pressure from aducm 350
  if (Wire.available()){
    pending_target_pressure = (int) Wire.read();
  }
}

void sendPressure() {
  // Send actual pressure as a response to the aducm 350

  // Structure actual pressure data to appropriate bytes
  //to send over the i2c bus
  send_actual_pressure[0] = kResponsePressureAvailable;
  send_actual_pressure[1] = (unsigned char) actual_pressure;
  send_actual_pressure[2] = (unsigned char) (actual_pressure >> 8);
  
  // Send structured actual pressure to the aducm 350
  Wire.write(send_actual_pressure, 3);
  
}


void setup() {

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

  }

void loop() {
  
  // Read actual pressure from Transducer.
  actual_pressure = analogRead(kTransducerPin);

  // Set a target pressure from pending valve
  // received from aducm350.
  target_pressure = pending_target_pressure;
  
  // Control loop to set the actual pressure to target pressure.
  if (actual_pressure < target_pressure) {
    // Motor on, solenoid on.
    // Disengage brake for motor (motor starts).
    digitalWrite(kMotorBrakePin, LOW);
    // Turn on solenoid (shuts valve). 
    digitalWrite(kSolenoidValvePin, HIGH); 
  } 
  else if (actual_pressure >= target_pressure && actual_pressure < target_pressure + 10){
    // Motor off, solenoid on.
    // Engage brake for motor (motor stops).
    digitalWrite(kMotorBrakePin, HIGH);
    //Turn on solenoid (shuts valve). 
    digitalWrite(kSolenoidValvePin, HIGH); 
  }
  else { 
    // Motor off, solenoid off.
    // engage brake for motor (motor stops).
    digitalWrite(kMotorBrakePin, HIGH); 
    // turn off solenoid (opens valve).
    digitalWrite(kSolenoidValvePin, LOW);
  }

}
