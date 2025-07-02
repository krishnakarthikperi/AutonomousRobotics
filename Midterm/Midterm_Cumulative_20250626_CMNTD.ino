/*
 * ============================================================================
 * File: Midterm_Cumulative_20250626_CMNTD.ino
 * Author: Krishna Karthik, Peri
 * Date Modified: 26.06.2025
 * Purpose: Autonomous Robotics System, Midterm Exam
 * Description:
 *   Arduino sketch for a smart skateboard controller featuring:
 *     - Dual control modes: accelerometer-based and joystick-based
 *     - DC motor speed control with safety lock on fall detection
 *     - Stepper motor steering control
 *     - 7-segment display via shift register for speed feedback
 *     - Servo-based analog speedometer
 *     - Automatic and manual headlight LED brightness control
 *     - Obstacle detection and automatic braking using ultrasonic sensor
 *     - Comprehensive startup and test routines
 *
 * Pin Selection Rationale:
 *   - PWM-capable pins (3, 10, 11) are used for DC motor (analogWrite), servo (Servo library), and headlight LED (PWM via shift register OE).
 *   - Stepper motor pins (1, 2, 4, 12) are chosen to match the excitation coil order required by the Stepper.h library for correct stepping sequence.
 *   - Analog pins (A0-A3) are used for sensors and joystick axes for high-resolution analog input.
 *   - Digital pins 7-9, 11 are used for 74HC595 shift register control, as these pins are free and support fast digitalWrite operations.
 *   - Pin 13 is used for the joystick button, leveraging the onboard pull-up resistor.
 *   - I2C communication with the ADXL345 accelerometer uses dedicated SDA/SCL pins (handled by Wire.h).
 *
 * Stepper Motor Initialization:
 *   The Stepper object is initialized as:
 *     Stepper(stepsPerRevolution, IN1, IN2, IN3, IN4)
 *   The order of hBridgeInput1, hBridgeInput2, hBridgeInput3, hBridgeInput4 matches the coil excitation order required by the Stepper.h library for correct rotation.
 *
 * Required Libraries:
 *   - Servo.h: For controlling the analog speedometer servo
 *   - Stepper.h: For stepper motor control (steering)
 *   - Wire.h: For I2C communication with the ADXL345 accelerometer
 *
 * Additional Notes:
 *   - The code is modular, with each function responsible for a specific subsystem.
 *   - All hardware initialization and safety checks are performed in setup().
 *   - The main loop handles real-time control, display, and safety logic.
 *   - All pin assignments and hardware dependencies are documented for easy hardware mapping and troubleshooting.
 * ============================================================================
 */

// -------------------- Pin Configuration --------------------
// H-Bridge motor driver pins for stepper motor control
#define hBridgeInput1 12    // IN1 for stepper motor
#define hBridgeInput2 4     // IN2 for stepper motor
#define hBridgeInput3 2     // IN3 for stepper motor
#define hBridgeInput4 1     // IN4 for stepper motor

// Ultrasonic sensor pins
#define ultraSonicEcho 5    // Echo pin for ultrasonic distance sensor
#define ultraSonicTrigger 6 // Trigger pin for ultrasonic distance sensor

// 74HC595 Shift Register pins for 7-segment display and headlight LED
#define shiftRegisterInput 7                    // Serial data input (DS)
#define shiftRegisterOutputRegisterClock 8      // Latch clock (ST_CP)
#define shiftRegisterShiftClock 9               // Shift clock (SH_CP)
#define shiftRegisterEnable 11                  // Output enable (OE), also used for headlight LED PWM

// Servo motor pin for speedometer
#define servoMotorSignal 10 // Servo signal pin

// Joystick button pin
#define joystickButton 13   // Joystick push button (active LOW)

// I2C address for ADXL345 accelerometer
#define ADXL345 0x53

// 7-segment display segment mapping (to shift register outputs)
#define displaySegmentA 0
#define displaySegmentB 1
#define displaySegmentC 2
#define displaySegmentD 3
#define displaySegmentE 4
#define displaySegmentF 5
#define displaySegmentG 6
#define displaySegmentH 7 // Decimal point (not used)

// Analog pins for joystick and sensors
#define joyStickThumbGripX A2         // Joystick X-axis
#define joyStickThumbGripY A3         // Joystick Y-axis
#define headLightPotentiometer A0     // Potentiometer for headlight brightness
#define headPhotoResistor A1          // Photoresistor for ambient light

// DC motor PWM output pin
#define dcMotorOutput 3

#include <Servo.h>
#include <Stepper.h>
#include <Wire.h>

// -------------------- Global Variables and Constants --------------------

