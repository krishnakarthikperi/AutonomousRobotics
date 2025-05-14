#include <Servo.h>

/*
* The photoresitor is configured to have analogRead range of 6 -> 679
* The potentiometer is configured to have the analogRead value from 0 -> 1023
*/

Servo myservo;
int servoPin = 3;
int pushButtonPin = 2;
int closePosition = 178;
int openPosition = 90;
int lastPhotoResistorReading = 0;
int threshold =540;
float maxAnalogPhotoresistor = 679;
float minAnalogPhotoresistor = 6;
float maxAnalogPotentiometer = 1023;
float minAnalogPotentiometer = 0;
float lastPotentioMeterReading = 0;
int lastButtonState = LOW;
unsigned int debounceDelay = 500;
unsigned int debounceTime = 0;
float thresholdVoltage = 2.2;
float inputVoltage = 5.0;
int buttonStatus = LOW;

void setup()
{
  Serial.begin(9600);
  myservo.attach(servoPin);  
  pinMode(pushButtonPin, INPUT_PULLUP);
}

void loop()
{
  int photoResistorReading = analogRead(A0);
  int potentioMeterReading = analogRead(A1);
  int buttonState = digitalRead(pushButtonPin);
  float photoResistanceVoltage;
  if((millis() - debounceTime) > debounceDelay){
    if(photoResistorReading != lastPhotoResistorReading){
    	lastPhotoResistorReading = photoResistorReading;
        debounceTime = millis();
    }
    if(potentioMeterReading != lastPotentioMeterReading){
        lastPotentioMeterReading = potentioMeterReading;
        thresholdVoltage = inputVoltage *(1 -  (potentioMeterReading/(maxAnalogPotentiometer-minAnalogPotentiometer)));
        debounceTime = millis();
    }
    if(buttonState != lastButtonState){
        lastButtonState = buttonState;
        debounceTime = millis();
      if(buttonState){
      	buttonStatus = !buttonStatus;
      }
    }

    switch (buttonStatus)
    {
    case LOW:
        if(photoResistorReading < threshold){
            myservo.write(openPosition);
        } else {
            myservo.write(closePosition);
        }
        break;
    
    case HIGH:
        photoResistanceVoltage = inputVoltage * (1 - (photoResistorReading/(maxAnalogPhotoresistor - minAnalogPhotoresistor)));
        Serial.print(thresholdVoltage);
        Serial.print("|");
        Serial.println(photoResistanceVoltage);
        if(photoResistanceVoltage > thresholdVoltage){
            myservo.write(openPosition);
        } else {
            myservo.write(closePosition);
        }
        break;
            
    default:
        break;
    }
  }
  
}