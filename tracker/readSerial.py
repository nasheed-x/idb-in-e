import serial
import csv
import time

# Replace 'COM3' with the appropriate serial port for your system
serial_port = '/dev/cu.usbmodem1101'  # For Windows, it might be 'COM3', 'COM4', etc.
baud_rate = 115200  # Set to the baud rate used in your Arduino code

# Open the serial port
ser = serial.Serial(serial_port, baud_rate, timeout=1)

# Open a CSV file for writing
with open('sensor_data.csv', 'w', newline='') as csvfile:
    csv_writer = csv.writer(csvfile)
    # Write the header row
    csv_writer.writerow(['Timestamp', 'Sensor', 'Data'])

    while True:
        try:
            # Read a line from the serial port
            line = ser.readline().decode('utf-8').strip()
            if line:
                # Get the current timestamp in milliseconds
                timestamp = int(time.time() * 1000)
                # Print to console (optional)
                print(f"{timestamp}: {line}")
                # Write to CSV file
                csv_writer.writerow([timestamp] + line.split(": "))
        except serial.SerialException as e:
            print(f"Serial error: {e}")
            break
        except KeyboardInterrupt:
            print("Program interrupted by user")
            break
        except Exception as e:
            print(f"Unexpected error: {e}")
            break

# Close the serial port
ser.close()
