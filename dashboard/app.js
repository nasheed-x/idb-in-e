const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');
const express = require('express');
const http = require('http');
const WebSocket = require('ws');

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

const port = new SerialPort({
  path: '/dev/cu.usbmodem1101',
  baudRate: 9600
});

const parser = port.pipe(new ReadlineParser({ delimiter: '\n' }));

let clients = [];  // This will store all connected WebSocket clients

app.use(express.static('public'));

wss.on('connection', (ws) => {
  clients.push(ws);  // Add the new client to the list of clients
  console.log('Client connected');

  ws.on('close', () => {
    clients = clients.filter(client => client !== ws);  // Remove the client from the list on disconnect
    console.log('Client disconnected');
  });
});

parser.on('data', (data) => {
  console.log('Raw data received:', data);
  const trimmedData = data.trim();

  // Skip processing if the line is a timeout or does not contain a valid packet.
  if (!trimmedData.startsWith('Received packet:') || trimmedData.includes('No packet received, RX timeout')) {
    console.log('Skipping non-packet data or timeout:', trimmedData);
    return; // This will exit the function before any parsing attempts
  }

  // Process the packet
  let packetContent = trimmedData.replace('Received packet: [', '').replace(']', '');
  try {
    const parts = packetContent.split(';');
    if (parts.length < 2) {
      console.error('Malformed packet:', packetContent);
      return; // Skip if the packet does not have at least two parts
    }

    const sensorId = parseInt(parts[0], 10);
    const values = parts[1].split(',').map(parseFloat);
    console.log(`Parsed sensor ID: ${sensorId}, Values: ${values.join(', ')}`);

    const dataLabels = {
      1: ["Latitude", "Longitude", "SIV"],
      2: ["Pressure", "Temperature", "Altitude"],
      3: ["X-Acceleration", "Y-Acceleration", "Z-Acceleration"],
      4: ["X-Gyro", "Y-Gyro", "Z-Gyro"],
      5: ["X-Magnet", "Y-Magnet", "Z-Magnet"],
      6: ["Temperature", "Humidity"],
      7: ["Latitude", "Longitude", "SIV"],
      8: ["RSSI", "SNR"]
    };

    const formattedData = {};
    dataLabels[sensorId]?.forEach((label, index) => {
      formattedData[label] = values[index];
    });
    console.log(`Formatted data for sensor ID ${sensorId}:`, formattedData);

    // Broadcast the formatted sensor data to all connected WebSocket clients
    clients.forEach(client => {
      client.send(JSON.stringify({ type: 'sensor', sensorId: sensorId, data: formattedData }));
    });
  } catch (error) {
    console.error('Error parsing data:', error);
    clients.forEach(client => {
      client.send(JSON.stringify({ type: 'error', message: 'Error parsing data: ' + error.message }));
    });
  }
});

server.listen(3000, () => {
  console.log('Server running on port 3000');
});

process.on('SIGINT', () => {
  console.log('Received SIGINT. Server closing.');
  server.close(() => {
    console.log('Server closed');
    process.exit(0);
  });
});
