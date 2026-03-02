"""
app.py — Smart Home Matter Starter Dashboard Server

Flask + SocketIO server that:
1. Polls Home Assistant API for sensor data (temperature, occupancy)
2. Polls OTBR REST API for Thread mesh topology
3. Pushes updates to the D3.js frontend via WebSocket

Run: python app.py
"""

import os
import time
import logging
import threading

import yaml
import requests
from flask import Flask, send_from_directory, jsonify
from flask_socketio import SocketIO
from flask_cors import CORS

from data_store import DataStore, SensorReading
from mesh_monitor import MeshMonitor

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(name)s] %(levelname)s: %(message)s",
)
logger = logging.getLogger("matter-thermal-mesh")

# Load config
config_path = os.path.join(os.path.dirname(__file__), "config.yaml")
with open(config_path, "r") as f:
    config = yaml.safe_load(f)

# Flask app
app = Flask(
    __name__,
    static_folder="../frontend/public",
    static_url_path="",
)
CORS(app)
socketio = SocketIO(app, cors_allowed_origins="*", async_mode="eventlet")

# Data store
store = DataStore(max_history=60)

# Mesh monitor
otbr_url = config["otbr"]["url"]
mesh_monitor = MeshMonitor(store, otbr_url)

# HA config
ha_url = config["homeassistant"]["url"]
ha_token = os.environ.get("HA_TOKEN", config["homeassistant"]["token"])


# --- Routes ---

@app.route("/")
def index():
    return send_from_directory("../frontend/public", "index.html")

@app.route("/api/state")
def api_state():
    """REST endpoint for full dashboard state."""
    return jsonify(store.get_dashboard_state())

@app.route("/api/config")
def api_config():
    """Return dashboard layout config to frontend."""
    return jsonify({
        "nodes": config["nodes"],
        "floorplan": config["floorplan"],
        "heatmap": config["heatmap"],
    })

@app.route("/api/mesh")
def api_mesh():
    """REST endpoint for mesh topology."""
    mesh = store.get_mesh()
    if mesh is None:
        return jsonify({"error": "No mesh data yet"}), 503
    state = store.get_dashboard_state()
    return jsonify(state["mesh"])


# --- WebSocket Events ---

@socketio.on("connect")
def handle_connect():
    logger.info("Client connected")
    socketio.emit("state", store.get_dashboard_state())

@socketio.on("disconnect")
def handle_disconnect():
    logger.info("Client disconnected")


# --- Background Pollers ---

def poll_homeassistant():
    """Poll Home Assistant API for sensor entity states."""
    interval = config["homeassistant"]["poll_interval_sec"]
    headers = {
        "Authorization": f"Bearer {ha_token}",
        "Content-Type": "application/json",
    }

    while True:
        try:
            if not ha_token:
                time.sleep(interval)
                continue

            for node_cfg in config["nodes"]:
                node_id = node_cfg["id"]
                room = node_cfg["room"]

                # Fetch temperature
                temp_resp = requests.get(
                    f"{ha_url}/api/states/{node_cfg['ha_temp_entity']}",
                    headers=headers,
                    timeout=5,
                )
                temperature = 0.0
                if temp_resp.status_code == 200:
                    state_val = temp_resp.json().get("state", "0")
                    try:
                        temperature = float(state_val)
                    except ValueError:
                        temperature = 0.0

                # Fetch occupancy
                occ_resp = requests.get(
                    f"{ha_url}/api/states/{node_cfg['ha_occupancy_entity']}",
                    headers=headers,
                    timeout=5,
                )
                occupied = False
                if occ_resp.status_code == 200:
                    occ_state = occ_resp.json().get("state", "off")
                    occupied = occ_state in ("on", "true", "1")

                reading = SensorReading(
                    node_id=node_id,
                    room=room,
                    temperature=temperature,
                    occupied=occupied,
                    person_count=1 if occupied else 0,
                    timestamp=time.time(),
                )
                store.update_sensor(reading)

            # Push to all connected WebSocket clients
            socketio.emit("state", store.get_dashboard_state())

        except requests.ConnectionError:
            logger.warning("Cannot reach Home Assistant at %s", ha_url)
        except Exception as e:
            logger.error("HA poll error: %s", e)

        time.sleep(interval)


def poll_mesh():
    """Poll OTBR REST API for mesh topology."""
    interval = config["otbr"]["poll_interval_sec"]

    while True:
        snapshot = mesh_monitor.poll()
        if snapshot:
            socketio.emit("mesh", store.get_dashboard_state()["mesh"])
        time.sleep(interval)


# --- Demo Mode (no HA needed) ---

def demo_mode():
    """Generate fake sensor data for dashboard development without hardware."""
    import random
    logger.info("DEMO MODE — generating fake sensor data")

    while True:
        for node_cfg in config["nodes"]:
            base_temp = 22.0 + random.uniform(-1, 1)
            occupied = random.random() > 0.5
            max_temp = base_temp + (random.uniform(3, 8) if occupied else 0)

            reading = SensorReading(
                node_id=node_cfg["id"],
                room=node_cfg["room"],
                temperature=round(max_temp, 1),
                occupied=occupied,
                person_count=random.randint(1, 2) if occupied else 0,
                timestamp=time.time(),
            )
            store.update_sensor(reading)

        socketio.emit("state", store.get_dashboard_state())
        time.sleep(2)


# --- Main ---

if __name__ == "__main__":
    server_config = config["server"]

    use_demo = os.environ.get("DEMO_MODE", "false").lower() == "true"

    if use_demo:
        t = threading.Thread(target=demo_mode, daemon=True)
        t.start()
    else:
        # Start HA poller
        t1 = threading.Thread(target=poll_homeassistant, daemon=True)
        t1.start()

    # Start mesh poller
    t2 = threading.Thread(target=poll_mesh, daemon=True)
    t2.start()

    logger.info("Dashboard server starting on %s:%d",
                server_config["host"], server_config["port"])
    if use_demo:
        logger.info("Running in DEMO mode (fake data)")

    socketio.run(
        app,
        host=server_config["host"],
        port=server_config["port"],
        debug=server_config["debug"],
    )
