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

  // Initialize RFM95 with specific settings
  int state = radio.begin(915.0, 62.5, 12, 8, RADIOLIB_SX127X_SYNC_WORD, 20, 8, 0);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.println(F("RFM 95 initialization failed!"));
    while (1);  // Stay in the loop forever if the radio initialization failed
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

    // Get RSSI and SNR
    float rssi = radio.getRSSI();
    float snr = radio.getSNR();

    // Print RSSI and SNR
    Serial.print(F("RSSI: "));
    Serial.print(rssi);
    Serial.print(F(" dBm, SNR: "));
    Serial.print(snr);
    Serial.println(F(" dB"));
  } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    // Print error if no packet received
    Serial.println(F("No packet received, RX timeout."));
  } else {
    // Print any other error
    Serial.print(F("Receive failed, error code: "));
    Serial.println(state);
  }
}
