#define hBridgeInput2 2
#define hBridgeInput1 4
#define hBridgeEnableA 3
#define ultraSonicEcho 5
#define ultraSonicTrigger 6
#define shiftRegisterInput 7
#define shiftRegisterOutputRegisterClock 8
#define shiftRegisterShiftClock 9
#define servoMotorSignal 10
#define joystickButton 13
#define ADXL345 0x53
#define displaySegmentA 0
#define displaySegmentB 1
#define displaySegmentC 2
#define displaySegmentD 3
#define displaySegmentE 4
#define displaySegmentF 5
#define displaySegmentG 6
#define displaySegmentH 7
#define joyStickThumbGripX A2
#define joyStickThumbGripY A3
#define headLightPotentiometer A4
#define headPhotoResistor A5

#include <Arduino.h>
#include <Wire.h>
#include <Servo.h>

// Register: Output 8 -> Output 2
bool isRemoteControlled = false;
char registerOutputSevenSegmentDisplayMap[7] = {displaySegmentC, displaySegmentD, displaySegmentE, displaySegmentG, displaySegmentF, displaySegmentA, displaySegmentB};
int headlightLED = LOW;
int joyStickThumbGripXReading, joyStickThumbGripYReading;
int sevenSegmentDisplayNumberMap[10][7] = {
    // a,b,c,d,e,f,g -> 7 segment
    {1,1,1,1,1,1,0}, //0
    {0,1,1,0,0,0,0}, //1
    {1,1,0,1,1,0,1}, //2
    {1,1,1,1,0,0,1}, //3
    {0,1,1,0,0,1,1}, //4
    {1,0,1,1,0,1,1}, //5
    {1,0,1,1,1,1,1}, //6
    {1,1,1,0,0,0,0}, //7
    {1,1,1,1,1,1,1}, //8
    {1,1,1,1,0,1,1} //9
};
float accelerationX, accelerationY, accelerationZ;
float headLightBrightness;
float motorSpeed;
Servo speedometerServo;
unsigned int joyStickThumbGripDebounceDelay = 500;
unsigned int joyStickThumbGripXDebounceTime = 0;
unsigned int joyStickThumbGripYDebounceTime = 0;

void setup(){
    pinMode(hBridgeEnableA, OUTPUT);
    pinMode(hBridgeInput1, OUTPUT);
    pinMode(hBridgeInput2, OUTPUT);
    pinMode(ultraSonicEcho, INPUT);
    pinMode(ultraSonicTrigger, OUTPUT);
    pinMode(shiftRegisterInput, OUTPUT);
    pinMode(shiftRegisterOutputRegisterClock, OUTPUT);
    pinMode(shiftRegisterShiftClock, OUTPUT);
    pinMode(servoMotorSignal, OUTPUT);
    pinMode(joystickButton, INPUT_PULLUP);
    digitalWrite(hBridgeEnableA, HIGH);
    digitalWrite(hBridgeInput1, HIGH);
    digitalWrite(hBridgeInput2, LOW);
    speedometerServo.attach(servoMotorSignal);
    Serial.begin(9600);
    startupCheck();
}
void loop(){
    motorSpeed = 255;
    setDcMotorSpeed();
    slowDcMotorOnObstacle();

}

void setDcMotorSpeed() {
    analogWrite(hBridgeEnableA, motorSpeed);
}

void readAccelerometer(){
    Wire.beginTransmission(ADXL345);
    Wire.write(0x32);
    Wire.endTransmission(false);
    Wire.requestFrom(ADXL345, 6 ,true);
    accelerationX = (Wire.read() | Wire.read() << 8);
    accelerationX = accelerationX/256;
    accelerationY = (Wire.read() | Wire.read() << 8);
    accelerationY = accelerationY/256;
    accelerationZ = (Wire.read() | Wire.read() << 8);
    accelerationZ = accelerationZ/256;
}

