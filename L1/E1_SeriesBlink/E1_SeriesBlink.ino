/* 
* Multiple LEDs: Have a series of 5 LEDs and have them turn on one after 
* the other, left to right.
*/
int pins[] = {11,10,9,5,3};
void setup() {
  // put your setup code here, to run once:
  for (int i=0; i<sizeof(pins)/sizeof(*pins);i++){
    pinMode(pins[i],OUTPUT);
  }
}

void loop() {
  for (int i=0; i<sizeof(pins)/sizeof(*pins);i++){
    digitalWrite(pins[i],HIGH);
    delay(200);
    digitalWrite(pins[i],LOW);
  }
}
