#include <Arduino.h>
#include <RadioLib.h>
#include <ArduinoBLE.h>

// Define pins for the RFM95 LoRa module
#define NSS_PIN 4
#define DIO0_PIN 3
#define RESET_PIN 2

// Create an instance of the RFM95 module
RFM95 radio = new Module(NSS_PIN, DIO0_PIN, RESET_PIN);

// BLE Service and Characteristic UUIDs
#define SERVICE_UUID "12345678-1234-1234-1234-123456789ABC"
#define CHARACTERISTIC_UUID "ABCDEFAB-ABCD-ABCD-ABCD-ABCDEFABCDEF"

// Create BLE Service and Characteristic
BLEService dataService(SERVICE_UUID);
BLEStringCharacteristic dataCharacteristic(CHARACTERISTIC_UUID, BLENotify, 512); // Max 512 bytes

void setup() {
  Serial.begin(9600);
  while (!Serial);  // Wait for serial port to connect

  // Initialize RFM95 with specific settings
  int state = radio.begin(915.0, 62.5, 12, 8, RADIOLIB_SX127X_SYNC_WORD, 20, 8, 0);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.println(F("RFM 95 initialization failed!"));
    while (1);  // Stay in the loop forever if the radio initialization failed
  }
  Serial.println(F("RFM 95 Receiver ready!"));

  // Initialize BLE
  if (!BLE.begin()) {
    Serial.println("Failed to initialize BLE!");
    while (1);
  }

  // Get BLE address
  String address = BLE.address();
  Serial.print("BLE address = ");
  Serial.println(address);

  // Construct a unique local name using the last two bytes of the BLE address
  String name = "BLE_LoRa_Receiver-";
  name += address.substring(address.length() - 5, address.length() - 3);
  name += address.substring(address.length() - 2, address.length());

  Serial.print("Local name set to: ");
  Serial.println(name);

  // Set BLE device and local names
  BLE.setLocalName(name.c_str());
  BLE.setDeviceName(name.c_str());
  BLE.setAdvertisedService(dataService);

  // Add characteristic
  dataService.addCharacteristic(dataCharacteristic);

  // Add service
  BLE.addService(dataService);

  // Start advertising
  BLE.advertise();
  Serial.println("BLE advertising started");
}

void loop() {
  if (BLE.connected()) {
    String receivedData;
    int state = radio.receive(receivedData);

    if (state == RADIOLIB_ERR_NONE) {
      // Successfully received a packet from LoRa
      Serial.print("Received packet via LoRa: ");
      Serial.println(receivedData);

      // Transmitting received data over BLE
      dataCharacteristic.writeValue(receivedData);
      Serial.print("Transmitted over BLE: ");
      Serial.println(receivedData);
    } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
      Serial.println("No packet received, RX timeout.");
    } else {
      // Print any other error
      Serial.print("Receive failed, error code: ");
      Serial.println(state);
    }
  }
  delay(1000); // Delay to slow down the loop
}
