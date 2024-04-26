#include <Arduino.h>
#include <RadioLib.h>
#include <SparkFun_u-blox_GNSS_v3.h>
#include <math.h>

// Define pins for the RFM95 LoRa module
#define NSS_PIN 4   // Chip select pin
#define DIO0_PIN 3  // DIO0 pin used for interrupt and transmit/receive status
#define RESET_PIN 2 // Reset pin

// Create an instance of the RFM95 module
RFM95 radio = new Module(NSS_PIN, DIO0_PIN, RESET_PIN);

// Sensor and GPS instances
SFE_UBLOX_GNSS myGNSS; // Create an instance for the u-blox GNSS module

// Function to calculate Haversine distance between two points
double haversine(double lat1, double lon1, double lat2, double lon2)
{
  const double R = 6371e3; // Earth radius in meters
  double phi1 = lat1 * (M_PI / 180);
  double phi2 = lat2 * (M_PI / 180);
  double deltaPhi = (lat2 - lat1) * (M_PI / 180);
  double deltaLambda = (lon2 - lon1) * (M_PI / 180);

  double a = sin(deltaPhi / 2) * sin(deltaPhi / 2) +
             cos(phi1) * cos(phi2) *
                 sin(deltaLambda / 2) * sin(deltaLambda / 2);
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));

  double distance = R * c; // Distance in meters
  return distance;
}

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

void loop()
{
  String receivedData;
  int state = radio.receive(receivedData);

  // Declare variables to store GPS coordinates
  double lat1 = 0, lon1 = 0;
  bool hasGPSFix = false;  // Flag to check GPS fix status

  // Check if new GPS data is available
  if (myGNSS.getPVT())
  {
    lat1 = myGNSS.getLatitude() * 1e-7;  // Current latitude in degrees
    lon1 = myGNSS.getLongitude() * 1e-7; // Current longitude in degrees
    hasGPSFix = true;  // Set GPS fix flag

    // Print current GPS coordinates
    Serial.print(F("Current GPS Coordinates - Latitude: "));
    Serial.print(lat1, 7);
    Serial.print(F(", Longitude: "));
    Serial.println(lon1, 7);
  }
  else
  {
    Serial.println(F("Waiting for GPS fix..."));
  }

  if (state == RADIOLIB_ERR_NONE)
  {
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

    // Parsing the packet
    int sensorId;
    double lat2, lon2;
    int siv;
    sscanf(receivedData.c_str(), "[%d;%lf,%lf, %d]", &sensorId, &lat2, &lon2, &siv);

    // Convert received lat/lon from microdegrees to degrees
    lat2 *= 1e-7;
    lon2 *= 1e-7;

    if (hasGPSFix)  // Ensure you have a current GPS fix to calculate distance
    {
      // Calculate distance
      double distance = haversine(lat1, lon1, lat2, lon2);
      Serial.print(F("Distance to Rocket "));
      Serial.print(sensorId);
      Serial.print(F(": "));
      Serial.print(distance);
      Serial.println(F(" meters"));
    }
  }
  else if (state == RADIOLIB_ERR_RX_TIMEOUT)
  {
    // Print error if no packet received
    Serial.println(F("No packet received, RX timeout."));
  }
  else
  {
    // Print any other error
    Serial.print(F("Receive failed, error code: "));
    Serial.println(state);
  }
}