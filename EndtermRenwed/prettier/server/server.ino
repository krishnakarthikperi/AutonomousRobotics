/**
 * @file server.ino
 * @brief Server-side code for a two-factor authentication (2FA) secure vault system.
 *
 * This Arduino sketch acts as the central controller for a secure vault.
 * It listens for requests from a client via an nRF24L01 radio module,
 * validates credentials, and monitors a suite of security sensors.
 *
 * --- Features ---
 * 1.  **RF Communication:** Receives requests from and sends responses/commands to a client.
 * 2.  **Authentication Handling:** Validates passwords and 2FA game coordinates sent by the client.
 * 3.  **Intrusion Detection:** Monitors multiple sensors to detect security breaches:
 * - Laser Tripwire (Laser Diode + LDR)
 * - Vibration/Shock Sensor (ADXL345 Accelerometer)
 * 4.  **Environmental Monitoring:** Uses a DHT11 sensor for temperature/humidity and a fan for ventilation.
 * 5.  **Master Lock Mode:** Enters a lockdown state upon intrusion, sounding an alarm and notifying the client.
 * 6.  **Real-Time Clock (RTC):** Keeps track of time for logging and scheduled events.
 * 7.  **Servo Control:** Operates a server-side locking mechanism.
 *
 * --- Hardware ---
 * - Arduino (or compatible board)
 * - nRF24L01 Radio Module
 * - Servo Motor
 * - Buzzer
 * - DHT11 Temperature & Humidity Sensor
 * - ADXL345 Accelerometer
 * - LDR (Light Dependent Resistor)
 * - Laser Diode
 * - Fan/Motor for ventilation
 * - DS3231 Real-Time Clock Module
 */

//==============================================================================
// LIBRARIES
//==============================================================================
#include <SPI.h>       // Required for nRF24L01 communication
#include <RF24.h>      // Main library for the nRF24L01 radio module
#include <Wire.h>      // Required for I2C communication (RTC, Accelerometer)
#include <RTClib.h>    // For interfacing with the DS3231 RTC
#include <DHT.h>       // For the DHT temperature and humidity sensor
#include <Servo.h>     // For controlling the servo motor

//==============================================================================
// PIN & CONSTANT DEFINITIONS
//==============================================================================

// --- nRF24L01 Radio Pins ---
#define CE_PIN 7
#define CSN_PIN 8

// --- Component Pins ---
#define SERVO_LOCK_PIN 6      // PWM pin for the server-side lock servo
#define BUZZER_PIN 4          // Pin to activate the alarm buzzer
#define DHTPIN 2              // Data pin for the DHT11 sensor
#define MOTORPIN 3            // Pin to control the ventilation fan/motor
#define LDR_PIN A0            // Analog pin to read the LDR value for the tripwire
#define LASER_TRANSMITTER 5   // Pin to power the laser diode for the tripwire

// --- RF & Authentication Credentials ---
RF24 radio(CE_PIN, CSN_PIN);
const byte addresses[][6] = {"00001", "00002"}; // Pipe addresses (must be inverse of client)
const char* authorizedPW = "1234";              // The correct password
const int authorizedX = 3;                      // The correct X-coordinate for the 2FA game
const int authorizedY = 3;                      // The correct Y-coordinate for the 2FA game
// --- State Variables & Objects ---
Servo servoLock;
bool isMasterLocked = false;          // If true, the system is in a critical lockdown state
bool securitySystemArmed = true;      // If true, intrusion sensors are active
int ADXL345 = 0x53;                   // I2C address for the accelerometer
float X_out, Y_out, Z_out;            // Variables to store accelerometer data
RTC_DS3231 rtc;                       // Create an RTC object
DHT dht(DHTPIN, DHT11);               // Create a DHT sensor object

// --- Sensor Thresholds & Timers ---
const float TEMP_THRESHOLD = 32.0;    // Temp in Celsius to trigger ventilation
const float HUMIDITY_THRESHOLD = 62.0; // Humidity % to trigger ventilation
const int SENSOR_THRESHOLD = 300;     // LDR value below which the laser is considered "tripped"
const float acclThreshold = 1.50;     // g-force magnitude on Z-axis to trigger brute-force alert
unsigned long previousMillis = 0;     // Used for non-blocking sensor checks
const long interval = 5000;           // Interval for periodic tasks (5 seconds)
bool emailSent = false;               // Flag for a one-time scheduled event

