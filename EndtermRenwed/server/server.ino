#include <SPI.h>
#include <RF24.h>
#include <Wire.h>

#define CE_PIN 9
#define CSN_PIN 8

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
    radioSetup();
}

void radioSetup(){
    radio.begin();
    radio.setChannel(105);
    radio.openReadingPipe(1, pipeAddresses[0]);
    radio.openWritingPipe(pipeAddresses[1]);
    radio.setPALevel(RF24_PA_LOW);
    radio.startListening();
    DEBUG_PRINTLN("Server RF online");
}

bool isMasterLocked = false;
bool securitySystemArmed = true;
const char* authorizedPW = "1234";
void loop(){
    if(radio.available()){
        RequestData req = {};
        radio.read(&req, sizeof(RequestData));
        if (req.magic != 0xA5) {
            DEBUG_PRINTLN("Warning: Invalid packet received. Ignored.");
            return;
        }
        switch (req.reason) {
        case RESET_REQUEST:
            DEBUG_PRINTLN("RESET command received! Unlocking system.");
            isMasterLocked = false;
            securitySystemArmed = true; // Re-arm security on reset
            break;
        case AUTH_REQUEST:
            DEBUG_PRINT("RF_RX: Received AUTH request with password: ");
            DEBUG_PRINTLN(req.password);
            radio.stopListening();
            if (strcmp(req.password, authorizedPW) == 0) {
                if(radio.write("VALID", sizeof("VALID")))
                    DEBUG_PRINTLN("RF_TX: Sent response: VALID");
                else
                    DEBUG_PRINTLN("RF_TX: Password Validated | Tranmission Failed");                
                } else {
                    if(radio.write("INVALID", sizeof("INVALID")))
                        Serial.println("RF_TX: Sent response: INVALID");
                    else
                        DEBUG_PRINTLN("RF_TX: Password NOT Validated | Tranmission Failed");                
            }
            radio.startListening();
            break;

        case DISARM_REQUEST:
            securitySystemArmed = false;
            DEBUG_PRINTLN("Info: Security system DISARMED by client.");
            break;

        case ARM_REQUEST:
            securitySystemArmed = true;
            DEBUG_PRINTLN("Info: Security system ARMED by client.");
            break;

        }
    }
}