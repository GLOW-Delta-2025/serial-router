import serial
import time

# -------- CONFIG --------
# Change this to your Teensy’s serial port (check Arduino IDE > Tools > Port)
# Windows example: 'COM5'
# macOS/Linux example: '/dev/ttyACM0' or '/dev/ttyUSB0' or 'COM7'(could be different on your system, check device manager)
PORT = 'COM7'
BAUD = 9600

# -------- SETUP SERIAL --------
try:
    ser = serial.Serial(PORT, BAUD, timeout=1)
    time.sleep(2)  # Wait for Teensy to reset after opening port
    print(f"Connected to {PORT} at {BAUD} baud.")
except serial.SerialException as e:
    print(f"Error: {e}")
    exit(1)

# -------- SEND COMMAND --------
def send_command(cmd: str):
    """Send full command string ending with ##"""
    if not cmd.endswith("##"):
        cmd += "##"
    ser.write(cmd.encode('utf-8'))
    ser.flush()
    print(f"Sent → {cmd}")

# -------- READ RESPONSES --------
def read_responses():
    """Read available responses from Teensy"""
    while ser.in_waiting > 0:
        response = ser.readline().decode('utf-8', errors='ignore').strip()
        if response:
            print(f"Received ← {response}")

# -------- MAIN LOOP --------
try:
    while True:
        # Example: send a request to ARM1
        send_command("!![ARM1]:REQUEST:MAKE_STAR{120,RED,90,6}##")
        time.sleep(1)

        read_responses()
        time.sleep(2)

except KeyboardInterrupt:
    print("\nExiting...")
finally:
    ser.close()
