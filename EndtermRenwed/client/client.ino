#include <SPI.h>
#include <RF24.h>
#include <Servo.h>

#define CE_PIN 9
#define CSN_PIN 8
#define RST_PIN         4           // Configurable, see typical pin layout above
#define SS_PIN          7          // Configurable, see typical pin layout above

#define DEBUG 1 // Set to 1 for detailed Serial output, 0 for quiet operation
#if DEBUG
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif


RF24 radio(CE_PIN, CSN_PIN);
const byte pipeAddresses[][6] = {"trnmtr","recivr"};

enum RequestReason : uint8_t {
  AUTH_REQUEST = 0,
  DISARM_REQUEST = 1,
  ARM_REQUEST = 2,
  RESET_REQUEST = 3
};
struct RequestData {
    uint8_t magic;
    char password[8];
    RequestReason reason;
};

void setup(){
    Serial.begin(9600);
    pinMode(SS_PIN, OUTPUT);     
    pinMode(RST_PIN, OUTPUT);    
    // digitalWrite(SS_PIN, HIGH);  // Disable RFID chip select
    // digitalWrite(RST_PIN, LOW);  // Hold RFID in reset
    enableRF();
    delay(100);  // Let everything settle

    radioSetup();
}

void loop(){
/*     if(radio.available()){
        char command[32] = "";
        radio.read(&command,sizeof(command));
        DEBUG_PRINT("RF_RX: Received command: ");
        DEBUG_PRINTLN(command);

        if (strcmp(command, "LOCKDOWN") == 0) {
        DEBUG_PRINTLN("LOCKDOWN received! Forcing vault lock.");
        closeVault();
        }        
    } */

    if(Serial.available()>0){
        String userInput = Serial.readString();
        userInput.trim();
        // CHANGED: Added an 'else if' for the reset command
        if (userInput == "close") {
            DEBUG_PRINTLN("Closing vault....");
        } else if (userInput == "reset") {
        DEBUG_PRINTLN("Sending RESET command to server...");
        RequestData req;
        req.magic = 0xA5;
        req.reason = RESET_REQUEST;
        radio.stopListening();
        radio.write(&req, sizeof(RequestData));
        radio.startListening();
        } else {
            sendPasswordForValidation(userInput);
        }    
    }
}

void radioSetup(){
    radio.begin();
    radio.setChannel(105);
    radio.openWritingPipe(pipeAddresses[0]);
    radio.openReadingPipe(1,pipeAddresses[1]);
    radio.setPALevel(RF24_PA_LOW);
    radio.startListening();
    DEBUG_PRINTLN("Client RF online");
}

void sendPasswordForValidation(String password){
    RequestData req;
    req.magic = 0xA5;
    req.reason = AUTH_REQUEST;
    strncpy(req.password, password.c_str(), 8);
    radio.stopListening();
    radio.write(&req, sizeof(RequestData));
    radio.startListening();

    unsigned long started_waiting_at = millis();
    bool timeout = false;
    while(!radio.available()){
        if ((millis() - started_waiting_at ) % 200 == 0) DEBUG_PRINTLN("Waiting for response");
    }
    char response[32] = "";
    radio.read(&response, sizeof(response));
    DEBUG_PRINT("RF_RX: Server responded: ");
    DEBUG_PRINTLN(response);

}

void enableRF() {
  digitalWrite(SS_PIN, HIGH);     // Disable RC522
  digitalWrite(RST_PIN, LOW);     // Hold RC522 in reset (disabled)
  delay(100);

  digitalWrite(CSN_PIN, LOW);     // Enable RF24
  delay(100);
}

void enableRFID() {
  digitalWrite(CSN_PIN, HIGH);    // Disable RF24
  delay(100);

  digitalWrite(RST_PIN, HIGH);    // Release RC522 from reset
  digitalWrite(SS_PIN, LOW);      // Enable RC522
  delay(100);
}