void setNumberToSevenSegmentDisplay(int n){
    digitalWrite(shiftRegisterOutputRegisterClock, LOW);
    for(int i=7; i>=0;i--){
        digitalWrite(shiftRegisterShiftClock, LOW);
        digitalWrite(shiftRegisterInput, sevenSegmentDisplayNumberMap[n][registerOutputSevenSegmentDisplayMap[i]]);
        digitalWrite(shiftRegisterShiftClock, HIGH);
    }
    digitalWrite(shiftRegisterShiftClock, LOW);
    digitalWrite(shiftRegisterInput, headlightLED);
    digitalWrite(shiftRegisterShiftClock, HIGH);
    digitalWrite(shiftRegisterOutputRegisterClock, HIGH);
}


void setServoPosition(int servoPosition){
    speedometerServo.write(servoPosition);
}

float getUltraSonicDistance(){
    digitalWrite(ultraSonicTrigger, LOW);
    delayMicroseconds(2);
    digitalWrite(ultraSonicTrigger, HIGH);
    delayMicroseconds(10);
    digitalWrite(ultraSonicTrigger, LOW);
    float duration = pulseIn(ultraSonicEcho, HIGH);
    const float distance = (duration * 0.0343)/2;
    return distance;
}

void joyStickThumbGripReadings(){
    if((millis() - joyStickThumbGripXDebounceTime) > joyStickThumbGripDebounceDelay){
        int joyStickThumbGripXReading = analogRead(joyStickThumbGripX);
    }
    if((millis() - joyStickThumbGripYDebounceTime) > joyStickThumbGripDebounceDelay){
        int joyStickThumbGripYReading = analogRead(joyStickThumbGripY);
    }    
}

unsigned int joyStickButtonDebounceDelay = 500;
unsigned int joyStickButtonDebounceTime = 0;
bool isJoyStickButtonPressed(){
    if((millis() - joyStickButtonDebounceTime) > joyStickButtonDebounceDelay){
        if(digitalRead(joystickButton) == 0){
            isRemoteControlled = !isRemoteControlled;
        }        
    }
}

void setHeadLightLedBrightness(){
    float headLightResistanceReading = analogRead(headLightPotentiometer);
    if(headLightResistanceReading < 100){
        headLightResistanceReading = analogRead(headPhotoResistor);
        headLightBrightness = (255*(1023.0-headLightResistanceReading)/1024);
    }
    else{
        headLightBrightness = (255*(headLightResistanceReading)/1024);
    }
}

/* template<typename MotorSpeed>
int getScaledMotorSpeedForDisplay(MotorSpeed motorSpeed){
    return static_cast<int>(motorSpeed)/25;
}
 */

int getScaledMotorSpeedForDigitalDisplay(int motorSpeed){
    int scaledSpeed = motorSpeed >2 && motorSpeed <26 ? 1 : static_cast<int>(motorSpeed)/25;
    return scaledSpeed == 10? 9 : scaledSpeed;
}

int getScaledMotorSpeedForAnalogDisplay(int motorSpeed){
    // Might need to return 180- if direction is in reverse
   return 180*(static_cast<int>(motorSpeed)/255.0);
}

const float ultraSoundObstacleDistanceThreshold = 50.0;
const int brakingFactor = 2;
void slowDcMotorOnObstacle(){
    int distanceToObstacle = getUltraSonicDistance();
    if (distanceToObstacle< ultraSoundObstacleDistanceThreshold){
        motorSpeed = ((distanceToObstacle/ultraSoundObstacleDistanceThreshold)*255)/brakingFactor;
        motorSpeed = distanceToObstacle < 15 ? 0 : motorSpeed;
        setDcMotorSpeed();
    }
    Serial.print("Ultrasonic distance: ");
    Serial.println(distanceToObstacle);
    Serial.print(" | Motor Speed: ");
    Serial.println(motorSpeed);
}

void startupCheck(){
    // Servo check
    setServoPosition(180);
    delay(1000);
    setServoPosition(0);

    // 7 Segment display check
    for(int i=0; i<=9; i++){
        setNumberToSevenSegmentDisplay(i);
        delay(250);
    }

    // motor check
    motorSpeed = 20;
    setDcMotorSpeed();
    delay(700);
    motorSpeed = 100;
    setDcMotorSpeed();
    delay(700);
    motorSpeed = 255;
    setDcMotorSpeed();

    // Ultrasound check
    Serial.print("Ultrasonic distance: ");
    Serial.println(getUltraSonicDistance());
}