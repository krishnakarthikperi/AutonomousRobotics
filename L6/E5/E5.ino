// C++ code
//
void setup()
{
  Serial.begin(9600);
  pinMode(3, OUTPUT);
}

void loop()
{
  float reading = analogRead(A0);
  float ledBrightness = (255*(1023-reading)/1024);
  analogWrite(3,ledBrightness);
}