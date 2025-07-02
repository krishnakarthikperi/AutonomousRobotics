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

#include <Servo.h>
#include <Stepper.h>
#include <Wire.h>

bool isRemoteControlled = false;
bool isMasterLocked = false;
bool lastJoyStickButtonState = HIGH;
char registerOutputSevenSegmentDisplayMap[7] = {displaySegmentC, displaySegmentD, displaySegmentE, displaySegmentG, displaySegmentF, displaySegmentA, displaySegmentB}; // Register: Output 8 -> Output 2
const int accelerometerXOffset = -2;
const int accelerometerYOffset = -1;
const int accelerometerZOffset = 1;
const int brakingFactor = 1;
const int maximumMotorSpeed = 255; 
const int maximumSpeedIncrement = 8;
const int minimumMotorSpeed = 155; 
const int minimumSpeedIncrement = 1;
const int stepperMotorStepsPerRevolution = 2048;
const int stepperMotorSpeed = 10;
const float accelerationXMaximum = 2.00;
const float accelerationXThreshold = 0.35;
const float emergencyBrakeThreshold = 15.0;
const float gForceDeadzoneMinimum = 0.3;
const float gForceDeadzoneMaximum = 1.5;
const float joyStickThumbGripXThreshold = 30;
const float ultraSoundObstacleDistanceThreshold = 35.0;
float accelerationX, accelerationY, accelerationZ;
float motorSpeed, motorSpeedPercentage;
int headlightLED = HIGH;
int headLightBrightness;
int lastStepperMotorPosition = 0;
int joyStickThumbGripXReading, joyStickThumbGripYReading;
int preObstacleSpeedPercentage; 
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
// Stepper stepperMotor = Stepper(stepperMotorStepsPerRevolution, hBridgeInput1, hBridgeInput4, hBridgeInput3, hBridgeInput2); // Working on initial analysis
unsigned int headLightLedPotentiometerDebounceTime = 0;
unsigned int headLightLedPotentiometerDelay = 200;
unsigned int joyStickButtonDebounceDelay = 200;
unsigned int joyStickButtonDebounceTime = 0;
unsigned int joyStickThumbGripDebounceDelay = 200;
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
    digitalWrite(dcMotorOutput, HIGH);
    speedometerServo.attach(servoMotorSignal);
    stepperMotor.setSpeed(10);
    // stepperMotor.setSpeed(10);
    Serial.begin(9600);
    calibrateAccelerometer();
    startupCheck();
    analogWrite(dcMotorOutput, 155);
    motorSpeedPercentage = 25;
}

void loop(){
    if(true){
        setControlModeOnJoyStickButtonPress();
        controlSkateBoard();
        setNumberToSevenSegmentDisplay(getScaledMotorSpeedForDigitalDisplay(motorSpeedPercentage));
        setServoPosition(getScaledMotorSpeedForAnalogDisplay(motorSpeedPercentage));
        handleFall();
        while(isMasterLocked){
            digitalWrite(shiftRegisterEnable,HIGH);
            delay(1000);
            digitalWrite(shiftRegisterEnable,LOW);
            delay(1000);
        };
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
        testStepperMotor();
    }
}

