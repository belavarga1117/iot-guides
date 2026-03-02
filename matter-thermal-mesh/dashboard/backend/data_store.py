"""
data_store.py — In-memory store for sensor data and mesh topology.

Thread-safe ring buffer for thermal frames and mesh snapshots.
The dashboard reads from here via WebSocket.
"""

import time
import threading
from dataclasses import dataclass, field


@dataclass(frozen=True)
class SensorReading:
    """Immutable snapshot of a single node's sensor data."""
    node_id: str
    room: str
    temperature: float
    occupied: bool
    person_count: int
    timestamp: float


@dataclass(frozen=True)
class MeshLink:
    """Immutable representation of a Thread mesh link between two nodes."""
    source_rloc16: str
    target_rloc16: str
    link_quality: int   # 0-3 (Thread link quality indicator)
    rssi: int           # dBm


@dataclass(frozen=True)
class MeshNode:
    """Immutable representation of a Thread mesh node."""
    rloc16: str
    role: str           # "router", "end-device", "leader", "border-router"
    ext_address: str
    node_id: str = ""   # Mapped from config if known


@dataclass(frozen=True)
class MeshSnapshot:
    """Immutable snapshot of the full Thread mesh topology."""
    nodes: tuple
    links: tuple
    timestamp: float


class DataStore:
    """Thread-safe in-memory data store for the dashboard."""

    def __init__(self, max_history=60):
        self._lock = threading.Lock()
        self._max_history = max_history

        # Current sensor readings per node
        self._current_readings: dict[str, SensorReading] = {}

        # History ring buffer per node (for motion trails)
        self._history: dict[str, list[SensorReading]] = {}

        # Latest mesh topology
        self._mesh: MeshSnapshot | None = None

    def update_sensor(self, reading: SensorReading) -> None:
        """Store a new sensor reading (immutable — creates new state)."""
        with self._lock:
            node_id = reading.node_id

            # Update current
            self._current_readings = {
                **self._current_readings,
                node_id: reading
            }

            # Append to history (trim to max)
            history = list(self._history.get(node_id, []))
            history.append(reading)
            if len(history) > self._max_history:
                history = history[-self._max_history:]

            self._history = {
                **self._history,
                node_id: history
            }

    def update_mesh(self, snapshot: MeshSnapshot) -> None:
        """Store a new mesh topology snapshot."""
        with self._lock:
            self._mesh = snapshot

    def get_current_readings(self) -> dict[str, SensorReading]:
        """Get latest reading per node."""
        with self._lock:
            return dict(self._current_readings)

    def get_history(self, node_id: str, count: int = 10) -> list[SensorReading]:
        """Get recent history for a node (for motion trails)."""
        with self._lock:
            history = self._history.get(node_id, [])
            return list(history[-count:])

    def get_mesh(self) -> MeshSnapshot | None:
        """Get latest mesh topology."""
        with self._lock:
            return self._mesh

    def get_dashboard_state(self) -> dict:
        """Get full dashboard state as a serializable dict."""
        with self._lock:
            nodes_data = {}
            for node_id, reading in self._current_readings.items():
                nodes_data[node_id] = {
                    "node_id": reading.node_id,
                    "room": reading.room,
                    "temperature": reading.temperature,
                    "occupied": reading.occupied,
                    "person_count": reading.person_count,
                    "timestamp": reading.timestamp,
                }

            mesh_data = None
            if self._mesh:
                mesh_data = {
                    "nodes": [
                        {
                            "rloc16": n.rloc16,
                            "role": n.role,
                            "ext_address": n.ext_address,
                            "node_id": n.node_id,
                        }
                        for n in self._mesh.nodes
                    ],
                    "links": [
                        {
                            "source": lnk.source_rloc16,
                            "target": lnk.target_rloc16,
                            "link_quality": lnk.link_quality,
                            "rssi": lnk.rssi,
                        }
                        for lnk in self._mesh.links
                    ],
                    "timestamp": self._mesh.timestamp,
                }

            return {
                "sensors": nodes_data,
                "mesh": mesh_data,
                "server_time": time.time(),
            }
