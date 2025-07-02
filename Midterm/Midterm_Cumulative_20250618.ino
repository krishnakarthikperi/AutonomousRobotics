// #define hBridgeEnableA 3
#define hBridgeInput1 12
#define hBridgeInput2 4
#define hBridgeInput3 2
#define hBridgeInput4 1
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
#define dcMotorOutput 3

// #include <Arduino.h>
#include <Servo.h>
#include <Stepper.h>
#include <Wire.h>

// Register: Output 8 -> Output 2
bool isRemoteControlled = false;
bool isMasterLocked = false;
char registerOutputSevenSegmentDisplayMap[7] = {displaySegmentC, displaySegmentD, displaySegmentE, displaySegmentG, displaySegmentF, displaySegmentA, displaySegmentB};
const int accelerometerXOffset = -2;
const int accelerometerYOffset = -1;
const int accelerometerZOffset = 1;
const int brakingFactor = 2;
const int emergencyBrakeThreshold = 35;
const int maximumSpeedIncrement = 20;
const int minimumSpeedIncrement = 3;
const int stepperMotorStepsPerRevolution = 2048;
const int stepperMotorSpeed = 10;
const float accelerationXMaximum = 2.00;
const float accelerationXThreshold = 0.35;
const float joyStickThumbGripXThreshold = 30;
const float ultraSoundObstacleDistanceThreshold = 50.0;
float accelerationX, accelerationY, accelerationZ;
float motorSpeed;
int headlightLED = HIGH;
int headLightBrightness;
int lastStepperMotorPosition;
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
Stepper stepperMotor = Stepper(stepperMotorStepsPerRevolution, hBridgeInput1, hBridgeInput3, hBridgeInput2, hBridgeInput4);
unsigned int headLightLedPotentiometerDebounceTime = 0;
unsigned int headLightLedPotentiometerDelay = 200;
unsigned int joyStickButtonDebounceDelay = 500;
unsigned int joyStickButtonDebounceTime = 0;
unsigned int joyStickThumbGripDebounceDelay = 500;
unsigned int joyStickThumbGripXDebounceTime = 0;
unsigned int joyStickThumbGripYDebounceTime = 0;

void setup(){
    pinMode(dcMotorOutput, OUTPUT);
    pinMode(ultraSonicEcho, INPUT);
    pinMode(ultraSonicTrigger, OUTPUT);
    pinMode(shiftRegisterInput, OUTPUT);
    pinMode(shiftRegisterOutputRegisterClock, OUTPUT);
    pinMode(shiftRegisterShiftClock, OUTPUT);
    pinMode(shiftRegisterEnable, OUTPUT);
    pinMode(servoMotorSignal, OUTPUT);
    pinMode(joystickButton, INPUT_PULLUP);
    digitalWrite(dcMotorOutput, LOW);
    speedometerServo.attach(servoMotorSignal);
    Serial.begin(9600);
    calibrateAccelerometer();
    startupCheck();
}
void loop(){
    if(true){
        setControlModeOnJoyStickButtonPress();
        controlSkateBoard();
        setNumberToSevenSegmentDisplay(getScaledMotorSpeedForDigitalDisplay(motorSpeed));
        setServoPosition(getScaledMotorSpeedForAnalogDisplay(motorSpeed));
        handleFall();
        slowDcMotorOnObstacle();
        setHeadLightLedBrightness();
        debugLog();
    }
    else {
        testHandleFall();
        testHeadlight();
        testJoyStick();
        testSpeedDisplay();
        testUltraSound();
    }
}

void setDcMotorSpeed() {
    if(isMasterLocked){
        motorSpeed = 0;        
    } else {
        motorSpeed = constrain(motorSpeed, 0, 255);
    }
    analogWrite(dcMotorOutput, motorSpeed);
    
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
            // headLightBrightness = (255*(1023.0-headLightResistanceReading)/1024);
            headLightBrightness = map(1023-headLightResistanceReading,0,1023,0,255);
            headLightBrightness = constrain(headLightBrightness, 0,255);
            Serial.print(" |PhoRes :");
            Serial.print(headLightResistanceReading);
        }
        else{
            // headLightBrightness = (255*(headLightResistanceReading)/1024);
            headLightBrightness = map(headLightResistanceReading,0,1023,0,255);
            headLightBrightness = constrain(headLightBrightness, 0,255);
            Serial.print(" |PotRes");
            Serial.print(headLightResistanceReading);
        }
        analogWrite(shiftRegisterEnable,headLightBrightness);
    }
}

int getScaledMotorSpeedForDigitalDisplay(int motorSpeed){
    int scaledSpeed = motorSpeed >2 && motorSpeed <26 ? 1 : static_cast<int>(motorSpeed)/25;
    return scaledSpeed == 10? 9 : scaledSpeed;
}

int getScaledMotorSpeedForAnalogDisplay(int motorSpeed){
    // Might need to return 180- if direction is in reverse
//    return 180*(static_cast<int>(motorSpeed)/255.0);
   return map(static_cast<int>(motorSpeed), 0, 255, 0, 180);
}

