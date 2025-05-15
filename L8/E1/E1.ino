#include<Servo.h>;
Servo myservo;
const int trigPin = 9;
const int echoPin = 10;
const int buzzerPin = 5;
const int servoPin = 3;

float duration, distance;
float threshold = 100;

void setup() {
  myservo.attach(servoPin);
  pinMode(trigPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(echoPin, INPUT);
  Serial.begin(9600);
}

void loop() {
  for(int i=0; i<181; i++){
    detectObject(i);
  }

  for(int i=181; i>=0; i--){
    detectObject(i);
  }
}
void detectObject(int i){
    myservo.write(i);
    ultraSonicDistance();
    while(distance<threshold){
          ultraSonicDistance();
          digitalWrite(buzzerPin, HIGH);
          Serial.print("Pos: ");
          Serial.print(i);
          Serial.print(" | Dist: ");
          Serial.println(distance);
    }
    digitalWrite(buzzerPin, LOW);
}

float ultraSonicDistance(){
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = (duration*.0343)/2;
  delay(100);
  return distance;
}