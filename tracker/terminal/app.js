const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');
const express = require('express');
const http = require('http');
const WebSocket = require('ws');

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

const port = new SerialPort({
  path: '/dev/cu.usbmodem2',
  baudRate: 9600,
});

const parser = port.pipe(new ReadlineParser({ delimiter: '\n' }));

app.use(express.static('public'));

wss.on('connection', (ws) => {
  console.log('Client connected');

  parser.on('data', (data) => {
    console.log('Raw data received:', data); // Log the raw data received

    try {
        // Remove the 'Sent packet: ' prefix and parse the remaining string
        const trimmedData = data.trim();
        const packet = trimmedData.replace('Sent packet: [', '').replace(']', '');
        const parts = packet.split(';');
        const sensorId = parseInt(parts[0], 10);
        const values = parts[1].split(',').map(v => parseFloat(v));

        const dataLabels = {
            1: ["Latitude", "Longitude", "SIV"],
            2: ["Pressure", "Temperature", "Altitude"],
            3: ["X-Acceleration", "Y-Acceleration", "Z-Acceleration"],
            4: ["X-Gyro", "Y-Gyro", "Z-Gyro"],
            5: ["X-Magnet", "Y-Magnet", "Z-Magnet"],
            6: ["Temperature", "Humidity"]
        };

        if (!dataLabels[sensorId]) {
            throw new Error(`Unsupported sensor ID: ${sensorId}`);
        }

        // Match the values array with their corresponding labels
        const formattedData = {};
        dataLabels[sensorId].forEach((label, index) => {
            formattedData[label] = values[index];
        });

        // Send the formatted data to the WebSocket client
        ws.send(JSON.stringify({ sensorId: sensorId, data: formattedData }));
    } catch (error) {
        console.error('Error parsing data:', error);
        ws.send(JSON.stringify({ error: 'Error parsing data: ' + error.message }));
    }
});

  ws.on('close', () => {
    console.log('Client disconnected');
  });
});

function determineSensorId(data) {
  // Define logic to infer sensor ID based on the keys in the data object
  const keys = Object.keys(data);
  if (keys.includes('Latitude') && keys.includes('Longitude')) {
    return 1; // GPS
  } else if (keys.includes('Pressure') && keys.includes('Temperature') && keys.includes('Altitude')) {
    return 2; // Barometric
  } else if (keys.includes('X-Acceleration') && keys.includes('Y-Acceleration')) {
    return 3; // IMU Accelerometer
  } else if (keys.includes('X-Gyro') && keys.includes('Y-Gyro')) {
    return 4; // IMU Gyroscope
  } else if (keys.includes('X-Magnet') && keys.includes('Y-Magnet')) {
    return 5; // IMU Magnetometer
  } else if (keys.includes('Temperature') && keys.includes('Humidity')) {
    return 6; // Temperature & Humidity
  }
  return null; // Sensor ID could not be determined
}

server.listen(3000, () => {
  console.log('Server running on port 3000');
});