void slowDcMotorOnObstacle(){
    int distanceToObstacle = getUltraSonicDistance();
    if (distanceToObstacle< ultraSoundObstacleDistanceThreshold){
        motorSpeed = ((distanceToObstacle/ultraSoundObstacleDistanceThreshold)*255)/brakingFactor;
        motorSpeed = distanceToObstacle < emergencyBrakeThreshold ? 0 : motorSpeed;
        setDcMotorSpeed();
    }
}

void onDroneControl(){
    getJoyStickThumbGripReadings();
    // motorSpeed = ((1024-joyStickThumbGripXReading)/1024.0)*255;
    long scaledJoyStickThumbGripXReading = map(joyStickThumbGripXReading,0,1023,-512,512);
    if(fabs(scaledJoyStickThumbGripXReading) > joyStickThumbGripXThreshold){
        int speedIncrement = map(fabs(scaledJoyStickThumbGripXReading), 0, 512, minimumSpeedIncrement, maximumSpeedIncrement);
        speedIncrement = constrain(speedIncrement, minimumSpeedIncrement, maximumSpeedIncrement);
        if(scaledJoyStickThumbGripXReading<0){
            motorSpeed = motorSpeed - speedIncrement;
        } else if (scaledJoyStickThumbGripXReading > 0)
        {
            motorSpeed = motorSpeed + speedIncrement;
        }        
        setDcMotorSpeed();
    }
    
    long targetPosition = map(joyStickThumbGripYReading,0,1023,-512,512);
    targetPosition = constrain(targetPosition, -512, 512);
    int stepsToMove = targetPosition-lastStepperMotorPosition;
    stepperMotor.step(stepsToMove);
    lastStepperMotorPosition = targetPosition;
 
}

void onAccelerometerControl(){
    getAccelerations();
    if(fabs(accelerationX)>accelerationXThreshold){
        int speedIncrement = minimumSpeedIncrement + ((fabs(accelerationX)-accelerationXThreshold)/(accelerationXMaximum - accelerationXThreshold))*(maximumSpeedIncrement - minimumSpeedIncrement);
        if(accelerationX>0){
            motorSpeed = (motorSpeed + speedIncrement) < 255 ? (motorSpeed + speedIncrement) : 255;
        } 
        else if (accelerationX <0){
            motorSpeed = (motorSpeed - speedIncrement) > 0 ? (motorSpeed - speedIncrement) : 0;
        }
        setDcMotorSpeed();    
    }
}

void controlSkateBoard(){
    switch (isRemoteControlled)
    {
    case 0:
        // Serial.println("Accelerometer Control Mode");
        onAccelerometerControl();
        break;

    case 1:
        // Serial.println("Remote Control Mode");
        onDroneControl();
        break;
    default:
        // Serial.println("No control selected");
        break;
    }
}

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
        isMasterLocked = true;
        motorSpeed = 0;
        digitalWrite(dcMotorOutput, LOW);
        headLightBrightness = 255;
        setHeadLightLedBrightness();
    }
}

float getTotalAcceleration(float accelerationX, float accelerationY, float accelerationZ) {
    return sqrt(accelerationX*accelerationX + accelerationY*accelerationY + accelerationZ*accelerationZ);
}

static unsigned long lastFallTime = 0;
static bool inFreeFall = false;
const float gForceDeadzoneMinimum = 0.3;
const float gForceDeadzoneMaximum = 1.5;
bool isFallDetected(float accelerationX, float accelerationY, float accelerationZ) {
    float accelerationMagnitude = getTotalAcceleration(accelerationX, accelerationY, accelerationZ);

    // return (accelerationMagnitude < gForceDeadzoneMinimum) || (accelerationMagnitude > gForceDeadzoneMaximum);
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

void testUltraSound(){
    motorSpeed = 255;
    setDcMotorSpeed();
    slowDcMotorOnObstacle();
}

void testSpeedDisplay(){
    for(int i=0; i<255; i++){
        motorSpeed = i;
        setDcMotorSpeed();
        setNumberToSevenSegmentDisplay(getScaledMotorSpeedForDigitalDisplay(i));
        setServoPosition(getScaledMotorSpeedForAnalogDisplay(i));
        Serial.print(getScaledMotorSpeedForDigitalDisplay(i));
        Serial.print(" ");
        Serial.println(getScaledMotorSpeedForAnalogDisplay(i));
        delay(700);
    }
}

void testJoyStick(){
    setControlModeOnJoyStickButtonPress();
    Serial.print("Control mode: ");
    Serial.println(isRemoteControlled);
    controlSkateBoard();
    delay(200);
}

void testHeadlight(){

    setHeadLightLedBrightness();
}

void testHandleFall(){
    handleFall();
}

void debugLog(){
    Serial.print(" |Speed ");
    Serial.print(motorSpeed);
    if(isRemoteControlled){
        Serial.print(" joyX ");
        Serial.print(joyStickThumbGripXReading);
        Serial.print(" joyY: ");
        Serial.print(joyStickThumbGripYReading);
    }
    else{
        Serial.print(" |aX ");
        Serial.print(accelerationX);
        Serial.print(" |aY ");
        Serial.print(accelerationY);
        Serial.print(" |aZ ");
        Serial.print(accelerationZ);
    }
    Serial.print(" |stepPos");
    Serial.print(lastStepperMotorPosition);
    Serial.print(" |dist ");
    Serial.print(getUltraSonicDistance());
    Serial.print(" |fall ");
    Serial.println(isMasterLocked);
}