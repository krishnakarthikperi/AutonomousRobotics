#define PIN_SHIFT 10 // connected to SHCP
#define PIN_STORE 9  // connected to STCP
#define PIN_DATA  8  // connected to DS


void setup()
{
  Serial.begin(9600);
  pinMode(PIN_STORE, OUTPUT);
  pinMode(PIN_SHIFT, OUTPUT);
  pinMode(PIN_DATA, OUTPUT);

  
}

void loop() {
  for (int number = 0; number < 256; number++) {
    int binary[8]; // to store the binary digits

    // Manually convert number to binary (MSB first)
    int n = number;
    for (int i = 7; i >= 0; i--) {
      binary[i] = n % 2;
      n = n / 2;
    }

    digitalWrite(PIN_STORE, LOW); // Start sending data

    // Send each bit
    for (int i = 0; i < 8; i++) {
      digitalWrite(PIN_SHIFT, LOW);       // Prepare for bit
      digitalWrite(PIN_DATA, binary[i]);  // Send current bit
      digitalWrite(PIN_SHIFT, HIGH);      // Clock in the bit
    }

    digitalWrite(PIN_STORE, HIGH); // Latch data to output

    Serial.println(number);
    delay(200); // Wait so we can see the LEDs
  }
}