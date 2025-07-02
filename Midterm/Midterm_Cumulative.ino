#define hBridgeInput2 2
#define hBridgeInput1 4
#define hBridgeEnableA 3
#define ultraSonicEcho 5
#define ultraSonicTrigger 6
#define shiftRegisterInput 7
#define shiftRegisterOutputRegisterClock 8
#define shiftRegisterShiftClock 9
#define shiftRegisterEnable 11
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
#define headLightPotentiometer A0
#define headPhotoResistor A1

#include <Arduino.h>
#include <Wire.h>
#include <Servo.h>

// Register: Output 8 -> Output 2
bool isRemoteControlled = false;
char registerOutputSevenSegmentDisplayMap[7] = {displaySegmentC, displaySegmentD, displaySegmentE, displaySegmentG, displaySegmentF, displaySegmentA, displaySegmentB};
float accelerationX, accelerationY, accelerationZ;
float motorSpeed;
int headlightLED = HIGH;
int headLightBrightness;
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
Servo speedometerServo;
unsigned int headLightLedPotentiometerDebounceTime = 0;
unsigned int headLightLedPotentiometerDelay = 200;
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
    pinMode(shiftRegisterEnable, OUTPUT);
    pinMode(servoMotorSignal, OUTPUT);
    pinMode(joystickButton, INPUT_PULLUP);
    digitalWrite(hBridgeEnableA, HIGH);
    digitalWrite(hBridgeInput1, HIGH);
    digitalWrite(hBridgeInput2, LOW);
    speedometerServo.attach(servoMotorSignal);
    Serial.begin(9600);
    calibrateAccelerometer();
    startupCheck();
}
void loop(){
    setControlModeOnJoyStickButtonPress();
    controlSkateBoard();
    setNumberToSevenSegmentDisplay(getScaledMotorSpeedForDigitalDisplay(motorSpeed));
    setServoPosition(getScaledMotorSpeedForAnalogDisplay(motorSpeed));
    handleFall();
    slowDcMotorOnObstacle();
    setHeadLightLedBrightness();
}

void setDcMotorSpeed() {
    analogWrite(hBridgeEnableA, motorSpeed);
}