// Control mode flags
bool isRemoteControlled = false;      // false: accelerometer, true: joystick
bool isMasterLocked = false;          // true: system locked due to fall
bool lastJoyStickButtonState = HIGH;  // For button debounce

// 7-segment display segment order mapping for shift register
char registerOutputSevenSegmentDisplayMap[7] = {displaySegmentC, displaySegmentD, displaySegmentE, displaySegmentG, displaySegmentF, displaySegmentA, displaySegmentB}; // Register: Output 8 -> Output 2

// Accelerometer calibration offsets
const int accelerometerXOffset = -2;
const int accelerometerYOffset = -1;
const int accelerometerZOffset = 1;

// Motor speed and control constants
const int brakingFactor = 1;
const int maximumMotorSpeed = 255; 
const int maximumSpeedIncrement = 8;
const int minimumMotorSpeed = 155; 
const int minimumSpeedIncrement = 1;

// Stepper motor configuration
const int stepperMotorStepsPerRevolution = 2048;
const int stepperMotorSpeed = 10;

// Accelerometer thresholds
const float accelerationXMaximum = 2.00;
const float accelerationXThreshold = 0.35;
const float emergencyBrakeThreshold = 15.0;
const float gForceDeadzoneMinimum = 0.3;
const float gForceDeadzoneMaximum = 1.5;

// Joystick thresholds
const float joyStickThumbGripXThreshold = 30;

// Ultrasonic sensor threshold
const float ultraSoundObstacleDistanceThreshold = 35.0;

// Variables for sensor readings and control
float accelerationX, accelerationY, accelerationZ;
float motorSpeed, motorSpeedPercentage;
int headlightLED = HIGH;
int headLightBrightness;
int lastStepperMotorPosition = 0;
int joyStickThumbGripXReading, joyStickThumbGripYReading;
int preObstacleSpeedPercentage; 

// 7-segment display digit to segment mapping
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

// Servo and stepper motor objects
Servo speedometerServo;
Stepper stepperMotor = Stepper(stepperMotorStepsPerRevolution, hBridgeInput1, hBridgeInput2, hBridgeInput3, hBridgeInput4);

// Debounce timing variables
unsigned int headLightLedPotentiometerDebounceTime = 0;
unsigned int headLightLedPotentiometerDelay = 200;
unsigned int joyStickButtonDebounceDelay = 200;
unsigned int joyStickButtonDebounceTime = 0;
unsigned int joyStickThumbGripDebounceDelay = 200;
unsigned int joyStickThumbGripXDebounceTime = 0;
unsigned int joyStickThumbGripYDebounceTime = 0;

// -------------------- Arduino Setup --------------------
/*
 * Initializes all hardware, calibrates sensors, and performs startup checks.
 */
void setup(){
    // Pin modes
    pinMode(dcMotorOutput, OUTPUT);
    pinMode(ultraSonicEcho, INPUT);
    pinMode(ultraSonicTrigger, OUTPUT);
    pinMode(shiftRegisterInput, OUTPUT);
    pinMode(shiftRegisterOutputRegisterClock, OUTPUT);
    pinMode(shiftRegisterShiftClock, OUTPUT);
    pinMode(shiftRegisterEnable, OUTPUT);
    pinMode(servoMotorSignal, OUTPUT);
    pinMode(joystickButton, INPUT_PULLUP);

    digitalWrite(dcMotorOutput, HIGH); // Ensure motor is off at start

    speedometerServo.attach(servoMotorSignal); // Attach servo
    stepperMotor.setSpeed(10);                 // Set stepper speed

    Serial.begin(9600);                        // Serial for debugging

    calibrateAccelerometer();                  // Calibrate ADXL345
    startupCheck();                            // Run hardware checks

    motorSpeedPercentage = 25;                 // Set initial speed
    setDcMotorSpeed();
}

// -------------------- Arduino Main Loop --------------------
/*
 * Main control loop: handles mode switching, control, display, safety, and diagnostics.
 */
void loop(){
    if(true){
        setControlModeOnJoyStickButtonPress(); // Switch between control modes
        controlSkateBoard();                   // Main control logic
        setNumberToSevenSegmentDisplay(getScaledMotorSpeedForDigitalDisplay(motorSpeedPercentage)); // Update 7-segment
        setServoPosition(getScaledMotorSpeedForAnalogDisplay(motorSpeedPercentage));                // Update servo
        handleFall();                          // Detect and handle falls
        while(isMasterLocked){
            // Blink display to indicate lock
            digitalWrite(shiftRegisterEnable,HIGH);
            delay(1000);
            digitalWrite(shiftRegisterEnable,LOW);
            delay(1000);
        };
        slowDcMotorOnObstacle();               // Obstacle avoidance
        setHeadLightLedBrightness();           // Headlight control
        debugLog();                            // Print debug info
    }
    else {
        // Test routines (not used in normal operation)
        testHandleFall();
        testHeadlight();
        testJoyStick();
        testSpeedDisplay();
        testUltraSound();
        testStepperMotor();
    }
}