void calibrateAccelerometer(){
    Wire.begin();
    Wire.beginTransmission(ADXL345);
    Wire.write(0x2D);
    Wire.write(8);
    Wire.endTransmission();

    //X-axis
    Wire.beginTransmission(ADXL345);
    Wire.write(0x1E);  // X-axis offset register
    Wire.write(accelerometerXOffset);
    Wire.endTransmission();
    delay(10);

    //Y-axis
    Wire.beginTransmission(ADXL345);
    Wire.write(0x1F); // Y-axis offset register
    Wire.write(accelerometerYOffset);
    Wire.endTransmission();
    delay(10);


    //Z-axis
    Wire.beginTransmission(ADXL345);
    Wire.write(0x20); // Z-axis offset register
    Wire.write(accelerometerZOffset);
    Wire.endTransmission();
    delay(10);
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

void debugLog(){
    Serial.print(" |Speed ");
    Serial.print(motorSpeed);
    Serial.print("(");
    Serial.print(motorSpeedPercentage);
    Serial.print(")");
    if(isRemoteControlled){
        Serial.print(" |joyX ");
        Serial.print(joyStickThumbGripXReading);
        Serial.print(" |joyY: ");
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
    Serial.print(isMasterLocked);
    Serial.print(" |totalG ");
    Serial.println(getTotalAcceleration(accelerationX, accelerationY, accelerationZ));
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

int getScaledMotorSpeedForDigitalDisplay(int motorSpeedPercentage){
    int scaledSpeed = motorSpeedPercentage >2 && motorSpeedPercentage <19 ? 1 : static_cast<int>(motorSpeedPercentage)/10;
    return scaledSpeed == 10? 9 : scaledSpeed;
}

int getScaledMotorSpeedForAnalogDisplay(int motorSpeedPercentage){
    // Might need to return 180- if direction is in reverse
//    return 180*(static_cast<int>(motorSpeed)/255.0);
   return map(static_cast<int>(motorSpeedPercentage), 0, 100, 0, 180);
}

float getTotalAcceleration(float accelerationX, float accelerationY, float accelerationZ) {
    return sqrt(accelerationX*accelerationX + accelerationY*accelerationY + accelerationZ*accelerationZ);
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

bool isFallDetected(float accelerationX, float accelerationY, float accelerationZ) {
    float accelerationMagnitude = getTotalAcceleration(accelerationX, accelerationY, accelerationZ);
    return (accelerationMagnitude < gForceDeadzoneMinimum) || (accelerationMagnitude > gForceDeadzoneMaximum);
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

void onAccelerometerControl(){
    getAccelerations();
    if(fabs(accelerationX)>accelerationXThreshold){
        int speedIncrement = minimumSpeedIncrement + ((fabs(accelerationX)-accelerationXThreshold)/(accelerationXMaximum - accelerationXThreshold))*(maximumSpeedIncrement - minimumSpeedIncrement);
        if(accelerationX>0){
            motorSpeedPercentage = motorSpeedPercentage + speedIncrement;
        } 
        else if (accelerationX <0){
            motorSpeedPercentage = motorSpeedPercentage - speedIncrement;
        }
        motorSpeedPercentage = constrain(motorSpeedPercentage, 0 ,100);
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
            motorSpeedPercentage = motorSpeedPercentage - speedIncrement;
        } else if (scaledJoyStickThumbGripXReading > 0)
        {
            motorSpeedPercentage = motorSpeedPercentage + speedIncrement;
        }        
        motorSpeedPercentage = constrain(motorSpeedPercentage, 0 ,100);
        setDcMotorSpeed();
    }
    
    if((millis() - joyStickThumbGripYDebounceTime) > joyStickThumbGripDebounceDelay){
        long scaledY = map(joyStickThumbGripYReading, 0, 1023, -512, 512);
        long targetPosition = constrain(scaledY, -180, 180);
        long stepsToMove = targetPosition - lastStepperMotorPosition;
        Serial.print(targetPosition);
        stepperMotor.setSpeed(10);  // You can make speed dynamic if needed
        stepperMotor.step(stepsToMove);
        lastStepperMotorPosition = targetPosition;
        joyStickThumbGripYDebounceTime = millis();
    }
 
}

bool setControlModeOnJoyStickButtonPress(){
    bool currentJoyStickButtonState = digitalRead(joystickButton);
    if(currentJoyStickButtonState != lastJoyStickButtonState && currentJoyStickButtonState == 0){
    if((millis() - joyStickButtonDebounceTime) > joyStickButtonDebounceDelay){
            isRemoteControlled = !isRemoteControlled;
            joyStickButtonDebounceTime = millis();
        }
    }
    lastJoyStickButtonState = currentJoyStickButtonState;
}

void setDcMotorSpeed() {
    if(isMasterLocked){
        motorSpeed = 0;        
    } else {
        motorSpeed = map(motorSpeedPercentage, 0, 100, minimumMotorSpeed,maximumMotorSpeed);
        motorSpeed = constrain(motorSpeed, 155, 255);
    }
    motorSpeedPercentage > 5 ? analogWrite(dcMotorOutput, motorSpeed) : analogWrite(dcMotorOutput, 0);    
}

void setHeadLightLedBrightness(){
    int headLightResistanceReading;
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

void slowDcMotorOnObstacle(){
    int distanceToObstacle = getUltraSonicDistance();
    if (distanceToObstacle< ultraSoundObstacleDistanceThreshold){
        motorSpeedPercentage = map(distanceToObstacle, emergencyBrakeThreshold, ultraSoundObstacleDistanceThreshold, 0, 100)/brakingFactor;
        // motorSpeed = ((distanceToObstacle/ultraSoundObstacleDistanceThreshold)*255)/brakingFactor;
        // motorSpeedPercentage = distanceToObstacle < emergencyBrakeThreshold ? 0 : motorSpeedPercentage;
        motorSpeedPercentage = constrain(motorSpeedPercentage, 0, preObstacleSpeedPercentage/brakingFactor);
        setDcMotorSpeed();
    } else
    {
        preObstacleSpeedPercentage = motorSpeedPercentage;
    }
    
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
    motorSpeedPercentage = 20;
    setDcMotorSpeed();
    delay(700);
    motorSpeedPercentage = 50;
    setDcMotorSpeed();
    delay(700);
    motorSpeedPercentage = 100;
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

    // Stepper checck
    testStepperMotor();
}

void testHandleFall(){
    handleFall();
}

void testHeadlight(){
    setHeadLightLedBrightness();
}

void testJoyStick(){
    setControlModeOnJoyStickButtonPress();
    Serial.print("Control mode: ");
    Serial.println(isRemoteControlled);
    controlSkateBoard();
    delay(200);
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

void testUltraSound(){
    motorSpeed = 255;
    setDcMotorSpeed();
    slowDcMotorOnObstacle();
}

void testStepperMotor(){
    getJoyStickThumbGripReadings();
    if((millis() - joyStickThumbGripYDebounceTime) > joyStickThumbGripDebounceDelay){
        long scaledY = map(joyStickThumbGripYReading, 0, 1023, -512, 512);
        long targetPosition = constrain(scaledY, -180, 180);
        long stepsToMove = targetPosition - lastStepperMotorPosition;
        Serial.print(targetPosition);
        stepperMotor.setSpeed(10);  // You can make speed dynamic if needed
        stepperMotor.step(stepsToMove);
        lastStepperMotorPosition = targetPosition;
        joyStickThumbGripYDebounceTime = millis();
    }
    Serial.println(" |");
}
