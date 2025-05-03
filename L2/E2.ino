/* 
* Have an LED turn on when a button is pressed. 
* Read the button by the Arduino, and then 
* control then LED by the Arduino. 
*/

const int buttonPin = 2;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
int buttonState;            
int lastButtonState = LOW;  
int ledState = HIGH;

void setup(){
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(buttonPin, INPUT);
  digitalWrite(LED_BUILTIN, ledState);
}

void loop(){
  int reading = digitalRead(buttonPin);
  Serial.println(reading);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == HIGH) {
        ledState = !ledState;
      }
    }
  }
  digitalWrite(LED_BUILTIN, ledState);
  lastButtonState = reading;
}