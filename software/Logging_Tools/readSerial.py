import serial
import time

# Configure the serial port (adjust the port name and baud rate)
arduino = serial.Serial('COM5', 115200)
time.sleep(2)  # Give time for Arduino to reset

input("Waiting... (Click enter)")
with open('arduino_data.csv', 'w') as file:
    try:
        while True:
            if arduino.in_waiting > 0:
                data = arduino.readline().decode('utf-8').strip()
                print(f"Received: {data}")
   
                file.write(data + "\n")
    except KeyboardInterrupt:
        print("Data collection stopped.")
    finally:
        arduino.close()