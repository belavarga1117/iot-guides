const WebSocket = require('ws');
const TOKEN = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiIyODZjNDQ0OWE2OGM0N2FlYTcyMDhiMjE1NGFiZGU2MCIsImlhdCI6MTc3MjQ0ODk1NCwiZXhwIjoyMDg3ODA4OTU0fQ.PS9pd6Dj8qJKxOEAdoKWL_sVz9EvZLBgxCgv7pyr-nc';
const ws = new WebSocket('ws://192.168.1.111:8123/api/websocket');
let msgId = 1;

// Old unavailable devices to delete
const TO_DELETE = [
  'c6f30fd90c79341f3c2274c8a7131f6a',  // old Hallway Sensor
  'ef0cd75e0b16b6a4cb6d09d5477243f9',  // old Living Room Sensor
];

// New devices to identify (check identifiers for matter node_id)
const NEW_DEVICES = [
  {id: 'fad14e617d780e75e56f8cc3f0f6c9e9'},
  {id: 'dd3ce98ad1544a8e165b6252e71c25ff'},
  {id: '2dcdf74d67d57f414ba227b6de77931b'},
];

function send(obj) { obj.id = msgId++; ws.send(JSON.stringify(obj)); return obj.id; }

let pendingDeletes = 0;
let deleteResults = [];

ws.on('message', (data) => {
  const msg = JSON.parse(data);
  if (msg.type === 'auth_required') {
    ws.send(JSON.stringify({type: 'auth', access_token: TOKEN}));
  } else if (msg.type === 'auth_ok') {
    // First get device list to see identifiers
    send({type: 'config/device_registry/list'});
  } else if (msg.id === 1 && msg.result) {
    // Print identifiers for new devices
    const allDevices = msg.result;
    console.log('\n=== New device identifiers (matter node IDs) ===');
    NEW_DEVICES.forEach(nd => {
      const d = allDevices.find(x => x.id === nd.id);
      if (d) console.log(nd.id.slice(0,8), '| identifiers:', JSON.stringify(d.identifiers));
    });
    
    // Delete old devices
    console.log('\n=== Deleting old devices ===');
    pendingDeletes = TO_DELETE.length;
    TO_DELETE.forEach(deviceId => {
      const rid = send({type: 'config/device_registry/remove', device_id: deviceId});
      console.log('Sent delete for', deviceId.slice(0,8), '(request id:', rid, ')');
    });
  } else if (msg.id >= 2 && msg.result !== undefined) {
    deleteResults.push(msg);
    console.log('Delete response id', msg.id, ':', JSON.stringify(msg.result));
    if (deleteResults.length >= pendingDeletes) {
      console.log('\nDone!');
      ws.close(); process.exit(0);
    }
  }
});

ws.on('error', e => { console.error(e.message); process.exit(1); });
setTimeout(() => { ws.close(); process.exit(1); }, 20000);
