#include <Servo.h>

/*
* The photoresitor is configured to have analogRead range of 6 -> 679
*/

Servo myservo;
int servoPin = 3;
int closePosition = 178;
int openPosition = 90;
unsigned int debounceDelay = 500;
unsigned int lastDebounceTime = 0;
int lastPhotoResistorReading = 0;
int threshold =540;
  
void setup()
{
  myservo.attach(servoPin);  
}

void loop()
{
  int photoResistorReading = analogRead(A0);
  if(photoResistorReading != lastPhotoResistorReading){
    if((millis() - lastDebounceTime) > debounceDelay){
    	lastPhotoResistorReading = photoResistorReading;
    }
    lastDebounceTime = millis();
  }
  if(photoResistorReading < threshold){
  	myservo.write(openPosition);
  } else {
  	myservo.write(closePosition);
  }
}