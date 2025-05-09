
void setup()
{
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
}
int ledPins [] = {3,6,5};
unsigned int yellowColor[] = {245,245,0};
unsigned int redColor[] = {245,0,0};
unsigned int greenColor[] = {0,245,0};
void loop()
{
  //red
  lightLed(ledPins, redColor, 3);
  delay(1500);
  
  //yellow
  lightLed(ledPins, yellowColor, 3);
  delay(1500);
  
  //green
  lightLed(ledPins, greenColor, 3);
  delay(1500);
}

void lightLed(int rgbPins[], unsigned int rgbValues[], int numberOfPins){
  for(int i=0; i<numberOfPins; i++){
  	analogWrite(rgbPins[i],rgbValues[i]);
  }
}