void getAccelerations(){
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
    Serial.print("Xa= ");
    Serial.print(accelerationX);
    Serial.print("   Ya= ");
    Serial.print(accelerationY);
    Serial.print("   Za= ");
    Serial.println(accelerationZ);
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

void getJoyStickThumbGripReadings(){
    if((millis() - joyStickThumbGripXDebounceTime) > joyStickThumbGripDebounceDelay){
        joyStickThumbGripXDebounceTime = millis();
        joyStickThumbGripXReading = analogRead(joyStickThumbGripX);
    }
    if((millis() - joyStickThumbGripYDebounceTime) > joyStickThumbGripDebounceDelay){
        joyStickThumbGripYDebounceTime = millis();
        joyStickThumbGripYReading = analogRead(joyStickThumbGripY);
    }
}

unsigned int joyStickButtonDebounceDelay = 500;
unsigned int joyStickButtonDebounceTime = 0;
bool setControlModeOnJoyStickButtonPress(){
    if((millis() - joyStickButtonDebounceTime) > joyStickButtonDebounceDelay){
        joyStickButtonDebounceTime = millis();
        if(digitalRead(joystickButton) == 1){
            isRemoteControlled = !isRemoteControlled;
        }
    }
}

void setHeadLightLedBrightness(){
    float headLightResistanceReading;
    if((millis() - headLightLedPotentiometerDebounceTime) > headLightLedPotentiometerDelay){
        headLightLedPotentiometerDebounceTime = millis();
        headLightResistanceReading = analogRead(headLightPotentiometer);
        if(headLightResistanceReading < 100){
            headLightResistanceReading = analogRead(headPhotoResistor);
            headLightBrightness = (255*(1023.0-headLightResistanceReading)/1024);
            Serial.print("Photoresistor Reading :");
            Serial.println(headLightResistanceReading);
        }
        else{
            headLightBrightness = (255*(headLightResistanceReading)/1024);
            Serial.print("Potentiometer Reading :");
            Serial.println(headLightResistanceReading);
        }
        analogWrite(shiftRegisterEnable,headLightBrightness);
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
    // Serial.print("Ultrasonic distance: ");
    // Serial.println(distanceToObstacle);
    // Serial.print(" | Motor Speed: ");
    // Serial.println(motorSpeed);
}

void onDroneControl(){
    getJoyStickThumbGripReadings();
    /*
    * Might need to change formula depending on orientation of joystick
    */
    motorSpeed = ((1024-joyStickThumbGripXReading)/1024.0)*255;
    setDcMotorSpeed();
    // Serial.print("ThumbGripX: ");
    // Serial.print(joyStickThumbGripXReading);
    // Serial.print("ThumbGripY: ");
    // Serial.print(joyStickThumbGripYReading);

    /*
    * To-Do: steering control 
    */
}

const float accelerationXThreshold = 0.35;
const float accelerationXMaximum = 2.00;
const int minimumSpeedIncrement = 3;
const int maximumSpeedIncrement = 20;
void onAccelerometerControl(){
    if(fabs(accelerationX)>accelerationXThreshold){
        int speedIncrement = minimumSpeedIncrement + ((fabs(accelerationX)-accelerationXThreshold)/(accelerationXMaximum - accelerationXThreshold))*(maximumSpeedIncrement - minimumSpeedIncrement);
        if(accelerationX>0){
            motorSpeed = (motorSpeed + speedIncrement) < 255 ? (motorSpeed + speedIncrement) : 255;
        } 
        else if (accelerationX <0){
            motorSpeed = (motorSpeed - speedIncrement) > 0 ? (motorSpeed - speedIncrement) : 0;
        }
        
    }
}

void controlSkateBoard(){
    switch (isRemoteControlled)
    {
    case 0:
        Serial.println("Accelerometer Control Mode");
        onAccelerometerControl();
        break;

    case 1:
        Serial.println("Remote Control Mode");
        onDroneControl();
        break;
    default:
        Serial.println("No control selected");
        break;
    }
}

const int accelerometerXOffset = -2;
const int accelerometerYOffset = -1;
const int accelerometerZOffset = 1;
void calibrateAccelerometer(){
    Wire.begin();
    Wire.beginTransmission(ADXL345);
    Wire.write(0x2D);
    Wire.write(0);
    Wire.endTransmission();

    //X-axis
    Wire.beginTransmission(ADXL345);
    Wire.write(0x1E);  // X-axis offset register
    Wire.write(-2);
    Wire.endTransmission();
    delay(10);

    //Y-axis
    Wire.beginTransmission(ADXL345);
    Wire.write(0x1F); // Y-axis offset register
    Wire.write(-1);
    Wire.endTransmission();
    delay(10);


    //Z-axis
    Wire.beginTransmission(ADXL345);
    Wire.write(0x20); // Z-axis offset register
    Wire.write(+1);
    Wire.endTransmission();
    delay(10);
}

void handleFall(){
    getAccelerations();
    if(isFallDetected(accelerationX, accelerationY, accelerationZ)){
        digitalWrite(hBridgeInput1, LOW);
        digitalWrite(hBridgeEnableA, LOW);
        headLightBrightness = 255;
        setHeadLightLedBrightness();
    }
}

float getTotalAcceleration(float accelerationX, float accelerationY, float accelerationZ) {
    return sqrt(accelerationX*accelerationX + accelerationY*accelerationY + accelerationZ*accelerationZ);
}

bool isFallDetected(float accelerationX, float accelerationY, float accelerationZ) {
    float accelerationMagnitude = getTotalAcceleration(accelerationX, accelerationY, accelerationZ);

    static unsigned long lastFallTime = 0;
    static bool inFreeFall = false;

    if (!inFreeFall && accelerationMagnitude < 0.5) {
        inFreeFall = true;
        lastFallTime = millis();
    } else if (inFreeFall && accelerationMagnitude > 2.5) {
        inFreeFall = false;
        if (millis() - lastFallTime < 1000) {
            return true;  // Fall detected!
        }
    } else if (inFreeFall && millis() - lastFallTime > 1500) {
        inFreeFall = false;  // Timed out
    }

    return false;
}

void startupCheck(){
    // Servo check
    setServoPosition(180);
    delay(1000);
    setServoPosition(0);

    // 7 Segment display check
    // Headlight LED check
    for(int i=0; i<=9; i++){
        headlightLED = i%2;
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

    // Joystick check
    Serial.print("Button state: ");
    Serial.print(digitalRead(joystickButton));
    Serial.print(" | Stick X Reading: ");
    Serial.print(analogRead(joyStickThumbGripX));
    Serial.print(" | Stick Y Reading: ");
    Serial.print(analogRead(joyStickThumbGripY));
}