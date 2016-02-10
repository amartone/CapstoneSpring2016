
const int motorPin = 5;
const int solValve = 7;
const int transducer = A0;

int END = 0; //counter to run program once and end
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(solValve, OUTPUT); //solenoid valve
  pinMode(motorPin, OUTPUT); //pump
  
}

void loop() {
  // put your main code here, to run repeatedly:

 if (END==0){
  delay(2000);
  analogWrite(solValve, 674); //674 == 3.3V

  // Start pump
  int speed = 255;
  analogWrite(motorPin, speed);

  //Read Pressure b/f loop
  int pressure = analogRead(transducer);

  while(pressure < 634){
  pressure = analogRead(transducer);
  Serial.println(pressure);
  delay(55);
 
  }

  //Shutoff Motor
  speed = 0;
  analogWrite(motorPin, speed);
  delay(5000);

  //Dump valve vode 

  int x[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  int time[] = {300, 300, 300, 300, 300, 300, 300, 300, 400, 400, 400, 400, 400, 400, 400};
  int y[]={0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  int p1;
  int p2;
  int j;
  int diastole = 1024;
  int systole = 0;

  int pressureReadings[100];

  for (j=0; j<15; j++){
    analogWrite(solValve, 0);
    delay(100);
    analogWrite(solValve, 674);
    delay(250);
//    pressure = analogRead(transducer);
    //x[j] = pressure;
    for(int i=0; i<100; i++){
    pressureReadings[i] = analogRead(transducer);
    delay(10);
    Serial.println(pressureReadings[i]);
    

    if ((pressureReadings[i]-3) > pressureReadings[i-1]){
      Serial.println("Hearbeat detected");
      Serial.println(pressureReadings[i]-3);
      if (pressureReadings[i]>systole){
        systole = pressureReadings[i];
        Serial.println("Systole");
        Serial.println(systole);
      }
      if (pressureReadings[i]<diastole){
         diastole = pressureReadings[i];
         Serial.println("Diastole");
         Serial.println(diastole);
      }
    }
    
    }
   // delay(200);
    //y[j] = max(p1, p2) - pressure;

    Serial.println("Diastole");
    Serial.println(diastole);
    Serial.println("Systole");
    Serial.println(systole);
  }

  int maxm = 0;
  int i;
  
  for(i=5; i<15; i++){
    if(y[i]>maxm){
      maxm = y[i];
    }
  }

  int index[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  int k;
  int l=0;
  
  for (k=5; k<15; k++){
    if(y[k]==maxm){
      index[k-5] = k;
      l++;
    }
    
  }

int total = 0;
int a;
for(a=0; a<1; a++){
  total = x[index[a]] + total;

}

int MAP = ((total / l)-550)*200/102;
Serial.println("Mean Arterial pressure");
Serial.print (MAP);
Serial.print ("mmHg");

analogWrite(solValve, 0); 
END++;
  
 }
}
