int sensorPin = A3;
int sensorValue = 0;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  pinMode(7, OUTPUT); //solenoid valve
  digitalWrite(7,HIGH);//close valve
  
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(10);
  sensorValue = analogRead(sensorPin);
  Serial.println(sensorValue);
  
}