//==============================================================================
// DATA STRUCTURES (Must match client)
//==============================================================================

/**
 * @enum RequestReason
 * @brief Defines the purpose of a request received from the client.
 */
enum RequestReason : uint8_t {
  AUTH_REQUEST = 0, DISARM_REQUEST = 1, ARM_REQUEST = 2, RESET_REQUEST = 3,
  DOT_MATRIX_AUTH_REQUEST = 4, OPEN_LOCK_REQUEST = 5, CLOSE_LOCK_REQUEST = 6, INITIALIZATION = 7
};

/**
 * @struct RequestData
 * @brief The data packet structure for communication. Must be identical on the client.
 */
struct RequestData {
  char password[8];
  RequestReason reason;
};


//==============================================================================
// SETUP FUNCTION
//==============================================================================

/**
 * @brief Main setup function, runs once at startup.
 */
void setup() {
  Serial.begin(9600);

  // --- Initialize Hardware ---
  servoLock.attach(SERVO_LOCK_PIN);
  servoLock.write(0); // Start with the lock engaged

  pinMode(MOTORPIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LASER_TRANSMITTER, OUTPUT);
  dht.begin();

  // --- RTC Alarm ---
  rtc.disable32K();
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);
  DateTime alarmTime(0, 0, 0, 12, 12, 0); // Y,M,D are placeholders
  if (!rtc.setAlarm1(alarmTime, DS3231_A1_Hour)) {
    Serial.println("Error, alarm not set!");
  } else {
    Serial.println("Alarm is set for 12:12:00. Waiting...");
  }

  // --- Initialize I2C Devices ---
  Wire.begin();
  if (!rtc.begin()) { // Initialize Real-Time Clock
    Serial.println("FATAL: Couldn't find RTC");
    abort(); // Halt execution if RTC is not found
  }
  // Initialize ADXL345 Accelerometer
  Wire.beginTransmission(ADXL345);
  Wire.write(0x2D); // Access power control register
  Wire.write(8);    // Set to measurement mode
  Wire.endTransmission();

  // --- Initialize Radio Communication ---
  radio.begin();
  radio.setChannel(108);
  radio.openReadingPipe(1, addresses[0]); // Use address 0 for reading from client
  radio.openWritingPipe(addresses[1]);    // Use address 1 for writing to client
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening(); // Start in listening mode

  Serial.println("Server online. Security is ARMED.");
}

//==============================================================================
// MAIN LOOP
//==============================================================================

