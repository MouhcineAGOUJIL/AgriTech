import time
import serial
import threading
import json
import csv
import os
from datetime import datetime
from flask import Flask, jsonify, render_template
from flask_cors import CORS

# --- CONFIGURATION ---
SERIAL_PORT = 'COM2'  # Must match your VSPD pair
BAUD_RATE = 9600
CSV_FILE = 'AgrtiTech_logs.csv' # Name of the log file

app = Flask(__name__)
CORS(app)

# Global variable to store the latest sensor data
current_data = {
    "temp": 0,
    "humid": 0,
    "fan": False,
    "pump": False,
    "last_updated": 0,
    "status": "Disconnected"
}

# --- CSV LOGGING FUNCTION ---
def log_to_csv(temp, humid, fan, pump):
    """Appends a new row of data to the CSV file."""
    file_exists = os.path.isfile(CSV_FILE)
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    try:
        with open(CSV_FILE, 'a', newline='') as csvfile:
            writer = csv.writer(csvfile)
            
            # Write headers if this is a new file
            if not file_exists:
                writer.writerow(['Timestamp', 'Temperature (C)', 'Humidity (%)', 'Fan Status', 'Pump Status'])
            
            # Write the data
            writer.writerow([timestamp, temp, humid, 'ON' if fan else 'OFF', 'ON' if pump else 'OFF'])
            # print(f"Data saved to {CSV_FILE}") # Uncomment if you want to see every save in console
            
    except Exception as e:
        print(f"Error writing to CSV: {e}")

# --- SERIAL LISTENER THREAD ---
def read_serial_data():
    global current_data
    print(f"Trying to connect to {SERIAL_PORT}...")
    
    while True:
        try:
            with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
                print(f"Connected to {SERIAL_PORT}!")
                current_data["status"] = "Connected"
                
                while True:
                    if ser.in_waiting > 0:
                        try:
                            # Expecting line: "TEMP:25,HUM:60"
                            line = ser.readline().decode('utf-8', errors='ignore').strip()
                            
                            if "TEMP" in line and "HUM" in line:
                                parts = line.split(',')
                                # Parse TEMP
                                temp_str = parts[0].split(':')[1]
                                temp_val = int(float(temp_str))
                                
                                # Parse HUMID
                                hum_str = parts[1].split(':')[1]
                                hum_val = int(float(hum_str))
                                
                                # Derive Device Status
                                is_fan_on = temp_val > 30
                                is_pump_on = hum_val < 40
                                
                                # Update Global State
                                current_data["temp"] = temp_val
                                current_data["humid"] = hum_val
                                current_data["fan"] = is_fan_on
                                current_data["pump"] = is_pump_on
                                
                                current_data["last_updated"] = time.time()
                                current_data["status"] = "Online"
                                
                                print(f"Received: {line} | Saving to CSV...")
                                
                                # --- SAVE TO CSV HERE ---
                                log_to_csv(temp_val, hum_val, is_fan_on, is_pump_on)
                                
                        except Exception as e:
                            print(f"Parse Error: {e}")
                            
                    time.sleep(1.0) # Read every 1 second (matches PIC delay)

        except serial.SerialException:
            current_data["status"] = "Disconnected"
            print("Waiting for Serial connection...")
            time.sleep(2) # Retry delay

# Start the background thread
thread = threading.Thread(target=read_serial_data)
thread.daemon = True
thread.start()

# --- FLASK ROUTES ---

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/data')
def get_data():
    return jsonify(current_data)

if __name__ == '__main__':
    print("Server starting at http://localhost:5000")
    app.run(host='0.0.0.0', port=5000, debug=False)