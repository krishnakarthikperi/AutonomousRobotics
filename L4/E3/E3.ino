// Use the potentiometer as a voltage divider to change the brightness of an LED. 
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  pinMode(3,OUTPUT);
}
  
  // the loop routine runs over and over again forever:
void loop() {
  // read the input on analog pin 0:
  int sensorValue = analogRead(A0);
  // print out the value you read:
  Serial.println(sensorValue);
  delay(100);  // delay in between reads for stability
  analogWrite(3,sensorValue*(255.0/1024));
  // analogWrite(3,sensorValue);
}
  