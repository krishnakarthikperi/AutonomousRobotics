int red = 3;
int green = 5;
int blue = 6;
int buttonPin = 7;
int buzzerPin = 8;
int vehicleRed = 9;
int vehicleBlue = 11;
int vehicleGreen = 10;
int vehicleLED[] = {vehicleRed, vehicleGreen, vehicleBlue};
int pedestrianLED[] = {red, green, blue};
unsigned int yellowColor[] = {245,245,0};
unsigned int redColor[] = {245,0,0};
unsigned int greenColor[] = {0,245,0};
int numberOfVehicleLedPins = sizeof(vehicleLED)/sizeof(vehicleLED[0]);
int numberOfPedestrianLedPins = sizeof(pedestrianLED)/sizeof(pedestrianLED[0]);


void setup() {
  Serial.begin(9600);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  lightLed(pedestrianLED, redColor, numberOfPedestrianLedPins);
  lightLed(vehicleLED, greenColor, numberOfVehicleLedPins);
//  digitalWrite(red,HIGH);
//  digitalWrite(vehicleGreen, HIGH);
}

void loop() {
  int buttonState = digitalRead(buttonPin);
  Serial.println(buttonState);
  if (buttonState == LOW) {
    vehicleSwitchRed();
    pedestrianSwitchGreen();
    signalReset();
  }
  if (buttonState == HIGH) {
    digitalWrite(buzzerPin, LOW);
    lightLed(pedestrianLED,redColor, numberOfPedestrianLedPins);
  }
}

void vehicleSwitchRed(){
  lightLed(vehicleLED, yellowColor, numberOfVehicleLedPins);
  delay(3000);
  lightLed(vehicleLED, redColor, numberOfVehicleLedPins);
}

void pedestrianSwitchGreen(){
    delay(1500);
    digitalWrite(buzzerPin,HIGH);
  	lightLed(pedestrianLED, greenColor, numberOfPedestrianLedPins);
    delay(5000);
    for (int i=0;i<10;i++){
      digitalWrite(buzzerPin,LOW);
      delay(100);
      digitalWrite(buzzerPin,HIGH);
      delay(100);
    }
  	digitalWrite(buzzerPin, LOW);

}

void signalReset(){
  lightLed(pedestrianLED, redColor, numberOfPedestrianLedPins);
  delay(3000);
  lightLed(vehicleLED, greenColor, numberOfVehicleLedPins);
}

void lightLed(int rgbPins[], unsigned int rgbValues[], int numberOfPins){
  for(int i=0; i<numberOfPins; i++){
  	analogWrite(rgbPins[i],rgbValues[i]);
  }
}