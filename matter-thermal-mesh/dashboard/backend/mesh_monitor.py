"""
mesh_monitor.py — Thread Mesh Topology Monitor via OTBR REST API.

Queries the OpenThread Border Router REST API for:
- Active mesh nodes (routers, end devices)
- Link quality between nodes
- Mesh topology changes (self-healing detection)

OTBR REST API runs on the RPi at http://localhost:8081
"""

import time
import logging
import requests

from data_store import DataStore, MeshSnapshot, MeshNode, MeshLink

logger = logging.getLogger(__name__)


class MeshMonitor:
    """Polls OTBR REST API and updates the data store with mesh topology."""

    def __init__(self, store: DataStore, otbr_url: str = "http://localhost:8081"):
        self._store = store
        self._otbr_url = otbr_url.rstrip("/")
        self._previous_node_count = 0

    def poll(self) -> MeshSnapshot | None:
        """Fetch current mesh topology from OTBR and store it."""
        try:
            snapshot = self._fetch_topology()
            if snapshot:
                self._store.update_mesh(snapshot)
                self._detect_changes(snapshot)
            return snapshot
        except requests.ConnectionError:
            logger.warning("Cannot reach OTBR at %s", self._otbr_url)
            return None
        except Exception as e:
            logger.error("Mesh monitor error: %s", e)
            return None

    def _fetch_topology(self) -> MeshSnapshot | None:
        """Query OTBR REST API for mesh diagnostic data."""
        # OTBR diagnostic endpoint
        resp = requests.get(
            f"{self._otbr_url}/diagnostics",
            headers={"Accept": "application/json"},
            timeout=5,
        )

        if resp.status_code != 200:
            logger.warning("OTBR returned %d", resp.status_code)
            return None

        diag_data = resp.json()
        nodes = []
        links = []

        for entry in diag_data:
            rloc16 = entry.get("Rloc16", 0)
            rloc_hex = f"0x{rloc16:04X}"
            ext_addr = entry.get("ExtAddress", "")
            role = self._parse_role(entry.get("Mode", {}))

            nodes.append(MeshNode(
                rloc16=rloc_hex,
                role=role,
                ext_address=ext_addr,
            ))

            # Parse route table for links
            for route in entry.get("Route", {}).get("RouteData", []):
                target_rloc = route.get("RouteId", 0) << 10
                target_hex = f"0x{target_rloc:04X}"
                link_quality = route.get("LinkQualityIn", 0)

                links.append(MeshLink(
                    source_rloc16=rloc_hex,
                    target_rloc16=target_hex,
                    link_quality=link_quality,
                    rssi=route.get("RouteCost", 0),
                ))

            # Parse child table for end devices
            for child in entry.get("ChildTable", []):
                child_rloc = child.get("Rloc16", 0)
                child_hex = f"0x{child_rloc:04X}"

                nodes.append(MeshNode(
                    rloc16=child_hex,
                    role="end-device",
                    ext_address=child.get("ExtAddress", ""),
                ))

                links.append(MeshLink(
                    source_rloc16=rloc_hex,
                    target_rloc16=child_hex,
                    link_quality=child.get("LinkQualityIn", 0),
                    rssi=0,
                ))

        # Deduplicate nodes by rloc16
        seen = set()
        unique_nodes = []
        for node in nodes:
            if node.rloc16 not in seen:
                seen.add(node.rloc16)
                unique_nodes.append(node)

        return MeshSnapshot(
            nodes=tuple(unique_nodes),
            links=tuple(links),
            timestamp=time.time(),
        )

    def _parse_role(self, mode: dict) -> str:
        """Determine node role from OTBR mode flags."""
        if mode.get("DeviceType", False):
            return "router"
        return "end-device"

    def _detect_changes(self, snapshot: MeshSnapshot) -> None:
        """Log mesh topology changes (for self-healing demo)."""
        current_count = len(snapshot.nodes)
        if self._previous_node_count > 0 and current_count != self._previous_node_count:
            diff = current_count - self._previous_node_count
            if diff > 0:
                logger.info("MESH: +%d node(s) joined! Total: %d", diff, current_count)
            else:
                logger.info("MESH: %d node(s) left! Total: %d", diff, current_count)
        self._previous_node_count = current_count

    def get_node_list_url(self) -> str:
        """Return the OTBR node list URL for debugging."""
        return f"{self._otbr_url}/node"
