import serial
import requests
import time

# --- 1. CONFIGURATION: EDIT THESE TWO LINES ---

# Replace with the URL you got from Pipedream
PIPEDREAM_URL = 'https://eocp8ehbhjt16re.m.pipedream.net'

# Replace with your Arduino's serial port (e.g., 'COM3' or '/dev/tty.usbmodem...')
ARDUINO_PORT = 'COM3' 

# ----------------------------------------------

try:
    print(f"Attempting to connect to Arduino on {ARDUINO_PORT}...")
    arduino = serial.Serial(port=ARDUINO_PORT, baudrate=9600, timeout=1)
    print("--- Successfully connected to Arduino ---")
    print("--- Listening for serial output... ---")
    
    while True:
        # Read a line of text from the Arduino
        line = arduino.readline().decode('utf-8').strip()

        # If the line is not empty, print it
        if line:
            print(f"Arduino: {line}")
            
            # Check if this line is the trigger command
            if line == "SEND_EMAIL":
                print("\n>>> Email trigger detected! Sending request to Pipedream...")
                
                try:
                    requests.get(PIPEDREAM_URL)
                    print(">>> Success! Pipedream has been triggered.")
                except requests.exceptions.RequestException as e:
                    print(f">>> Error making web request: {e}")
                print("---") # Separator

        time.sleep(0.01)

except serial.SerialException:
    print(f"\nError: Could not connect to Arduino on port '{ARDUINO_PORT}'.")
    print("Please check that the port is correct and the IDE's Serial Monitor is closed.")

