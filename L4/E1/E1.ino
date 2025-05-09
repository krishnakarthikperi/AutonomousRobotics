// C++ code
//
void setup()
{
  Serial.begin(9600);
}

void loop()
{
  float reading = analogRead(A0);
  float voltage = 5*reading/1024;
  Serial.println(voltage);
  delay(400);
}