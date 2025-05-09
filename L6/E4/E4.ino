int redPin = 6;
int bluePin = 3;
int greenPin = 5;
int buttonPin = 2;
int ledPins [] = {redPin,bluePin,greenPin};
unsigned int red = 255; 
unsigned int blue = 255;
unsigned int green = 255;
int selectedLed = 0;
int buttonState;            // the current reading from the input pin
int lastButtonState = HIGH;  // the previous reading from the input pin
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

void setup()
{
  Serial.begin(9600);
  pinMode(redPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
}

void loop()
{
  int buttonStatus = digitalRead(buttonPin);
  if (buttonStatus != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (buttonStatus != buttonState) {
      buttonState = buttonStatus;
      if(buttonState == LOW){
        if (selectedLed == 2){
            selectedLed = 0;
        }
        else {
          ++selectedLed;
        }
      }
    }
  }
  lastButtonState = buttonStatus;
  float reading = analogRead(A0);
  float value = 255*reading/1024;
  switch (selectedLed) {
  	case 0:
    	red = value;
		break;
  	case 1:
    	green = value;
		break;  	
    case 2:
    	blue = value;
		break;
  }
  Serial.print(red);  
  Serial.print(",");  
  Serial.print(green);  
  Serial.print(",");  
  Serial.println(blue); 
  unsigned int ledValues [] = {red, blue, green};
  lightLed(ledPins, ledValues, 3);
}

void lightLed(int rgbPins[], unsigned int rgbValues[], int numberOfPins){
  for(int i=0; i<numberOfPins; i++){
  	analogWrite(rgbPins[i],rgbValues[i]);
  }
}