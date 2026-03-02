const WebSocket = require('ws');
const TOKEN = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiIyODZjNDQ0OWE2OGM0N2FlYTcyMDhiMjE1NGFiZGU2MCIsImlhdCI6MTc3MjQ0ODk1NCwiZXhwIjoyMDg3ODA4OTU0fQ.PS9pd6Dj8qJKxOEAdoKWL_sVz9EvZLBgxCgv7pyr-nc';
const ws = new WebSocket('ws://192.168.1.111:8123/api/websocket');
let msgId = 1;
let devices = [];
let entities = [];
let states = [];

function send(obj) { obj.id = msgId++; ws.send(JSON.stringify(obj)); return obj.id; }

ws.on('message', (data) => {
  const msg = JSON.parse(data);
  if (msg.type === 'auth_required') {
    ws.send(JSON.stringify({type: 'auth', access_token: TOKEN}));
  } else if (msg.type === 'auth_ok') {
    send({type: 'config/device_registry/list'});     // id=1
    send({type: 'config/entity_registry/list'});      // id=2
    send({type: 'get_states'});                       // id=3
  } else if (msg.id === 1 && msg.result) {
    devices = msg.result.filter(d => d.identifiers && d.identifiers.some(i => i[0] === 'matter'));
    tryPrint();
  } else if (msg.id === 2 && msg.result) {
    entities = msg.result;
    tryPrint();
  } else if (msg.id === 3 && msg.result) {
    states = msg.result;
    tryPrint();
  }
});

function tryPrint() {
  if (!devices.length || !entities.length || !states.length) return;
  
  const stateMap = {};
  states.forEach(s => { stateMap[s.entity_id] = s.state; });
  
  devices.forEach(d => {
    const devEntities = entities.filter(e => e.device_id === d.id);
    const tempEntity = devEntities.find(e => e.entity_id.includes('temperature'));
    const humEntity = devEntities.find(e => e.entity_id.includes('humidity'));
    const temp = tempEntity ? stateMap[tempEntity.entity_id] : 'n/a';
    const hum = humEntity ? stateMap[humEntity.entity_id] : null;
    console.log(`${d.id} | "${d.name_by_user || d.name}" | temp: ${temp}${hum ? ' | hum: '+hum+'%' : ''}`);
  });
  ws.close(); process.exit(0);
}

ws.on('error', e => { console.error(e.message); process.exit(1); });
setTimeout(() => { ws.close(); process.exit(1); }, 15000);
