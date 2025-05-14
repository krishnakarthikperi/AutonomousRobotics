// C++ code
//
float fixedResistance = 1000.0;
void setup()
{
  Serial.begin(9600);
}

void loop()
{
  float photoResistorReading = analogRead(A0);
  float fixedResistanceReading = analogRead(A1);
  Serial.print(photoResistorReading);
  Serial.print("|");
  Serial.print(fixedResistanceReading);
  Serial.print("|");
  float photoResistance = fixedResistanceReading*fixedResistance/(photoResistorReading+fixedResistanceReading);
  Serial.print("Photoresistance = ");
  Serial.println(photoResistance);
  delay(500);
}