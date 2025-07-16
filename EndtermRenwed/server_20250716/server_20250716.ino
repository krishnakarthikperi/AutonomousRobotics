#include <SPI.h>
#include <RF24.h>
#include <Wire.h>
#include <RTClib.h>
#include <DHT.h>
#include <Servo.h>

// --- Pin Definitions ---
#define CE_PIN 7
#define CSN_PIN 8
#define SERVO_LOCK_PIN 6
#define BUZZER_PIN 4
#define DHTPIN 2
#define MOTORPIN 3
#define LDR_PIN A0
#define LASER_TRANSMITTER 5

// --- RF & Auth Setup ---
RF24 radio(CE_PIN, CSN_PIN);
const byte addresses[][6] = {"00001", "00002"};
const char* authorizedPW = "1234";
const int authorizedX = 3;
const int authorizedY = 3;

// --- State Variables & Objects ---
Servo servoLock;
bool isMasterLocked = false;
bool securitySystemArmed = true; 
int ADXL345 = 0x53;
float X_out, Y_out, Z_out;
const float TEMP_THRESHOLD = 32.0;
const int SENSOR_THRESHOLD = 300;
const float acclThreshold = 1.50;
unsigned long previousMillis = 0;
const long interval = 5000;
bool emailSent = false;
unsigned long startTime = 0;
RTC_DS3231 rtc;
DHT dht(DHTPIN, DHT11);

// --- Data Structures ---
enum RequestReason : uint8_t {
  AUTH_REQUEST = 0, DISARM_REQUEST = 1, ARM_REQUEST = 2, RESET_REQUEST = 3,
  DOT_MATRIX_AUTH_REQUEST = 4, OPEN_LOCK_REQUEST = 5, CLOSE_LOCK_REQUEST = 6
};
struct RequestData {
  char password[8];
  RequestReason reason;
};


void setup() {
  Serial.begin(9600);
  servoLock.attach(SERVO_LOCK_PIN);
  servoLock.write(0);
  
  radio.begin();
  radio.setChannel(108);
  radio.openReadingPipe(1, addresses[0]);
  radio.openWritingPipe(addresses[1]);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
  Serial.println("RF24 Server active on Channel 108.");

  pinMode(MOTORPIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LASER_TRANSMITTER,OUTPUT);
  dht.begin();
  if (!rtc.begin()) { Serial.println("Couldn't find RTC"); abort(); }
  Wire.begin();
  Wire.beginTransmission(ADXL345);
  Wire.write(0x2D); Wire.write(8); Wire.endTransmission();
  startTime = millis();
  Serial.println("Server online. Security is ARMED.");
}

void loop() {
  digitalWrite(LASER_TRANSMITTER,securitySystemArmed);

  // Always check for RF commands first, so the reset command can get through
  if (radio.available()) {
    RequestData req;
    radio.read(&req, sizeof(RequestData));

    switch (req.reason) {
      case RESET_REQUEST:
        Serial.println("RESET command received! Unlocking system.");
        isMasterLocked = false;
        securitySystemArmed = true;
        digitalWrite(BUZZER_PIN, LOW);
        break;

      case AUTH_REQUEST:
        if (!isMasterLocked) {
          Serial.print("RF_RX: AUTH request with password: ");
          Serial.println(req.password);
          radio.stopListening();
          if (strcmp(req.password, authorizedPW) == 0) {
            radio.write("VALID", sizeof("VALID"));
          } else {
            radio.write("INVALID", sizeof("INVALID"));
          }
          radio.startListening();
        }
        break;

      case DOT_MATRIX_AUTH_REQUEST:
        if (!isMasterLocked) {
          Serial.print("RF_RX: Dot matrix coords: ");
          Serial.println(req.password);
          int parsedX, parsedY;
          sscanf(req.password, "%d|%d", &parsedX, &parsedY);
          radio.stopListening();
          if (parsedX == authorizedX && parsedY == authorizedY) {
            radio.write("VALID", sizeof("VALID"));
          } else {
            radio.write("INVALID", sizeof("INVALID"));
          }
          radio.startListening();
        }
        break;

      case OPEN_LOCK_REQUEST:
        if (!isMasterLocked) {
          Serial.println("Info: Received command to UNLOCK server-side servo.");
          servoLock.write(180);
        }
        break;
        
      case CLOSE_LOCK_REQUEST:
        Serial.println("Info: Received command to LOCK server-side servo.");
        servoLock.write(0);
        break;
        
      case DISARM_REQUEST:
        securitySystemArmed = false;
        digitalWrite(LASER_TRANSMITTER,LOW);
        Serial.println("Info: Security system DISARMED by client.");
        break;

      case ARM_REQUEST:
        securitySystemArmed = true;
        digitalWrite(LASER_TRANSMITTER,HIGH);
        Serial.println("Info: Security system ARMED by client.");
        break;
    }
  }

  // If system is master-locked, buzz and stop all other operations
  if (isMasterLocked) {
    digitalWrite(BUZZER_PIN, HIGH);
    return;
  }
  
  handleIntrusion();
  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    float t = dht.readTemperature();
    if (!isnan(t)) {
      Serial.print("Info: Temp is ");
      Serial.print(t);
      Serial.println(" C");
      if(t > TEMP_THRESHOLD){
        Serial.println("Temperature is high! Ventilation functioning!!");
        digitalWrite(MOTORPIN, HIGH);
      } else {
        digitalWrite(MOTORPIN, LOW);
      }
    
    }

    // Background Task 2: Report Current Time
    DateTime now = rtc.now();
    Serial.print("Info: Time is ");
    Serial.print(now.hour()); Serial.print(':');
    if (now.minute() < 10) { Serial.print('0'); }
    Serial.print(now.minute()); Serial.print(':');
    if (now.second() < 10) { Serial.print('0'); }
    Serial.print(now.second());
    Serial.println();
    Serial.println("---");
  }
  
  if (!emailSent && (currentMillis - startTime >= 120000)) {
    Serial.println("SEND_EMAIL");
    emailSent = true; 
  }
}

// --- Helper Functions ---
void handleIntrusion() {
  if (securitySystemArmed) {
    bool isLaserTripped = analogRead(LDR_PIN) < SENSOR_THRESHOLD;
    bool isBruteForced = fabs(getZAccelerationMagnitude()) > acclThreshold;
    if (isLaserTripped || isBruteForced) {
      isMasterLocked = true;
      Serial.println("CRITICAL: INTRUSION DETECTED! SYSTEM LOCKED!");
      radio.stopListening();
      radio.write("LOCKDOWN", sizeof("LOCKDOWN"));
      radio.startListening();
    }
  }
}

void getAccelerations(){
  Wire.beginTransmission(ADXL345);
  Wire.write(0x32);
  Wire.endTransmission(false);
  Wire.requestFrom(ADXL345, 6, true);
  X_out = (Wire.read()| Wire.read() << 8) / 256.0;
  Y_out = (Wire.read()| Wire.read() << 8) / 256.0;
  Z_out = (Wire.read()| Wire.read() << 8) / 256.0;
}

float getZAccelerationMagnitude(){
  getAccelerations();
  return fabs(Z_out);
}