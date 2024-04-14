// Include necessary libraries for the sensors and communication modules
#include <Arduino.h>
#include <Wire.h>
#include <RadioLib.h>
#include <Arduino_BMI270_BMM150.h>
#include <Arduino_HS300x.h>
#include <Arduino_LPS22HB.h>
#include <SparkFun_u-blox_GNSS_v3.h>

// Define pins for the RFM95 LoRa module
#define NSS_PIN 4       // Chip select pin
#define DIO0_PIN 3      // DIO0 pin used for interrupt and transmit/receive status
#define RESET_PIN 2     // Reset pin

// Create an instance of the RFM95 module
RFM95 radio = new Module(NSS_PIN, DIO0_PIN, RESET_PIN);

// Sensor and GPS instances
SFE_UBLOX_GNSS myGNSS;  // Create an instance for the u-blox GNSS module

// Function to initialize LoRa communication
void setupLoRa()
{
  // Initialize RFM95 with specific settings for maximum range
  int state = radio.begin(915.0, 62.5, 12, 8, RADIOLIB_SX127X_SYNC_WORD, 20, 8, 0);
  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println("RFM 95 Initialization successful!");
    radio.setOutputPower(20); // Set TX power to maximum (+20 dBm)
  }
  else
  {
    Serial.print("Starting RFM 95 failed! Error code: ");
    Serial.println(state);
    while (1)
      ;
  }
}

// Function to send sensor data over LoRa
void sendSensorData(int sensorId, String data)
{
  String packet = "[" + String(sensorId) + ";" + data + "]";

  // Send packet over LoRa
  int state = radio.transmit(packet);
  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.print("Sent packet: ");
    Serial.println(packet);
  }
  else
  {
    Serial.print("Sending packet failed, error: ");
    Serial.println(state);
  }
}

// Function to send GNSS data
void sendGNSSData()
{
  long latitude = myGNSS.getLatitude();
  long longitude = myGNSS.getLongitude();
  byte SIV = myGNSS.getSIV();
  sendSensorData(1, String(latitude) + "," + String(longitude) + "," + String(SIV));
}

// Function to send LPS22HB data
void sendLPS22HBData()
{
  float pressure = BARO.readPressure();
  float temperature = BARO.readTemperature();
  float altitude = BARO.readAltitude();
  sendSensorData(2, String(pressure) + "," + String(temperature) + "," + String(altitude));
}

// Function to send accelerometer data from IMU
void sendIMUAcceleration()
{
  float x, y, z;
  // Accelerometer
  if (IMU.accelerationAvailable())
  {
    IMU.readAcceleration(x, y, z);
    sendSensorData(3, String(x) + "," + String(y) + "," + String(z));
  }
}

// Function to send gyroscope data from IMU
void sendIMUGyroscope()
{
  float x, y, z;
  // Gyroscope
  if (IMU.gyroscopeAvailable())
  {
    IMU.readGyroscope(x, y, z);
    sendSensorData(4, String(x) + "," + String(y) + "," + String(z));
  }
}

// Function to send magnetometer data from IMU
void sendIMUMagnetometer()
{
  float x, y, z;
  // Magnetometer
  if (IMU.magneticFieldAvailable())
  {
    IMU.readMagneticField(x, y, z);
    sendSensorData(5, String(x) + "," + String(y) + "," + String(z));
  }
}

// Function to send temperature and humidity data from HS300x sensor
void sendHS300xData()
{
  float temperature = HS300x.readTemperature();
  float humidity = HS300x.readHumidity();
  sendSensorData(6, String(temperature) + "," + String(humidity));
}

// Initial setup function
void setup()
{
  Serial.begin(9600);
  // while (!Serial);  // Wait for serial port to connect

  setupLoRa();  // Initialize LoRa communication

  // Initialize GNSS
  Wire.begin();
  if (myGNSS.begin() == false) {
    Serial.println("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing.");
    while (1);
  }
  myGNSS.setI2COutput(COM_TYPE_UBX);
  myGNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT);

  // Initialize Barometric Pressure and Temperature Sensor (LPS22HB)
  if (!BARO.begin()) {
    Serial.println("Failed to initialize LPS22HB sensor!");
    while (1);
  }

  // Initialize IMU for Accelerometer, Gyroscope, and Magnetometer
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  // Initialize Humidity and Temperature Sensor (HS300x)
  if (!HS300x.begin()) {
    Serial.println("Failed to initialize HS300x sensor!");
    while (1);
  }

  Serial.println("All sensors initialized successfully");
}

// Main program loop
void loop()
{
  static long lastCheckGNSS = 0;
  if (millis() - lastCheckGNSS > 1000) // Sending data every second
  { 
    lastCheckGNSS = millis();
    sendGNSSData();
  }
  static long lastCheck = 0;
   if (millis() - lastCheck > 250) // Sending data every 0.25 seconds
  { 
    lastCheck = millis();
    sendLPS22HBData();
    // sendIMUMagnetometer();
    sendIMUAcceleration();
    sendIMUGyroscope();
    sendHS300xData();
  }
}