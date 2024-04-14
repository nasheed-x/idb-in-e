#include <Arduino.h>
#include <RadioLib.h>

// Define pins for the RFM95 LoRa module
#define NSS_PIN 4       // Chip select pin
#define DIO0_PIN 3      // DIO0 pin used for interrupt and transmit/receive status
#define RESET_PIN 2     // Reset pin

// Create an instance of the RFM95 module
RFM95 radio = new Module(NSS_PIN, DIO0_PIN, RESET_PIN);

void setup() {
  Serial.begin(9600);
  while (!Serial);  // Wait for serial port to connect

  // Initialize RFM95 with the same settings as the transmitter
  if (radio.begin() != RADIOLIB_ERR_NONE) {
    Serial.println(F("RFM 95 initialization failed!"));
    while (1);
  }

  Serial.println(F("RFM 95 Receiver ready!"));
}

void loop() {
  String receivedData;
  int state = radio.receive(receivedData);

  if (state == RADIOLIB_ERR_NONE) {
    // Successfully received a packet
    Serial.print(F("Received packet: "));
    Serial.println(receivedData);

    // Extract the current time
    char timestamp[9];
    sprintf(timestamp, "%02d:%02d:%02d", hour(), minute(), second());

    // Parse the received data
    int sensorId;
    char data[64];
    if (sscanf(receivedData.c_str(), "[%d;%s]", &sensorId, data) == 2) {
      // Check which sensor the data came from
      switch (sensorId) {
        case 1: // GNSS data
          long latitude, longitude;
          byte SIV;
          if (sscanf(data, "%ld,%ld,%hhu", &latitude, &longitude, &SIV) == 3) {
            Serial.printf("[GNSS]: Current Time: %s, Latitude: %ld, Longitude: %ld, SIV: %d\n", timestamp, latitude, longitude, SIV);
          }
          break;
        case 2: // LPS22HB data
          float pressure, temperature, altitude;
          if (sscanf(data, "%f,%f,%f", &pressure, &temperature, &altitude) == 3) {
            Serial.printf("[LPS22HB]: Current Time: %s, Pressure: %.2f, Temperature: %.2f, Altitude: %.2f\n", timestamp, pressure, temperature, altitude);
          }
          break;
        case 3: // IMU Acceleration
        case 4: // IMU Gyroscope
        case 5: // IMU Magnetometer
          float x, y, z;
          if (sscanf(data, "%f,%f,%f", &x, &y, &z) == 3) {
            const char* sensorName = (sensorId == 3) ? "IMU Acceleration" : (sensorId == 4) ? "IMU Gyroscope" : "IMU Magnetometer";
            Serial.printf("[%s]: Current Time: %s, X: %.2f, Y: %.2f, Z: %.2f\n", sensorName, timestamp, x, y, z);
          }
          break;
        case 6: // HS300x data
          float temp, humidity;
          if (sscanf(data, "%f,%f", &temp, &humidity) == 2) {
            Serial.printf("[HS300x]: Current Time: %s, Temperature: %.2f, Humidity: %.2f\n", timestamp, temp, humidity);
          }
          break;
        default:
          Serial.println("Unknown sensor ID");
          break;
      }
    } else {
      Serial.println("Data parsing error");
    }
  } else if (state != RADIOLIB_ERR_RX_TIMEOUT) {
    // Some sort of error occurred
    Serial.print(F("Receive failed, error code: "));
    Serial.println(state);
  }
}
