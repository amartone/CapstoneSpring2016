#include <Wire.h>
#include <AFMotor.h>

AF_DCMotor motor(1);

const int kMotorPin = 5;
const int kSolValve = 7;
const int kTransducer = A0;
const int kMaxPressure = 634;
const int kMotorSpeed = 200;
const int kI2cSlaveAddress = 0x77;
const int kReadPressureCommand = 0x42;
const char kResponsePressureNotYetAvailable[] = {(char) 0x23, (char) 0x23, (char) 0x23, (char) 0x23, (char) 0x23};
const char kResponsePressureAvailable = (char) 0x24;

bool measure_requested = false;
unsigned int last_diastole, last_systole;
bool pressure_measured = false;

// TODO: these may be ISRs. Consider moving blocking work to loop() function.

void CommandReceived(int numBytes) {
  Serial.println(F("Received command from ADUCM350!"));
  while (Wire.available()) {
    unsigned int num = (unsigned int) Wire.read();
    if (num == kReadPressureCommand) {
      Serial.println(F("ADUCM350 requested cuff pressure."));
      measure_requested = true;
    }
  }
}

void RequestReceived() {
  if (!pressure_measured) {
    Wire.write(kResponsePressureNotYetAvailable, 5);
  } else {
    char to_write[5];
    to_write[0] = kResponsePressureAvailable;
    to_write[1] = (char) last_diastole;
    to_write[2] = (char) (last_diastole >> 8);
    to_write[3] = (char) last_systole;
    to_write[4] = (char) (last_systole >> 8);
    Wire.write(to_write, 5);
  }
}

void setup() {
  // Set up the serial connection.
  Serial.begin(9600);

  // Set up the I2C connection.
  Wire.begin(kI2cSlaveAddress);
  Wire.onReceive(CommandReceived);
  Wire.onRequest(RequestReceived);

  // Configure the solenoid valve.
  pinMode(kSolValve, OUTPUT);

  // Configure the motor.
  motor.setSpeed(kMotorSpeed);

  Serial.println(F("Waiting for signal from master to read pressure..."));
}

void loop() {
  if (!measure_requested) {
    return;
  }

  Serial.println(F("Starting pressure measurement..."));

  // TODO: remove me for actual motor operation!
  delay(5000);
  last_diastole = 123;
  last_systole = 456;
  pressure_measured = true;
  measure_requested = false;
  return;

  // Start the motor and wait a couple seconds for the pump to inflate.
  motor.run(RELEASE);
  delay(2000);

  // Open the solenoid valve.
  analogWrite(kSolValve, 674);  // 674 == 3.3V

  // Continously read the pressure until we hit the pressure shut-off value.
  Serial.println(F("Inflating..."));
  int pressure = analogRead(kTransducer);
  while (pressure < kMaxPressure) {
    pressure = analogRead(kTransducer);
    Serial.println(pressure);
    delay(55);
  }

  // Shut off the motor and wait a few seconds to restrict blood flow.
  motor.run(BRAKE);
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
    analogWrite(kSolValve, 0);
    delay(100);
    analogWrite(kSolValve, 674);

    // Turn the valve on.
    delay(250);

    // Read pressure values.
    for (int i = 0; i < 100; i++) {
      pressureReadings[i] = analogRead(kTransducer);
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
  }

  Serial.println("Final diastolic pressure:");
  Serial.println(diastole);
  Serial.println("Final systolic pressure:");
  Serial.println(systole);

  // Release the solenoid valve.
  analogWrite(kSolValve, 0);

  // Update the pressure values to send to the ADUCM350.
  last_diastole = diastole;
  last_systole = systole;
  pressure_measured = true;
  measure_requested = false;
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
