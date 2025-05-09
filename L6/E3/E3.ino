void setup()
{
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
}
int ledPins [] = {6,3,5};
unsigned int purpleColor[] = {128,0,128};
unsigned int whiteColor[] = {255,255,255};
unsigned int orangeColor[] = {255,165,0};
void loop()
{
  //purple
  lightLed(ledPins, purpleColor, 3);
  delay(1500);
  
  //white
  lightLed(ledPins, whiteColor, 3);
  delay(1500);
  
  //orange
  lightLed(ledPins, orangeColor, 3);
  delay(1500);
}

void lightLed(int rgbPins[], unsigned int rgbValues[], int numberOfPins){
  for(int i=0; i<numberOfPins; i++){
  	analogWrite(rgbPins[i],rgbValues[i]);
  }
}