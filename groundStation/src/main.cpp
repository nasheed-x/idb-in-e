#include <Arduino.h>
#include <RadioLib.h>
#include <SparkFun_u-blox_GNSS_v3.h>

// Define pins for the RFM95 LoRa module
#define NSS_PIN 4   // Chip select pin
#define DIO0_PIN 3  // DIO0 pin used for interrupt and transmit/receive status
#define RESET_PIN 2 // Reset pin

// Create an instance of the RFM95 module
RFM95 radio = new Module(NSS_PIN, DIO0_PIN, RESET_PIN);

// Sensor and GPS instances
SFE_UBLOX_GNSS myGNSS; // Create an instance for the u-blox GNSS module

// Timing variable for GPS update interval
unsigned long lastGPSUpdate = 0;
const unsigned long gpsUpdateInterval = 5000; // 5 seconds in milliseconds

void setup()
{
  Serial.begin(9600);
  while (!Serial)
    ; // Wait for serial port to connect

  Wire.begin();
  if (myGNSS.begin() == false)
  {
    Serial.println("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing.");
    while (1)
      ;
  }
  myGNSS.setI2COutput(COM_TYPE_UBX);
  myGNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT);

  // Initialize RFM95 with specific settings
  int state = radio.begin(915.0, 62.5, 12, 8, RADIOLIB_SX127X_SYNC_WORD, 20, 8, 0);
  if (state != RADIOLIB_ERR_NONE)
  {
    Serial.println(F("RFM 95 initialization failed!"));
    while (1)
      ; // Stay in the loop forever if the radio initialization failed
  }

  Serial.println(F("RFM 95 Receiver ready!"));
}

void loop() {
  // Check and print GPS position every 5 seconds
  if (millis() - lastGPSUpdate >= gpsUpdateInterval) {
    lastGPSUpdate = millis(); // Update the last GPS update time

    if (myGNSS.getPVT())
    {
      double lat = myGNSS.getLatitude();  // Current latitude in degrees
      double lon = myGNSS.getLongitude(); // Current longitude in degrees
      int siv = myGNSS.getSIV();                 // Satellites in View

      // Print current GPS coordinates in the specified format
      Serial.print(F("Received packet: [7;"));
      Serial.print(lat);
      Serial.print(F(","));
      Serial.print(lon);
      Serial.print(F(","));
      Serial.print(siv);
      Serial.println(F("]"));
    }
    else
    {
      Serial.println(F("Waiting for GPS fix..."));
    }
  }

  // Continue to receive and process packets
  String receivedData;
  int state = radio.receive(receivedData);
  if (state == RADIOLIB_ERR_NONE) {
    // Successfully received a packet
    Serial.print(F("Received packet: "));
    Serial.println(receivedData);

    // Get RSSI and SNR
    float rssi = radio.getRSSI();
    float snr = radio.getSNR();

    // Print RSSI and SNR in the exact specified format
    Serial.print(F("Received packet: [8;"));
    Serial.print(rssi);
    Serial.print(F(","));
    Serial.print(snr);
    Serial.println(F("]"));
  } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    // Print error if no packet received
    Serial.println(F("No packet received, RX timeout."));
  } else {
    // Print any other error
    Serial.print(F("Receive failed, error code: "));
    Serial.println(state);
  }
}
