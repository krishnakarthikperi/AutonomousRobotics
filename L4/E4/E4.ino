/* 
* Each time you press the button, the number displayed on the 7-segment 
* display increases by 1. The display should return to “0” after “9”. Define each 
* number display as a function 
*/
// Pin definitions
int a = 8;
int b = 9;
int c = 3;
int d = 4;
int e = 5;
int f = 7;
int g = 6;
int dp = 2; // Decimal point (not used here but can be implemented)

// Button Pin
int buttonPin = 10; // Button connected to pin 10

// Variables
int currentNumber = 0;
int lastButtonState = LOW;
int buttonState = LOW;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50; // debounce delay in milliseconds

// 7-Segment Display Pins
int outputPins[] = {a, b, c, d, e, f, g, dp};

void setup() {
    Serial.begin(9600);

    // Set the pins as output
    for (int i = 0; i < sizeof(outputPins) / sizeof(outputPins[0]); i++) {
        pinMode(outputPins[i], OUTPUT);
    }

    // Set the button pin as input
    pinMode(buttonPin, INPUT);
}

void loop() {
    // Read the button state
    int reading = digitalRead(buttonPin);

    // Check if the button state has changed
    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }

    // If the button has been pressed and debounce time has passed
    if ((millis() - lastDebounceTime) > debounceDelay) {
        // If the button state has changed and is now HIGH (pressed)
        if (reading == HIGH && buttonState == LOW) {
            // Increment the current number, wrap around at 10
            currentNumber++;
            if (currentNumber > 9) {
                currentNumber = 0;
            }

            // Display the new number on the 7-segment display
            displayNumber(currentNumber);
        }
        buttonState = reading;
    }

    lastButtonState = reading;
}

// Function to display a number (0-9) on the 7-segment display
void displayNumber(int num) {
    // Turn off all segments first
    clearDisplay();

    switch (num) {
        case 0:
            display0();
            break;
        case 1:
            display1();
            break;
        case 2:
            display2();
            break;
        case 3:
            display3();
            break;
        case 4:
            display4();
            break;
        case 5:
            display5();
            break;
        case 6:
            display6();
            break;
        case 7:
            display7();
            break;
        case 8:
            display8();
            break;
        case 9:
            display9();
            break;
    }
}

// Functions to display each digit (0-9)
void display0() {
    digitalWrite(a, HIGH);
    digitalWrite(b, HIGH);
    digitalWrite(c, HIGH);
    digitalWrite(d, HIGH);
    digitalWrite(e, HIGH);
    digitalWrite(f, HIGH);
    digitalWrite(g, LOW);  // Turn off the middle segment
}

void display1() {
    digitalWrite(a, LOW);
    digitalWrite(b, HIGH);
    digitalWrite(c, HIGH);
    digitalWrite(d, LOW);
    digitalWrite(e, LOW);
    digitalWrite(f, LOW);
    digitalWrite(g, LOW);
}

void display2() {
    digitalWrite(a, HIGH);
    digitalWrite(b, HIGH);
    digitalWrite(c, LOW);
    digitalWrite(d, HIGH);
    digitalWrite(e, HIGH);
    digitalWrite(f, LOW);
    digitalWrite(g, HIGH);
}

void display3() {
    digitalWrite(a, HIGH);
    digitalWrite(b, HIGH);
    digitalWrite(c, HIGH);
    digitalWrite(d, HIGH);
    digitalWrite(e, LOW);
    digitalWrite(f, LOW);
    digitalWrite(g, HIGH);
}

void display4() {
    digitalWrite(a, LOW);
    digitalWrite(b, HIGH);
    digitalWrite(c, HIGH);
    digitalWrite(d, LOW);
    digitalWrite(e, LOW);
    digitalWrite(f, HIGH);
    digitalWrite(g, HIGH);
}

void display5() {
    digitalWrite(a, HIGH);
    digitalWrite(b, LOW);
    digitalWrite(c, HIGH);
    digitalWrite(d, HIGH);
    digitalWrite(e, LOW);
    digitalWrite(f, HIGH);
    digitalWrite(g, HIGH);
}

void display6() {
    digitalWrite(a, HIGH);
    digitalWrite(b, LOW);
    digitalWrite(c, HIGH);
    digitalWrite(d, HIGH);
    digitalWrite(e, HIGH);
    digitalWrite(f, HIGH);
    digitalWrite(g, HIGH);
}

void display7() {
    digitalWrite(a, HIGH);
    digitalWrite(b, HIGH);
    digitalWrite(c, HIGH);
    digitalWrite(d, LOW);
    digitalWrite(e, LOW);
    digitalWrite(f, LOW);
    digitalWrite(g, LOW);
}

void display8() {
    digitalWrite(a, HIGH);
    digitalWrite(b, HIGH);
    digitalWrite(c, HIGH);
    digitalWrite(d, HIGH);
    digitalWrite(e, HIGH);
    digitalWrite(f, HIGH);
    digitalWrite(g, HIGH);
}

void display9() {
    digitalWrite(a, HIGH);
    digitalWrite(b, HIGH);
    digitalWrite(c, HIGH);
    digitalWrite(d, HIGH);
    digitalWrite(e, LOW);
    digitalWrite(f, HIGH);
    digitalWrite(g, HIGH);
}

// Function to turn off all segments
void clearDisplay() {
    for (int i = 0; i < sizeof(outputPins) / sizeof(outputPins[0]); i++) {
        digitalWrite(outputPins[i], LOW);
    }
}
