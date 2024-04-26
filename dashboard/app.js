const fs = require('fs');
const fastcsv = require('fast-csv');
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

// Get current date and time formatted as YYYYMMDD-HHMMSS
const now = new Date();
const dateStr = now.toISOString().substring(0, 19).replace(/[^0-9]/g, "");
const filename = `sensor_data_${dateStr}.csv`;

// CSV setup
const csvStream = fastcsv.format({ headers: true });
const writableStream = fs.createWriteStream(filename, { flags: 'a' }); // 'a' for append
csvStream.pipe(writableStream);

app.use(express.static('public'));

wss.on('connection', (ws) => {
  console.log('Client connected');

  parser.on('data', (data) => {
    console.log('Raw data received:', data);

    try {
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

      const formattedData = {};
      dataLabels[sensorId].forEach((label, index) => {
        formattedData[label] = values[index];
      });

      // Add timestamp
      const timestamp = new Date().toISOString();
      formattedData.timestamp = timestamp;

      // Write the formatted data with timestamp to CSV
      csvStream.write(formattedData);

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
  // Logic to determine sensor ID (based on your application's requirements)
  const keys = Object.keys(data);
  if (keys.includes('Latitude') && keys.includes('Longitude')) {
    return 1;
  } else if (keys.includes('Pressure') && keys.includes('Temperature') && keys.includes('Altitude')) {
    return 2;
  } else if (keys.includes('X-Acceleration') && keys.includes('Y-Acceleration') && keys.includes('Z-Acceleration')) {
    return 3;
  } else if (keys.includes('X-Gyro') && keys.includes('Y-Gyro')) {
    return 4;
  } else if (keys.includes('X-Magnet') && keys.includes('Y-Magnet')) {
    return 5;
  } else if (keys.includes('Temperature') && keys.includes('Humidity')) {
    return 6;
  }
  return null; // Sensor ID could not be determined
}

server.listen(3000, () => {
  console.log('Server running on port 3000');
});

process.on('SIGINT', () => {
  console.log('Received SIGINT. Closing CSV stream and server.');
  csvStream.end();
  server.close(() => {
    console.log('Server closed');
    process.exit(0);
  });
});