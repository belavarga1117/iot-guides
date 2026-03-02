/**
 * heatmap.js — D3.js Thermal Heatmap Renderer
 *
 * Renders 8×8 Grid-EYE data as smooth colored blobs on the canvas.
 * Uses bilinear interpolation and the "inferno" colormap.
 */

const HeatMap = (() => {
  let canvas, ctx;
  let config = {};
  const nodeData = {};

  // Inferno colormap (sampled at 10 stops)
  const INFERNO = [
    [0, 0, 4],       // 0.0 — near black
    [40, 11, 84],     // 0.1
    [101, 21, 110],   // 0.2
    [159, 42, 99],    // 0.3
    [202, 70, 67],    // 0.4
    [237, 105, 37],   // 0.5
    [251, 155, 6],    // 0.6
    [247, 209, 60],   // 0.7
    [252, 254, 164],  // 0.8
    [252, 255, 164],  // 0.9
  ];

  function init(heatmapConfig) {
    config = heatmapConfig;
    canvas = document.getElementById("heatmap-canvas");
    ctx = canvas.getContext("2d");
    resize();
    window.addEventListener("resize", resize);
  }

  function resize() {
    const container = canvas.parentElement;
    canvas.width = container.clientWidth;
    canvas.height = container.clientHeight;
  }

  function updateNode(nodeId, temperature, occupied, nodeCfg) {
    nodeData[nodeId] = {
      temperature,
      occupied,
      x: nodeCfg.floor_x,
      y: nodeCfg.floor_y,
      timestamp: Date.now(),
    };
    render();
  }

  function render() {
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    for (const [nodeId, data] of Object.entries(nodeData)) {
      if (!data.occupied) continue;

      const cx = data.x * canvas.width;
      const cy = data.y * canvas.height;
      const radius = 60;

      // Draw radial gradient blob for occupied nodes
      const gradient = ctx.createRadialGradient(cx, cy, 0, cx, cy, radius);
      const color = tempToColor(data.temperature);
      const colorStr = `rgb(${color[0]}, ${color[1]}, ${color[2]})`;

      gradient.addColorStop(0, colorStr);
      gradient.addColorStop(0.3, colorStr.replace("rgb", "rgba").replace(")", ", 0.6)"));
      gradient.addColorStop(1, "rgba(0, 0, 0, 0)");

      ctx.fillStyle = gradient;
      ctx.beginPath();
      ctx.arc(cx, cy, radius, 0, Math.PI * 2);
      ctx.fill();
    }
  }

  function tempToColor(temp) {
    const tMin = config.temp_min || 18;
    const tMax = config.temp_max || 35;
    const t = Math.max(0, Math.min(1, (temp - tMin) / (tMax - tMin)));

    const idx = t * (INFERNO.length - 1);
    const lower = Math.floor(idx);
    const upper = Math.min(lower + 1, INFERNO.length - 1);
    const frac = idx - lower;

    return [
      Math.round(INFERNO[lower][0] + (INFERNO[upper][0] - INFERNO[lower][0]) * frac),
      Math.round(INFERNO[lower][1] + (INFERNO[upper][1] - INFERNO[lower][1]) * frac),
      Math.round(INFERNO[lower][2] + (INFERNO[upper][2] - INFERNO[lower][2]) * frac),
    ];
  }

  return { init, updateNode, render };
})();
