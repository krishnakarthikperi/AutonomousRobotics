
int motorEnableA = 11;
int motorInput1 = 10;
int motorInput2 = 9;
int joystickButton = 2;
int joystickX = A0;
int joystickY = A1;

int lastButtonState = LOW;
unsigned int debounceDelay = 500;
unsigned int debounceTime = 0;
bool motorEnabled = LOW;
float motorSpeed;
void setup() {
	pinMode(motorEnableA, OUTPUT);
	pinMode(motorInput1, OUTPUT);
	pinMode(motorInput2, OUTPUT);
	pinMode(joystickButton, INPUT_PULLUP);

	// Turn off motor - Initial state
	digitalWrite(motorInput1, LOW);
	digitalWrite(motorInput2, LOW);
  Serial.begin(9600);
}

void loop() {
    int buttonState = digitalRead(joystickButton);
    if((millis() - debounceTime) > debounceDelay){
        if(buttonState != lastButtonState){
            lastButtonState = buttonState;
            debounceTime = millis();
            if(buttonState){
                motorEnabled = !motorEnabled;
                Serial.print("MotorEnabled");
                Serial.print(motorEnabled);
                Serial.print("|");
                digitalWrite(motorInput1, motorEnabled);
                digitalWrite(motorInput2, LOW);
            }
        }
    }

    float joystickXReading = analogRead(joystickX);
    motorSpeed = ((1024-joystickXReading)/1024.0)*255;
  speedControl();
  	Serial.print(buttonState);
  	Serial.print("|");
  	Serial.print(motorEnabled);
  	Serial.print("|");
  	Serial.println(motorSpeed);
}

// This function lets you control speed of the motors
void speedControl() {
    analogWrite(motorEnableA, motorSpeed);
}