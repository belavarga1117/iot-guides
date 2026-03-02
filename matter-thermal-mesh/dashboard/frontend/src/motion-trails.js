/**
 * motion-trails.js — Motion Trail Effects for Presence History
 *
 * Shows fading trails of where people were detected,
 * creating a visual "heat ghost" effect on the floor plan.
 * Older readings fade out, recent ones glow brighter.
 */

const MotionTrails = (() => {
  let canvas, ctx;
  const trails = {};  // nodeId → array of { x, y, timestamp, occupied }
  const MAX_TRAIL_LENGTH = 30;
  const TRAIL_FADE_MS = 30000;  // 30 seconds to fully fade

  function init() {
    canvas = document.getElementById("heatmap-canvas");
    ctx = canvas.getContext("2d");
    requestAnimationFrame(renderLoop);
  }

  function addPoint(nodeId, occupied, nodeCfg) {
    if (!trails[nodeId]) {
      trails[nodeId] = [];
    }

    const trail = trails[nodeId];
    trail.push({
      x: nodeCfg.floor_x,
      y: nodeCfg.floor_y,
      timestamp: Date.now(),
      occupied,
    });

    // Trim old entries
    if (trail.length > MAX_TRAIL_LENGTH) {
      trails[nodeId] = trail.slice(-MAX_TRAIL_LENGTH);
    }
  }

  function renderLoop() {
    // Don't clear — the heatmap.js render() handles that.
    // We draw trails AFTER heatmap blobs.
    renderTrails();
    requestAnimationFrame(renderLoop);
  }

  function renderTrails() {
    if (!canvas || !ctx) return;

    const now = Date.now();

    for (const [nodeId, trail] of Object.entries(trails)) {
      for (let i = 0; i < trail.length; i++) {
        const point = trail[i];
        if (!point.occupied) continue;

        const age = now - point.timestamp;
        if (age > TRAIL_FADE_MS) continue;

        const opacity = 1 - (age / TRAIL_FADE_MS);
        const cx = point.x * canvas.width;
        const cy = point.y * canvas.height;
        const radius = 8 + (1 - opacity) * 20;  // Grows as it fades

        ctx.beginPath();
        ctx.arc(cx, cy, radius, 0, Math.PI * 2);
        ctx.fillStyle = `rgba(255, 68, 68, ${opacity * 0.15})`;
        ctx.fill();
      }
    }
  }

  return { init, addPoint };
})();
