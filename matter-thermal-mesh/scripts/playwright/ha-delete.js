const WebSocket = require('ws');
const TOKEN = 'YOUR_HA_LONG_LIVED_TOKEN';
const ws = new WebSocket('ws://smartpi.local:8123/api/websocket');
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
