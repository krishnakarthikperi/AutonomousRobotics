#include <Servo.h>

Servo myservo;
int servoPin = 3;
int joystickButton = 2;
int joystickX = A0;
int joystickY = A1;

int lastButtonState = LOW;
unsigned int debounceDelay = 500;
unsigned int joystickXDebounceTime = 0;
unsigned int joystickYDebounceTime = 0;
float lastServoSpeed = 0;
float lastServoPosition = 0;
float servoPosition;
float servoSpeed;

void setup() {
	pinMode(joystickButton, INPUT_PULLUP);
    myservo.attach(servoPin);
    Serial.begin(9600);
  	myservo.write(180);
}

void loop() {
    int joystickXReading = analogRead(joystickX);
    int joystickYReading = analogRead(joystickY);
    int currentServoPosition = ((1024-joystickXReading)/1024.0)*180;
    int currentServoSpeed = ((1024-joystickYReading)/1024.0)*45;
    if((millis() - joystickXDebounceTime) > debounceDelay){
        joystickXDebounceTime = millis();
        if(lastServoPosition != currentServoPosition){
            lastServoPosition = servoPosition;
            servoPosition = currentServoPosition;
        }
    }
    if((millis() - joystickYDebounceTime) > debounceDelay){
        joystickYDebounceTime = millis();
        if(servoSpeed != currentServoSpeed){
            lastServoSpeed = servoSpeed;
            servoSpeed = currentServoSpeed;
        }
    }
	Serial.print(servoPosition);
  	Serial.print("|");
  	Serial.println(servoSpeed);
	delay(700);
  	moveServo();    
}

void moveServo() {
    if(servoSpeed == 0){
        servoSpeed = 1;
    }
    if(servoPosition>lastServoPosition){
        for(int i = lastServoPosition; i<=servoPosition;i = i+servoSpeed){
            myservo.write(i);
        }
        myservo.write(servoPosition);
    }
    if(servoPosition<lastServoPosition){
        for(int i = lastServoPosition; i>=servoPosition;i = i-servoSpeed){
            // if(abs(lastServoPosition-servoPosition)<abs(lastServoPosition-servoSpeed)){
            //     myservo.write(servoPosition);
            //     i = servoPosition+1;
            //     break;
            // }
            myservo.write(i);
        }
        myservo.write(servoPosition);
    }

}