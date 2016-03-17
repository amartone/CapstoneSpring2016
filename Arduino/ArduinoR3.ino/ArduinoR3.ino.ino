/*************************************************************
Motor Shield 1-Channel DC Motor Demo
by Randy Sarafan

For more information see:
http://www.instructables.com/id/Arduino-Motor-Shield-Tutorial/

*************************************************************/

const int solValve = 7;
const int transducer = A3;
int END = 0; //counter to run program once and end

void setup() {
  Serial.begin(9600);           // set up Serial library at 9600 bps
  //Setup Channel A
  Serial.println("setting up motor");
  pinMode(12, OUTPUT); //Initiates Motor Channel A pin
  pinMode(9, OUTPUT); //Initiates Brake Channel A pin
  pinMode(solValve, OUTPUT);

  digitalWrite(12, HIGH); //Establishes forward direction of Channel A
  digitalWrite(9, LOW);   //Disengage the Brake for Channel A
  
  //Open solenoid valve
  analogWrite(solValve, 674); //674 == 3.3V
  

}

void readPressure(int delayVal){
  int pressureReadings[100];
  for(int i=0; i<100; i++){
      pressureReadings[i] = analogRead(transducer);
      delay(delayVal);
      Serial.println(pressureReadings[i]);
      }
}

//Figure out ratio 
//450 = 160
//500 = 180
//550 = 200
//575 = 210

//Inflate to 150mmHg
//Check for pulse
//if pulse doesn't exist
    //Inflate to 180
    //check for pulse
    //if pulse doesnt exist
      //inflate to 210
      //check for pulse
      //if no pulse exists
        //throw error message, check pads, etc.

void loop(){

if (END==0){
  delay(2000);
  analogWrite(solValve, 674); //674 == 3.3V
  
  //Read Pressure
  int pressure = analogRead(transducer);
  
  //forward @ full speed
//  digitalWrite(9, HIGH); //Eengage the Brake for Channel A
  analogWrite(3, 255);   //Spins the motor on Channel A at full speed
 
  Serial.println("Spinning the motor at full speed");
  
//  delay(3000);
//  Serial.println("Engaging motor break");
        
  while(pressure < 575){  
  pressure = analogRead(transducer);
  Serial.println(pressure);
  delay(55);
  }
  
 
  digitalWrite(9, HIGH); //Eengage the Brake for Channel A
  
  int diastole = 1024;
  int systole = 0;
  int pressureReadings[100];
  int delayVal=10;

//Turn the valve on and off overtime
  for (int j=0; j<15; j++){
     analogWrite(solValve, 0); //shut the valve off
     Serial.println("Valve shut off");

    readPressure(delayVal);
    analogWrite(solValve, 674); //turn the valve on
    readPressure(delayVal);
    //Read pressure values
//    for(int i=0; i<100; i++){
//    pressureReadings[i] = analogRead(transducer);
//    delay(10);
//    Serial.println(pressureReadings[i]);
//
//    //Check for heartbeats
//    //If the previous values are greater than current values
//    if ((pressureReadings[i]-3) > pressureReadings[i-1]){
//      Serial.println("Hearbeat detected");
//      Serial.println(pressureReadings[i]-3);
//      if (pressureReadings[i]>systole){
//        systole = pressureReadings[i];
//        Serial.println("Systole");
//        Serial.println(systole);
//      }
//      if (pressureReadings[i]<diastole){
//         diastole = pressureReadings[i];
//         Serial.println("Diastole");
//         Serial.println(diastole);
//      }
//    }
//
//    }
//   // delay(200);
//    //y[j] = max(p1, p2) - pressure;
//
//    Serial.println("Diastole");
//    Serial.println(diastole);
//    Serial.println("Systole");
//    Serial.println(systole);
//  }

  analogWrite(solValve, 0);
  END++;
  }
}
}
