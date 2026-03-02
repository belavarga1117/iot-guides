const WebSocket = require('ws');
const TOKEN = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiIyODZjNDQ0OWE2OGM0N2FlYTcyMDhiMjE1NGFiZGU2MCIsImlhdCI6MTc3MjQ0ODk1NCwiZXhwIjoyMDg3ODA4OTU0fQ.PS9pd6Dj8qJKxOEAdoKWL_sVz9EvZLBgxCgv7pyr-nc';
const ws = new WebSocket('ws://192.168.1.111:8123/api/websocket');
let msgId = 1;

const TO_DELETE = [
  'c6f30fd90c79341f3c2274c8a7131f6a',
  'ef0cd75e0b16b6a4cb6d09d5477243f9',
];

ws.on('message', (data) => {
  const msg = JSON.parse(data);
  if (msg.type === 'auth_required') {
    ws.send(JSON.stringify({type: 'auth', access_token: TOKEN}));
  } else if (msg.type === 'auth_ok') {
    TO_DELETE.forEach(deviceId => {
      const id = msgId++;
      console.log('Deleting', deviceId, 'as id', id);
      ws.send(JSON.stringify({id, type: 'config/device_registry/remove', device_id: deviceId}));
    });
  } else if (msg.type === 'result') {
    console.log('Result id', msg.id, '| success:', msg.success, '| error:', msg.error || 'none');
    if (msg.id >= TO_DELETE.length) {
      ws.close(); process.exit(0);
    }
  }
});

ws.on('error', e => { console.error(e.message); process.exit(1); });
setTimeout(() => { console.log('timeout'); ws.close(); process.exit(1); }, 15000);