void loop() {
  // Continuously check for intrusions if the system is armed
  handleIntrusion();

  // Turn the laser on or off based on the security system's armed state
  digitalWrite(LASER_TRANSMITTER, securitySystemArmed);

  // If system is master-locked, sound the alarm and halt all other operations.
  // The only way out is a RESET command from the client.
  if (isMasterLocked) {
    digitalWrite(BUZZER_PIN, HIGH);
  }

  // --- Task 1: Process incoming commands from the client ---
  if (radio.available()) {
    RequestData req;
    radio.read(&req, sizeof(RequestData)); // Read the incoming data packet

    // Use a switch to handle the request based on its reason
    switch (req.reason) {
      case RESET_REQUEST:
        Serial.println("RESET command received! Unlocking system and re-arming.");
        isMasterLocked = false;         // Exit master lock mode
        securitySystemArmed = true;     // Re-arm the system
        digitalWrite(BUZZER_PIN, LOW);  // Turn off the alarm
        break;

      case AUTH_REQUEST: // First factor: Password check
        if (!isMasterLocked) {
          Serial.print("RF_RX: AUTH request with password: ");
          Serial.println(req.password);
          radio.stopListening();
          // Compare received password with the authorized one
          if (strcmp(req.password, authorizedPW) == 0) {
            radio.write("VALID", sizeof("VALID"));
          } else {
            radio.write("INVALID", sizeof("INVALID"));
          }
          radio.startListening();
        }
        break;

      case DOT_MATRIX_AUTH_REQUEST: // Second factor: Game coordinates check
        if (!isMasterLocked) {
          Serial.print("RF_RX: Dot matrix coords: ");
          Serial.println(req.password);
          int parsedX, parsedY;
          sscanf(req.password, "%d|%d", &parsedX, &parsedY); // Parse "X|Y" string
          radio.stopListening();
          // Compare coordinates with authorized ones
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
          servoLock.write(180); // Open the lock
        }
        break;
      
      case CLOSE_LOCK_REQUEST:
        Serial.println("Info: Received command to LOCK server-side servo.");
        servoLock.write(0); // Close the lock
        break;
        
      case DISARM_REQUEST:
        securitySystemArmed = false;
        Serial.println("Info: Security system DISARMED by client.");
        break;

      case ARM_REQUEST:
        securitySystemArmed = true;
        Serial.println("Info: Security system ARMED by client.");
        break;

      case INITIALIZATION: // Received when client comes online
        Serial.println("Info: Received client initialization. Resetting to secure state.");
        servoLock.write(0);
        securitySystemArmed = true;
        break;
    }
  }

  // --- Task 2: Perform periodic checks (non-blocking) ---
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    DateTime now = rtc.now(); // Get current time from RTC

    // Sub-task A: Check temperature/humidity and control ventilation
    float t = dht.readTemperature();
    float humidity = dht.readHumidity();
    if (!isnan(t)) { // Check for a valid sensor reading
      Serial.print("Info: Temp is ");
      Serial.print(t);
      Serial.print(" C | Humidity is ");
      Serial.println(humidity);
      // Turn on fan if thresholds are exceeded
      if (t > TEMP_THRESHOLD || humidity > HUMIDITY_THRESHOLD) {
        Serial.println("Temperature/humidity is high! Ventilation functioning!!");
        digitalWrite(MOTORPIN, HIGH);
      } else {
        digitalWrite(MOTORPIN, LOW);
      }
    }

    // Sub-task B: Report current time to Serial Monitor for logging
    Serial.print("Info: Time is ");
    Serial.print(now.hour()); Serial.print(':');
    if (now.minute() < 10) { Serial.print('0'); }
    Serial.print(now.minute()); Serial.print(':');
    if (now.second() < 10) { Serial.print('0'); }
    Serial.println(now.second());
    Serial.println("---");
  }

  // --- Task 3: Check for scheduled events ---
  // This is a placeholder for a scheduled action, like sending an email via a connected ESP.
  if (!emailSent && rtc.alarmFired(1)) {
    rtc.clearAlarm(1);
    Serial.println("SCHEDULED_EVENT: SEND_EMAIL"); // Command for a listening Python script
    emailSent = true; // Ensure this only runs once
  }
}

//==============================================================================
// HELPER FUNCTIONS
//==============================================================================

/**
 * @brief Checks intrusion sensors and triggers a master lockdown if breached.
 */
void handleIntrusion() {
  if (securitySystemArmed && !isMasterLocked) {
    // Check 1: Laser tripwire. Low LDR value means the beam is broken.
    bool isLaserTripped = analogRead(LDR_PIN) < SENSOR_THRESHOLD;
    
    // Check 2: Brute force. A sudden spike in Z-axis acceleration indicates a shock.
    bool isBruteForced = fabs(getZAccelerationMagnitude()) > acclThreshold;

    if (isLaserTripped || isBruteForced) {
      isMasterLocked = true; // CRITICAL: Engage master lock
      Serial.println("CRITICAL: INTRUSION DETECTED! SYSTEM LOCKED!");
      
      // Notify the client immediately to force its own lockdown
      radio.stopListening();
      radio.write("LOCKDOWN", sizeof("LOCKDOWN"));
      radio.startListening();
    }
  }
}

/**
 * @brief Reads the raw X, Y, and Z acceleration values from the ADXL345.
 */
void getAccelerations() {
  Wire.beginTransmission(ADXL345);
  Wire.write(0x32); // Start reading from the data register
  Wire.endTransmission(false);
  Wire.requestFrom(ADXL345, 6, true); // Request 6 bytes of data
  // Read and combine bytes for each axis, then scale the value
  X_out = (Wire.read() | Wire.read() << 8) / 256.0;
  Y_out = (Wire.read() | Wire.read() << 8) / 256.0;
  Z_out = (Wire.read() | Wire.read() << 8) / 256.0;
}

/**
 * @brief A convenience function to get the absolute magnitude of Z-axis acceleration.
 * @return The absolute value of the Z-axis acceleration in g-forces.
 */
float getZAccelerationMagnitude() {
  getAccelerations();
  return fabs(Z_out);
}