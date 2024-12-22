#include <ArduinoBLE.h>
#include <Arduino_LPS22HB.h>
#include <Arduino_BMI270_BMM150.h>
#include <Arduino_HS300x.h>

#define GRAVITY 9.80665  // Conversion factor from g to m/s²

// Define BLE UUIDs
#define BLE_SENSE_UUID(val) ("6fbe1da7-" val "-44de-92c4-bb6e04fb0212")

const int VERSION = 0x00000000;

// BLE Service and Characteristics
BLEService                     service                       (BLE_SENSE_UUID("0000"));
BLEUnsignedIntCharacteristic   versionCharacteristic         (BLE_SENSE_UUID("1001"), BLERead);
BLEFloatCharacteristic         pressureCharacteristic        (BLE_SENSE_UUID("4001"), BLERead); // Float, kPa
BLEFloatCharacteristic         temperatureCharacteristic     (BLE_SENSE_UUID("4002"), BLERead); // Float, Celsius
BLEFloatCharacteristic         humidityCharacteristic        (BLE_SENSE_UUID("4003"), BLERead); // Float, Percentage
BLECharacteristic              rgbLedCharacteristic          (BLE_SENSE_UUID("6001"), BLERead | BLEWrite, 3 * sizeof(byte)); // Array of 3 bytes, RGB
BLECharacteristic              accelerationCharacteristic    (BLE_SENSE_UUID("3001"), BLENotify, 3 * sizeof(float)); // Array of 3 floats, G
BLECharacteristic              gyroscopeCharacteristic       (BLE_SENSE_UUID("3002"), BLENotify, 3 * sizeof(float)); // Array of 3 floats, dps
BLECharacteristic              magneticFieldCharacteristic   (BLE_SENSE_UUID("3003"), BLENotify, 3 * sizeof(float)); // Array of 3 floats, uT

// Global Variables
String name;
char buffer[128];

// Sensor instances
// LPS22HB BARO;
// HS300x HS300xSensor;
// BMI270_BMM150 IMU;

void setup() {
  Serial.begin(115200);

  // Initialize Sensors
  if (!BARO.begin()) {
    Serial.println("Failed to initialize LPS22HB!");
    while (1);
  }

  if (!HS300x.begin()) {
    Serial.println("Failed to initialize HS300x!");
    while (1);
  }

  if (!IMU.begin()) {
    Serial.println("Failed to initialize BMI270 and BMM150!");
    while (1);
  }

  // Initialize BLE
  if (!BLE.begin()) {
    Serial.println("Failed to initialize BLE!");
    while (1);
  }

  // Set up BLE Device
  String address = BLE.address();
  address.toUpperCase();
  name = "BLESense-";
  name += address[address.length() - 5];
  name += address[address.length() - 4];
  name += address[address.length() - 2];
  name += address[address.length() - 1];
  BLE.setLocalName(name.c_str());
  BLE.setDeviceName(name.c_str());
  BLE.setAdvertisedService(service);

  // Add characteristics to the service
  service.addCharacteristic(versionCharacteristic);
  service.addCharacteristic(pressureCharacteristic);
  service.addCharacteristic(temperatureCharacteristic);
  service.addCharacteristic(humidityCharacteristic);
  service.addCharacteristic(rgbLedCharacteristic);
  service.addCharacteristic(accelerationCharacteristic);
  service.addCharacteristic(gyroscopeCharacteristic);
  service.addCharacteristic(magneticFieldCharacteristic);

  versionCharacteristic.setValue(VERSION);
  pressureCharacteristic.setEventHandler(BLERead, onPressureCharacteristicRead);
  temperatureCharacteristic.setEventHandler(BLERead, onTemperatureCharacteristicRead);
  humidityCharacteristic.setEventHandler(BLERead, onHumidityCharacteristicRead);
  rgbLedCharacteristic.setEventHandler(BLEWritten, onRgbLedCharacteristicWrite);

  BLE.addService(service);
  BLE.advertise();

  Serial.println("Setup complete");
}

void loop() {
  BLEDevice central = BLE.central();
  if (central) {
    Serial.println("Connected to central device");
    while (central.connected()) {
      checkAndNotifySensors();
    }
    Serial.println("Disconnected from central device");
  }
}

void checkAndNotifySensors() {
  // LPS22HB - Pressure and Temperature
  if (pressureCharacteristic.subscribed()) {
    float pressure = BARO.readPressure();
    pressureCharacteristic.writeValue(pressure);
  }

  if (temperatureCharacteristic.subscribed()) {
    float temperature = HS300x.readTemperature();
    temperatureCharacteristic.writeValue(temperature);
  }

  // HS300x - Humidity
  if (humidityCharacteristic.subscribed()) {
    float humidity = HS300x.readHumidity();
    humidityCharacteristic.writeValue(humidity);
  }

  // BMI270 - Accelerometer and Gyroscope
  if (IMU.accelerationAvailable() && accelerationCharacteristic.subscribed()) {
    float x, y, z;
    IMU.readAcceleration(x, y, z);
    float acceleration[3] = { x * GRAVITY, y * GRAVITY, z * GRAVITY };
    accelerationCharacteristic.writeValue(acceleration, sizeof(acceleration));
  }

  if (IMU.gyroscopeAvailable() && gyroscopeCharacteristic.subscribed()) {
    float x, y, z;
    IMU.readGyroscope(x, y, z);
    float gyroscope[3] = { x, y, z };
    gyroscopeCharacteristic.writeValue(gyroscope, sizeof(gyroscope));
  }

  // BMM150 - Magnetometer
  if (IMU.magneticFieldAvailable() && magneticFieldCharacteristic.subscribed()) {
    float x, y, z;
    IMU.readMagneticField(x, y, z);
    float magneticField[3] = { x, y, z };
    magneticFieldCharacteristic.writeValue(magneticField, sizeof(magneticField));
  }
}

void onPressureCharacteristicRead(BLEDevice central, BLECharacteristic characteristic) {
  float pressure = BARO.readPressure();
  pressureCharacteristic.writeValue(pressure);
}

void onTemperatureCharacteristicRead(BLEDevice central, BLECharacteristic characteristic) {
  float temperature = HS300x.readTemperature();
  temperatureCharacteristic.writeValue(temperature);
}

void onHumidityCharacteristicRead(BLEDevice central, BLECharacteristic characteristic) {
  float humidity = HS300x.readHumidity();
  humidityCharacteristic.writeValue(humidity);
}

void onRgbLedCharacteristicWrite(BLEDevice central, BLECharacteristic characteristic) {
  byte r = rgbLedCharacteristic[0];
  byte g = rgbLedCharacteristic[1];
  byte b = rgbLedCharacteristic[2];

  setLedPinValue(LEDR, r);
  setLedPinValue(LEDG, g);
  setLedPinValue(LEDB, b);
}

void setLedPinValue(int pin, int value) {
  if (value == 0) {
    analogWrite(pin, 256);
  } else {
    analogWrite(pin, 255 - value);
  }
}
