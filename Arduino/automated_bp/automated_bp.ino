#include <AFMotor.h>
#include <Wire.h>

AF_DCMotor motor(1);

const int kMotorPin = 5;
const int kSolValve = 7;
const int kTransducer = A0;
const int kMaxPressure = 634;
const int kMotorSpeed = 200;
const int kI2cSlaveAddress = 0x77;

bool has_finished = false;

void TransmitPressureViaI2C(int diastole, int systole) {
  // TODO
}

void setup() {
  // Set up the serial connection.
  Serial.begin(9600);

  // Set up the I2C connection.
  Wire.begin(kI2cSlaveAddress);
  Wire.onReceive(printReceived);

  // Configure the solenoid valve.
  pinMode(solValve, OUTPUT);

  // Configure the motor.
  motor.setSpeed(kMotorSpeeed);
}

void loop() {
  if (has_finished) {
    return;
  }

  // Start the motor and wait a couple seconds for the pump to inflate.
  motor.run(RELEASE);
  delay(2000);

  // Open the solenoid valve.
  analogWrite(kSolValve, 674);  // 674 == 3.3V

  // Continously read the pressure until we hit the pressure shut-off value.
  int pressure = analogRead(transducer);
  while (pressure < kMaxPressure) {
    pressure = analogRead(transducer);
    Serial.println(pressure);
    delay(55);
  }

  // Shut off the motor and wait a few seconds to restrict blood flow.
  motor.run(BREAK);
  delay(5000);

  int x[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  int time[] = {300, 300, 300, 300, 300, 300, 300, 300,
                400, 400, 400, 400, 400, 400, 400};
  int y[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  int p1;
  int p2;
  int diastole = 1024;
  int systole = 0;
  int pressureReadings[100];

  // Turn the valve on and off over time.
  for (int j = 0; j < 15; j++) {
    // Shut the valve off.
    analogWrite(solValve, 0);
    delay(100);
    analogWrite(solValve, 674);

    // Turn the valve on.
    delay(250);

    // Read pressure values.
    for (int i = 0; i < 100; i++) {
      pressureReadings[i] = analogRead(transducer);
      delay(10);
      Serial.println(pressureReadings[i]);

      // Check for heartbeats, which can be detected if the previous
      // values are greater than the current values.
      if ((pressureReadings[i] - 3) > pressureReadings[i - 1]) {
        Serial.println("Heartbeat detected.");
        Serial.println(pressureReadings[i] - 3);
        if (pressureReadings[i] > systole) {
          systole = pressureReadings[i];
          Serial.println("Updated systolic pressure:");
          Serial.println(systole);
        }
        if (pressureReadings[i] < diastole) {
          diastole = pressureReadings[i];
          Serial.println("Updated diastolic pressure:");
          Serial.println(diastole);
        }
      }
    }
    // delay(200);
    // y[j] = max(p1, p2) - pressure;

    Serial.println("Final diastolic pressure:");
    Serial.println(diastole);
    Serial.println("Final systolic pressure:");
    Serial.println(systole);

    TransmitPressureViaI2C(diastole, systole);
  }

  // Release the solenoid valve.
  analogWrite(solValve, 0);
  has_finished = true;
}

// // Mean Arterial pressure code?
//   int maxm = 0;
//   int i;
//
//   for(i=5; i<15; i++){
//     if(y[i]>maxm){
//       maxm = y[i];
//     }
//   }
//
//   int index[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//   int k;
//   int l=0;
//
//   for (k=5; k<15; k++){
//     if(y[k]==maxm){
//       index[k-5] = k;
//       l++;
//     }
//
//   }
//
// int total = 0;
// int a;
// for(a=0; a<1; a++){
//   total = x[index[a]] + total;
//
// }
//
// int MAP = ((total / l)-550)*200/102;
// Serial.println("Mean Arterial pressure");
// Serial.print (MAP);
// Serial.print ("mmHg");
