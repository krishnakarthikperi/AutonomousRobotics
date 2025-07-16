#include <SPI.h>
#include <RF24.h>
#include <Servo.h>
#include <IRremote.hpp>

// === Pin Configuration ===
#define CE_PIN 7
#define CSN_PIN 8
#define SERVO_DOOR_PIN 6
#define IR_RECEIVE_PIN 2

// Shift register pins
const int dataPin  = 5;
const int clockPin = 3;
const int latchPin = 4;

// IR button codes
#define BTN_UP    0x18
#define BTN_DOWN  0x52
#define BTN_LEFT  0x08
#define BTN_RIGHT 0x5A
#define BTN_CENTER 0x1C

// --- RF Communication & State ---
RF24 radio(CE_PIN, CSN_PIN);
const byte pipeAddresses[][6] = {"00001", "00002"};
Servo servoDoor;
bool isVaultOpen = false;

// --- Data Structures (Must match the server) ---
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
  dotMatrixWithIrSetup();
  SPI.begin();
  servoDoor.attach(SERVO_DOOR_PIN);
  servoDoor.write(0);
  
  radio.begin();
  radio.setChannel(108);
  radio.openWritingPipe(pipeAddresses[0]);
  radio.openReadingPipe(1, pipeAddresses[1]);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
  
  Serial.println("Client online. Enter password, 'close', or 'reset'.");
}

void loop() {
  // Listen for emergency alerts from server
  if (radio.available()) {
    char command[32] = "";
    radio.read(&command, sizeof(command));
    Serial.print("RF_RX: Received command: ");
    Serial.println(command);
    
    if (strcmp(command, "LOCKDOWN") == 0) {
      Serial.println("LOCKDOWN received! Forcing vault lock.");
      closeVault();
    }
  }

  // Listen for user input
  if (Serial.available() > 0) {
    String userInput = Serial.readString();
    userInput.trim();

    if (userInput == "close") {
      closeVault();
    } else if (userInput == "reset") {
      Serial.println("Sending RESET command to server...");
      RequestData req;
      req.reason = RESET_REQUEST;
      sendServerRequest(req);
    } else {
      sendPasswordForValidation(userInput);
    }
  }
}

void sendPasswordForValidation(String password) {
  RequestData req;
  req.reason = AUTH_REQUEST;
  strncpy(req.password, password.c_str(), 8);

  char* response = sendServerRequest(req);
  
  if (strcmp(response, "VALID") == 0) {
    Serial.println("Password OK. Starting Dot Matrix Game...");
    if (startDotMatrixGameAuthentication()) {
      Serial.println("Access Granted!");
      // Disarm the server's security sensors FIRST
      RequestData disarmReq;
      disarmReq.reason = DISARM_REQUEST;
      sendServerRequest(disarmReq);
      // NOW open the vault, which handles both servos
      openVault();
    } else {
      Serial.println("Access Denied. Dot Matrix Authentication Failed.");
    }
  } else {
    Serial.println("Access Denied. Password authentication failed.");
  }
}

bool startDotMatrixGameAuthentication() {
  int posX = 0, posY = 0;
  showDot(posX, posY);
  while (IrReceiver.decode()) {
    IrReceiver.resume();
  }

  while (true) {
    if (IrReceiver.decode()) {
      uint8_t cmd = IrReceiver.decodedIRData.command;
      if (!(IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT)) {
        switch (cmd) {
          case BTN_DOWN:  posY = constrain(posY - 1, 0, 7); break;
          case BTN_UP:    posY = constrain(posY + 1, 0, 7); break;
          case BTN_LEFT:  posX = constrain(posX - 1, 0, 7); break;
          case BTN_RIGHT: posX = constrain(posX + 1, 0, 7); break;
          case BTN_CENTER:
            RequestData req;
            req.reason = DOT_MATRIX_AUTH_REQUEST;
            sprintf(req.password, "%d|%d", posX, posY);
            char* response = sendServerRequest(req);
            showDot(0,0);
            // On valid game, return true to proceed with opening
            return (strcmp(response, "VALID") == 0);
        }
      }
      IrReceiver.resume();
    }
    showDot(posX, posY); // Refresh display
  }
  return false; // This line is technically unreachable
}

char* sendServerRequest(RequestData req) {
  static char response[32] = "";
  memset(response, 0, sizeof(response));

  radio.stopListening();
  radio.write(&req, sizeof(RequestData));
  radio.startListening();

  unsigned long started_waiting_at = millis();
  while (!radio.available()) {
    if (millis() - started_waiting_at > 2000) {
      strcpy(response, "TIMEOUT");
      return response;
    }
  }
  radio.read(&response, sizeof(response));
  return response;
}

void openVault() {
  if (isVaultOpen) {
    Serial.println("Vault is already open.");
    return;
  }
  // 1. Tell the Server to unlock its servo
  Serial.println("Sending OPEN_LOCK command to server...");
  RequestData req;
  req.reason = OPEN_LOCK_REQUEST;
  sendServerRequest(req);

  // 2. Open the client-side door servo
  Serial.println("Opening client-side door...");
  for (int pos = 0; pos <= 180; pos++) { 
    servoDoor.write(pos);
    delay(15);
  }
  isVaultOpen = true;
  Serial.println("Vault is open.");
}

void closeVault() {
  if (!isVaultOpen) {
    Serial.println("Vault is already locked and secured.");
    return;
  }
  // 1. Close the client-side door servo
  Serial.println("Closing client-side door...");
  for (int pos = 180; pos >= 0; pos--) { 
    servoDoor.write(pos);
    delay(15);
  }
  
  // 2. Tell the Server to lock its servo
  Serial.println("Sending CLOSE_LOCK command to server...");
  RequestData lockReq;
  lockReq.reason = CLOSE_LOCK_REQUEST;
  sendServerRequest(lockReq);
  
  isVaultOpen = false;
  Serial.println("Vault is closed.");

  // 3. Tell server to re-arm security
  RequestData armReq;
  armReq.reason = ARM_REQUEST;
  sendServerRequest(armReq);
}

void dotMatrixWithIrSetup() {
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
}

void showDot(byte x, byte y) {
  byte rowData = (1 << y);
  byte colData = ~(1 << x);
  shiftOutData(rowData, colData);
}

void shiftOutData(byte rowData, byte colData) {
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, colData);
  shiftOut(dataPin, clockPin, MSBFIRST, rowData);
  digitalWrite(latchPin, HIGH);
}