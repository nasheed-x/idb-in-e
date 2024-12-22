#include <Arduino.h>
#include <Wire.h>
#include <Arduino_BMI270_BMM150.h>
#include <Arduino_HS300x.h>
#include <Arduino_LPS22HB.h>
#include <SparkFun_u-blox_GNSS_v3.h>

#define GRAVITY 9.80665  // Conversion factor from g to m/s²
#define BUFF_SIZE 128

// Sensor and GPS instances
SFE_UBLOX_GNSS myGNSS;  // Create an instance for the u-blox GNSS module

char buffer[BUFF_SIZE];

// Function to read and print GNSS data
void checkAndPrintGNSSData()
{
  myGNSS.checkUblox();  // Poll the GNSS

  long latitude = myGNSS.getLatitude();
  long longitude = myGNSS.getLongitude();
  byte SIV = myGNSS.getSIV();
  unsigned long timestamp = millis();
  snprintf(buffer, BUFF_SIZE, "GPS,%ld,%ld,%d,%lu", latitude, longitude, SIV, timestamp);
  Serial.println(buffer);
}

// Function to read and print LPS22HB data
void checkAndPrintLPS22HBData()
{
  float pressure = BARO.readPressure();
  float temperature = BARO.readTemperature();
  float altitude = BARO.readAltitude();
  unsigned long timestamp = millis();
  snprintf(buffer, BUFF_SIZE, "LPS22,%.5f,%.5f,%.5f,%lu", pressure, temperature, altitude, timestamp);
  Serial.println(buffer);
}

// Function to read and print BMI270 data (accelerometer and gyroscope)
void checkAndPrintBMI270Data()
{
  float xAccel, yAccel, zAccel;
  float xGyro, yGyro, zGyro;
  unsigned long timestamp = millis();

  if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable())
  {
    IMU.readAcceleration(xAccel, yAccel, zAccel);
    IMU.readGyroscope(xGyro, yGyro, zGyro);

    // Convert g to m/s²
    xAccel *= GRAVITY;
    yAccel *= GRAVITY;
    zAccel *= GRAVITY;

    snprintf(buffer, BUFF_SIZE, "BMI270,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%lu", xAccel, yAccel, zAccel, xGyro, yGyro, zGyro, timestamp);
    Serial.println(buffer);
  }
}

// Function to read and print magnetometer data from IMU
void checkAndPrintIMUMagnetometer()
{
  float x, y, z;
  unsigned long timestamp = millis();
  if (IMU.magneticFieldAvailable())
  {
    IMU.readMagneticField(x, y, z);
    snprintf(buffer, BUFF_SIZE, "BMM150,%.5f,%.5f,%.5f,%lu", x, y, z, timestamp);
    Serial.println(buffer);
  }
}

// Function to read and print temperature and humidity data from HS300x sensor
void checkAndPrintHS300xData()
{
  float temperature = HS300x.readTemperature();
  float humidity = HS300x.readHumidity();
  unsigned long timestamp = millis();
  snprintf(buffer, BUFF_SIZE, "HS300x,%.5f,%.5f,%lu", temperature, humidity, timestamp);
  Serial.println(buffer);
}

// Initial setup function
void setup()
{
  Serial.begin(115200);

  // Initialize GNSS
  Wire.begin();
  if (myGNSS.begin() == false) {
    Serial.println("u-blox GNSS not detected at default I2C address. Freezing.");
    while (1);
  }

  myGNSS.setI2COutput(COM_TYPE_UBX); // Set the I2C port to output UBX only
  myGNSS.setNavigationFrequency(40);  // Produce 40 solutions per second
  myGNSS.setAutoPVT(true, false);    // Auto PVT mode

  // Initialize Barometric Pressure and Temperature Sensor (LPS22HB)
  if (!BARO.begin()) {
    Serial.println("Failed to init LPS22HB!");
    while (1);
  }
  BARO.setOutputRate(RATE_75_HZ); // Set the output rate to 75 Hz

  // Initialize IMU for Accelerometer, Gyroscope, and Magnetometer
  if (!IMU.begin()) {
    Serial.println("Failed to init IMU!");
    while (1);
  }
  else {
    snprintf(buffer, BUFF_SIZE, "Gyro sample rate = %.2f Hz", IMU.gyroscopeSampleRate());
    Serial.println(buffer);

    snprintf(buffer, BUFF_SIZE, "Accelerometer sample rate = %.2f Hz", IMU.accelerationSampleRate());
    Serial.println(buffer);

    snprintf(buffer, BUFF_SIZE, "Magnetometer sample rate = %.2f Hz", IMU.magneticFieldSampleRate());
    Serial.println(buffer);

  }
  

  // Initialize Humidity and Temperature Sensor (HS300x)
  if (!HS300x.begin()) {
    Serial.println("Failed to init HS300x!");
    while (1);
  }

  Serial.println("All sensors init success");
}

// Main program loop
void loop()
{
  checkAndPrintBMI270Data(); 
  checkAndPrintIMUMagnetometer();
  checkAndPrintGNSSData();
  checkAndPrintLPS22HBData();
  checkAndPrintHS300xData();
}
