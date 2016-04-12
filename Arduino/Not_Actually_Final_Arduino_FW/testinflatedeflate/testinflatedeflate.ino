// Pump Motor Parameters
const int kMotorDirectionPin = 12;
const int kMotorBrakePin = 9;
const int kMotorSpeedPin = 3;
const int kMotorSpeed = 200;

// Solenoid Valve Parameters
const int kSolenoidValvePin = 7;

// Pressure Transducer Parameters
const int kTransducerPin = A3;

void setup() {
  Serial.begin(9600);
  Serial.println("Waiting...");
  delay(4000);
  
  
  // Motor Pump Configurations
  pinMode(kMotorDirectionPin, OUTPUT);
  pinMode(kMotorBrakePin, OUTPUT);
  //Configure Pump in Forward Direction
  digitalWrite(kMotorDirectionPin, HIGH);
  // Configure Pump Brake on
  digitalWrite(kMotorBrakePin, HIGH);
  // Configure Pump Speed
  analogWrite(kMotorSpeedPin, kMotorSpeed);

  // Solenoid Valve Configurations.
  pinMode(kSolenoidValvePin, OUTPUT);

  // Read initial actual pressure from Transducer.
  int actual_pressure = analogRead(kTransducerPin);
  // Motor on, solenoid on.
  // Disengage brake for motor (motor starts).
  digitalWrite(kMotorBrakePin, LOW);
  // Turn on solenoid (shuts valve). 
  digitalWrite(kSolenoidValvePin, HIGH);

  Serial.println("Inflating...");
  while (actual_pressure < 460) { // 160 mmHg
    // Wait for the motor to inflate the cuff...
    actual_pressure = analogRead(kTransducerPin);
    Serial.println(actual_pressure);
    delay(500);
  }

  // Engage brake for motor (motor stops).
  digitalWrite(kMotorBrakePin, HIGH);

  Serial.println("Inflated. About to release");
  delay(2000);

  int actual_pressure_before, actual_pressure_during, actual_pressure_after;
  actual_pressure_after = actual_pressure;
  digitalWrite(kSolenoidValvePin, LOW);
  delay(40);
}

void loop() {
}

