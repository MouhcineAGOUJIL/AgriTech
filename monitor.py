import serial
import time

# CONFIGURATION
# Proteus is on COM1, so Python must be on COM2
SERIAL_PORT = 'COM2'  
BAUD_RATE = 9600

print(f"Connecting to Greenhouse on {SERIAL_PORT}...")

try:
    # Open the connection
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    time.sleep(2) # Wait for connection to stabilize
    print("Connected! Waiting for data...")
    print("-" * 40)

    while True:
        # Check if data is available in the buffer
        if ser.in_waiting > 0:
            # Read the line, decode from bytes to string, strip whitespace
            try:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                
                # Filter out empty lines
                if len(line) > 0:
                    print(f"[RECEIVED] {line}")
                    
                    # OPTIONAL: Add Logic here
                    # if "TEMP:35" in line:
                    #     print(">>> WARNING: HIGH TEMP DETECTED ON PC! <<<")

            except Exception as e:
                print(f"Error decoding line: {e}")
                
except serial.SerialException:
    print(f"ERROR: Could not open {SERIAL_PORT}. Is it being used by another app?")
except KeyboardInterrupt:
    print("\nExiting...")
    if 'ser' in locals() and ser.is_open:
        ser.close()