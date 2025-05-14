// C++ code
//
/*
* The photoresistor is configured to have the analogRead value from 6 -> 679
* The potentiometer is configured to have the analogRead value from 0 -> 1023
*/

float maxAnalogPhotoresistor = 679;
float minAnalogPhotoresistor = 6;
float maxAnalogPotentiometer = 1023;
float minAnalogPotentiometer = 0;
float inputVoltage = 5.0;
float lastPotentioMeterReading = 0;  // the previous reading from the input pin
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers
float thresholdVoltage = 2.2;
int buzzerPin = 3;

void setup()
{
  pinMode(buzzerPin,OUTPUT);
  Serial.begin(9600);
}

void loop()
{
  float potentioMeterReading = analogRead(A0);
  float photoResistorReading = analogRead(A1);
  if(potentioMeterReading != lastPotentioMeterReading){
    if((millis() - lastDebounceTime) > debounceDelay){
      lastPotentioMeterReading = potentioMeterReading;
      thresholdVoltage = inputVoltage *(1 -  (potentioMeterReading/(maxAnalogPotentiometer-minAnalogPotentiometer)));
    }
    lastDebounceTime = millis();
  }
  float photoResistanceVoltage = inputVoltage * (1 - (photoResistorReading/(maxAnalogPhotoresistor - minAnalogPhotoresistor)));
  if (photoResistanceVoltage < thresholdVoltage){
  	digitalWrite(buzzerPin,HIGH);
  } else {
  	digitalWrite(buzzerPin,LOW);  
  }
  Serial.print("thresholdVoltage: ");
  Serial.print(thresholdVoltage);
  Serial.print(" | photoResistorVoltage: ");
  Serial.println(photoResistanceVoltage);
  delay(400);
}