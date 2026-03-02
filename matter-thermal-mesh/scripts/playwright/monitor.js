const WebSocket = require('ws');

const MS_WS  = 'ws://smartpi.local:5580/ws';
const HA_WS  = 'ws://smartpi.local:8123/api/websocket';
const HA_TOK = 'YOUR_HA_LONG_LIVED_TOKEN';

const NODE_NAMES = { 15: 'SparkFun #2 (06 Szoba)', 16: 'SparkFun #1 (Nappali)', 17: 'Nano Matter (07 Szoba)' };
const nodeState = {};

function ts() { return new Date().toLocaleTimeString('hu-HU'); }
function log(msg) { process.stdout.write(`[${ts()}] ${msg}\n`); }

// ── matter-server: watch node availability ────────────────────────────────────
const ms = new WebSocket(MS_WS);
ms.on('open', () => {
  ms.send(JSON.stringify({ message_id: 'nodes', command: 'get_nodes' }));
});
ms.on('message', data => {
  const m = JSON.parse(data);
  // Initial state
  if (m.message_id === 'nodes' && m.result) {
    m.result.forEach(n => {
      nodeState[n.node_id] = n.available;
      const name = NODE_NAMES[n.node_id] || `node ${n.node_id}`;
      log(`INIT  ${name}: ${n.available ? '🟢 online' : '🔴 offline'}`);
    });
    log('--- Monitoring started — unplug the devices ---');
  }
  // Events (node_added, node_updated, etc.)
  if (m.event) {
    const ev = m.event;
    if (ev.type === 'node_updated' && ev.data) {
      const n = ev.data;
      const name = NODE_NAMES[n.node_id] || `node ${n.node_id}`;
      const wasAvail = nodeState[n.node_id];
      if (n.available !== wasAvail) {
        nodeState[n.node_id] = n.available;
        log(`${n.available ? '🟢 ONLINE ' : '🔴 OFFLINE'} ${name}`);
      }
    }
  }
});
ms.on('error', e => log('matter-server WS error: ' + e.message));
ms.on('close', () => log('matter-server WS closed'));

// ── HA: watch occupancy state changes ────────────────────────────────────────
const OCCUPANCY_IDS = {
  'binary_sensor.matter_device_occupancy':   'SparkFun #2',
  'binary_sensor.matter_device_occupancy_5': 'Nano Matter',
  'binary_sensor.matter_device_occupancy_4': 'SparkFun #1',
};
const ha = new WebSocket(HA_WS);
let haMid = 1;
ha.on('message', data => {
  const m = JSON.parse(data);
  if (m.type === 'auth_required') ha.send(JSON.stringify({ type: 'auth', access_token: HA_TOK }));
  else if (m.type === 'auth_ok') ha.send(JSON.stringify({ id: haMid++, type: 'subscribe_events', event_type: 'state_changed' }));
  else if (m.type === 'event' && m.event?.data?.new_state) {
    const eid = m.event.data.entity_id;
    const name = OCCUPANCY_IDS[eid];
    if (name) {
      const state = m.event.data.new_state.state;
      if (state === 'on')  log(`👤 DETECTED  ${name}`);
      if (state === 'off') log(`   cleared   ${name}`);
    }
  }
});
ha.on('error', e => log('HA WS error: ' + e.message));
