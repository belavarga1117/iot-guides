const WebSocket = require('ws');
const TOKEN = 'YOUR_HA_LONG_LIVED_TOKEN';
const ws = new WebSocket('ws://smartpi.local:8123/api/websocket');
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
