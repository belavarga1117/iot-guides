const WebSocket = require('ws');
const TOKEN = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiIyODZjNDQ0OWE2OGM0N2FlYTcyMDhiMjE1NGFiZGU2MCIsImlhdCI6MTc3MjQ0ODk1NCwiZXhwIjoyMDg3ODA4OTU0fQ.PS9pd6Dj8qJKxOEAdoKWL_sVz9EvZLBgxCgv7pyr-nc';
const ws = new WebSocket('ws://192.168.1.111:8123/api/websocket');
let id = 1;

ws.on('message', (data) => {
  const msg = JSON.parse(data);
  if (msg.type === 'auth_required') {
    ws.send(JSON.stringify({type: 'auth', access_token: TOKEN}));
  } else if (msg.type === 'auth_ok') {
    ws.send(JSON.stringify({id: id++, type: 'config/device_registry/list'}));
  } else if (msg.id === 1 && msg.result) {
    const devices = msg.result;
    const matter = devices.filter(d => d.identifiers && d.identifiers.some(i => i[0] === 'matter'));
    console.log('Matter devices:', matter.length);
    matter.forEach(d => {
      console.log(JSON.stringify({
        id: d.id,
        name: d.name_by_user || d.name,
        disabled_by: d.disabled_by
      }));
    });
    ws.close(); process.exit(0);
  }
});
ws.on('error', e => { console.error(e.message); process.exit(1); });
setTimeout(() => { ws.close(); process.exit(1); }, 15000);
