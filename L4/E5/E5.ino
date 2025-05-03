int a =8;
int b =9;
int c =3;
int d =4;
int e =5;
int f = 7;
int g = 6;
int dp = 2;
int potentiometerInput = A0;
int numberState;
int oldNumber = 0;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
int outputPins[] = {a,b,c,d,e,f,g,dp};
const int numbers[][8] = {  // Segments mapping for numbers 0 to 9
    {a,b,c,d,e,f},
    {b,c},
    {a,b,g,e,d},
    {a,b,g,c,d},
    {f,g,b,c},
    {a,f,g,c,d},
    {a,f,g,c,d,e},
    {a,b,c},
    {a,b,c,d,e,f,g},
    {a,f,g,b,c,d}
};

void setup(){
  	Serial.begin(9600);
    for (int i=0; i<sizeof(outputPins)/sizeof(*outputPins); i++){
        pinMode(outputPins[i],OUTPUT);
    }
  	pinMode(potentiometerInput,INPUT);
}

void loop(){
  int reading = analogRead(potentiometerInput);
  int number = reading/(1024/10);
  if(number!=oldNumber){
    oldNumber = number;
	lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if(number != numberState){
      numberState = number;
    	clearDisplay();
        for (int j=0; j<sizeof(numbers[number])/sizeof(*numbers[number]); j++) {
            digitalWrite(numbers[number][j],HIGH);
        }      
    }
  }
}


void clearDisplay(void) 
{ 
    for (int i=0; i<sizeof(number8)/sizeof(*number8); i++) {
        digitalWrite(number8[i],LOW);
    }
} 