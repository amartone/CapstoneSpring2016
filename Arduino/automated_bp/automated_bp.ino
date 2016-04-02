#include <AFMotor.h>

AF_DCMotor motor(1);

const int motorPin = 5;
const int solValve = 7;
const int transducer = A0;
const int kMaxPressure = 634;


void setup() {
  Serial.begin(9600);

  // Configure the solenoid valve.
  pinMode(solValve, OUTPUT);

  // Configure and turn on the motor.
  motor.setSpeed(200);
  motor.run(RELEASE);
}

void loop() {  
  if (END == 0) {
    delay(2000);
    // Open the solenoid valve
    analogWrite(solValve, 674);  // 674 == 3.3V

    // Start the pump.
    int speed = 255;  // Is this the max speed of the pump?
    //  analogWrite(motorPin, speed);

    // Read Pressure
    int pressure = analogRead(transducer);
    // Continously read the pressure until we hit 634.
    //  motor.run(FORWARD);
    while (pressure < kMaxPressure) {
      pressure = analogRead(transducer);
      Serial.println(pressure);
      delay(55);
    }

    // Shut off the motor.
    speed = 0;
    //  analogWrite(motorPin, speed);
    motor.setSpeed(speed);
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

    // Turn the valve on and off overtime
    for (int j = 0; j < 15; j++) {
      analogWrite(solValve, 0);  // shut the valve off
      delay(100);
      analogWrite(solValve, 674);  // turn the valve on
      delay(250);
      // Read pressure values
      for (int i = 0; i < 100; i++) {
        pressureReadings[i] = analogRead(transducer);
        delay(10);
        Serial.println(pressureReadings[i]);

        // Check for heartbeats
        // If the previous values are greater than current values
        if ((pressureReadings[i] - 3) > pressureReadings[i - 1]) {
          Serial.println("Hearbeat detected");
          Serial.println(pressureReadings[i] - 3);
          if (pressureReadings[i] > systole) {
            systole = pressureReadings[i];
            Serial.println("Systole");
            Serial.println(systole);
          }
          if (pressureReadings[i] < diastole) {
            diastole = pressureReadings[i];
            Serial.println("Diastole");
            Serial.println(diastole);
          }
        }
      }
      // delay(200);
      // y[j] = max(p1, p2) - pressure;

      Serial.println("Diastole");
      Serial.println(diastole);
      Serial.println("Systole");
      Serial.println(systole);
    }

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

    analogWrite(solValve, 0);
    motor.run(RELEASE);
    END++;
  }
}