// -------------------- Sensor Calibration --------------------
/*
 * Calibrates the ADXL345 accelerometer by setting offsets for each axis.
 */
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

// -------------------- Main Control Logic --------------------
/*
 * Selects control mode: accelerometer or joystick.
 */
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

// -------------------- Debug Logging --------------------
/*
 * Prints current system state to Serial for debugging.
 */
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

// -------------------- Sensor Reading Functions --------------------
/*
 * Reads acceleration values from ADXL345.
 */
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

/*
 * Reads joystick X and Y analog values with debounce.
 */
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

// -------------------- Display Scaling Functions --------------------
/*
 * Scales motor speed percentage for 7-segment digital display (0-9).
 */
int getScaledMotorSpeedForDigitalDisplay(int motorSpeedPercentage){
    int scaledSpeed = motorSpeedPercentage >2 && motorSpeedPercentage <19 ? 1 : static_cast<int>(motorSpeedPercentage)/10;
    return scaledSpeed == 10? 9 : scaledSpeed;
}

/*
 * Scales motor speed percentage for analog servo display (0-180 degrees).
 */
int getScaledMotorSpeedForAnalogDisplay(int motorSpeedPercentage){
    // Might need to return 180- if direction is in reverse
//    return 180*(static_cast<int>(motorSpeed)/255.0);
   return map(static_cast<int>(motorSpeedPercentage), 0, 100, 0, 180);
}

// -------------------- Utility Functions --------------------
/*
 * Calculates total acceleration magnitude (g-force).
 */
float getTotalAcceleration(float accelerationX, float accelerationY, float accelerationZ) {
    return sqrt(accelerationX*accelerationX + accelerationY*accelerationY + accelerationZ*accelerationZ);
}

/*
 * Measures distance to obstacle using ultrasonic sensor (in cm).
 */
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

/*
 * Detects a fall based on acceleration magnitude.
 */
bool isFallDetected(float accelerationX, float accelerationY, float accelerationZ) {
    float accelerationMagnitude = getTotalAcceleration(accelerationX, accelerationY, accelerationZ);
    return (accelerationMagnitude < gForceDeadzoneMinimum) || (accelerationMagnitude > gForceDeadzoneMaximum);
}

// -------------------- Safety and Fall Handling --------------------
/*
 * Locks system and stops motor if a fall is detected.
 */
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

// -------------------- Control Modes --------------------
/*
 * Accelerometer-based speed control.
 */
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

/*
 * Joystick-based speed and steering control.
 */
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
    
    int targetPosition = map(joyStickThumbGripYReading, 0, 1023, -180, 180);
    targetPosition = constrain(targetPosition, -180, 180);
    int stepsToMove = targetPosition - lastStepperMotorPosition;
    stepperMotor.setSpeed(10);
    stepperMotor.step(stepsToMove);
    lastStepperMotorPosition = targetPosition;
 
}

// -------------------- Mode Switching --------------------
/*
 * Toggles control mode on joystick button press (with debounce).
 */
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

// -------------------- Motor and Output Control --------------------
/*
 * Sets DC motor speed based on current speed percentage and lock state.
 */
void setDcMotorSpeed() {
    if(isMasterLocked){
        motorSpeed = 0;        
    } else {
        motorSpeed = map(motorSpeedPercentage, 0, 100, minimumMotorSpeed,maximumMotorSpeed);
        motorSpeed = constrain(motorSpeed, 155, 255);
    }
    motorSpeedPercentage > 5 ? analogWrite(dcMotorOutput, motorSpeed) : analogWrite(dcMotorOutput, 0);    
}

/*
 * Sets headlight LED brightness based on potentiometer or photoresistor.
 */
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

/*
 * Updates 7-segment display with given digit.
 */
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

/*
 * Sets servo position for analog speed display.
 */
void setServoPosition(int servoPosition){
    speedometerServo.write(servoPosition);
}

/*
 * Slows down DC motor if obstacle detected by ultrasonic sensor.
 */
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

// -------------------- Startup and Test Routines --------------------
/*
 * Runs hardware checks at startup: servo, display, motor, sensors.
 */
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

/*
 * Test routines for debugging and validation.
 */
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
