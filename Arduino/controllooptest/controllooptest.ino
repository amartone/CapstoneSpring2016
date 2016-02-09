int sensorPin = A0;
int sensorValue = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  pinMode(7, OUTPUT);//solenoid valve
  pinMode(5, OUTPUT);//pump
  digitalWrite(5,HIGH);//turn on pump
  digitalWrite(7,HIGH);//close valve
}

void loop() {
  // put your main code here, to run repeatedly:
  sensorValue = analogRead(sensorPin);
  if (sensorValue > 300)
  {
//    Serial.println("we are above 100");
    digitalWrite(7, LOW);//open solenoid
    delay(50);
  }
  else
  {
    digitalWrite(7, HIGH);//close solenoid
  }
  Serial.println(sensorValue);
  
  

}
