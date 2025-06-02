#define PIN_SHIFT 11 // SHCP
#define PIN_STORE 10 // STCP
#define PIN_DATA  8  // DS
#define PIN_OUTPUT_ENABLE 9 // OE

void setup() {
  Serial.begin(9600);
  pinMode(PIN_STORE, OUTPUT);
  pinMode(PIN_SHIFT, OUTPUT);
  pinMode(PIN_DATA, OUTPUT);
  pinMode(PIN_OUTPUT_ENABLE, OUTPUT);

  digitalWrite(PIN_OUTPUT_ENABLE, LOW); // Enable output (active low)
}

// Each sub-array represents: [dp, c, d, e, g, f, a, b]
int numbers[10][8] = {
  {0, 1, 0, 0, 0, 0, 0, 1}, // 1
  {0, 0, 1, 1, 1, 0, 1, 1}, // 2
  {0, 1, 1, 0, 1, 0, 1, 1}, // 3
  {0, 1, 0, 0, 1, 1, 0, 1}, // 4
  {0, 1, 1, 0, 1, 1, 1, 0}, // 5
  {0, 1, 1, 1, 1, 1, 1, 0}, // 6
  {0, 1, 0, 0, 0, 0, 1, 1}, // 7
  {0, 1, 1, 1, 1, 1, 1, 1}, // 8
  {0, 1, 1, 0, 1, 1, 1, 1}, // 9
  {0, 1, 1, 1, 0, 1, 1, 1}  // 0
};

void loop() {
  for (int i = 0; i < 10; i++) {
    digitalWrite(PIN_STORE, LOW);
    
    // Send bits from MSB to LSB
    for (int j = 7; j >= 0; j--) {
      digitalWrite(PIN_SHIFT, LOW);
      digitalWrite(PIN_DATA, numbers[i][j]);
      digitalWrite(PIN_SHIFT, HIGH);
    }

    digitalWrite(PIN_STORE, HIGH);

    Serial.print("Displaying number: ");
    Serial.println(i);

    delay(1000); // 1 second delay between digits
  }
}
