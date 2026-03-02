#!/usr/bin/env node
/**
 * capture-demo.js — HeatSense dashboard demo recorder
 *
 * Runs a scripted walkthrough (no HA, no hardware, fully mocked).
 * Outputs a GIF for docs/README and an MP4 for LinkedIn/slides.
 *
 * Usage (from project root):
 *   node scripts/playwright/capture-demo.js
 *
 * Requires:
 *   - playwright already installed (scripts/playwright/node_modules)
 *   - ffmpeg: brew install ffmpeg
 *
 * Output:
 *   docs/screenshots/dashboard-demo.gif   ← embed in MacDown / GitHub
 *   docs/screenshots/dashboard-demo.mp4   ← LinkedIn / keynote
 */

const { chromium } = require('playwright');
const { execSync }  = require('child_process');
const path          = require('path');
const fs            = require('fs');

// ── Paths ──────────────────────────────────────────────────────────────────────
const ROOT      = path.resolve(__dirname, '../../');
const DASHBOARD = path.join(ROOT, 'dashboard/index.html');
const OUT_DIR   = path.join(ROOT, 'docs/screenshots');
const GIF_OUT   = path.join(OUT_DIR, 'dashboard-demo.gif');
const MP4_OUT   = path.join(OUT_DIR, 'dashboard-demo.mp4');
const FRAMES    = '/tmp/heatsense-frames';

// ── Video settings ─────────────────────────────────────────────────────────────
// 1280×720 = standard 16:9, good for LinkedIn / keynote
// GIF scaled to 900px wide (Retina-readable, file stays < 10MB)
const VW  = 1280;
const VH  = 720;
const FPS = 20;
const MS  = 1000 / FPS;

// ── Scripted sequence ──────────────────────────────────────────────────────────
// [sensorId | null, ms]
// null = no sensor active (corridor / empty)
// 'lr' = Living Room (SparkFun #1), 'r2' = Bedroom 1 (SparkFun #2), 'br' = Bedroom 2 (Nano)
const SEQ = [
  [null,  385],   // cold start — all rooms empty
  ['r2', 1800],   // person in Bedroom 1 (left)
  [null, 1400],   // walks to corridor
  ['lr', 1800],   // enters Living Room
  [null, 1230],   // back to corridor
  ['r2', 1800],   // back in Bedroom 1 (left)
  [null, 1400],   // walks to corridor
  ['br', 1800],   // enters Bedroom 2 (right)
  [null,  620],   // end
];

// ── WebSocket mock ─────────────────────────────────────────────────────────────
// Injected before any page script — prevents real HA connection.
// Also patches requestAnimationFrame → setTimeout so the draw loop runs
// in headless Chromium (rAF is throttled in headless, setTimeout is not).
const MOCK = `(function () {
  // Patch rAF so the canvas animation loop actually runs in headless mode
  window.requestAnimationFrame = function (cb) { return setTimeout(cb, 8); };
  window.cancelAnimationFrame  = function (id) { clearTimeout(id); };

  window.WebSocket = class MockWS extends EventTarget {
    constructor() {
      super();
      this.readyState = 1;
      setTimeout(() => { if (this.onopen) this.onopen({ type: 'open' }); }, 80);
    }
    send() {}
    close() {}
  };
  window.__setOcc = function (sensorId) {
    if (typeof occ === 'undefined') return;
    SENSORS.forEach(s => { occ[s.id] = 'off'; });
    if (sensorId !== null) occ[sensorId] = 'on';
    if (typeof updatePersonTarget === 'function') updatePersonTarget();
  };
})();`;

// ── Helpers ────────────────────────────────────────────────────────────────────
async function frames(page, count, ref) {
  for (let i = 0; i < count; i++) {
    const n = String(ref.n++).padStart(6, '0');
    await page.screenshot({ path: `${FRAMES}/f${n}.png` });
    await page.waitForTimeout(MS);
  }
}

// ── Main ───────────────────────────────────────────────────────────────────────
(async () => {
  // Prep dirs
  if (fs.existsSync(FRAMES)) fs.rmSync(FRAMES, { recursive: true });
  fs.mkdirSync(FRAMES);
  fs.mkdirSync(OUT_DIR, { recursive: true });

  const browser = await chromium.launch({ headless: true });
  const page    = await browser.newPage();
  await page.setViewportSize({ width: VW, height: VH });
  await page.addInitScript(MOCK);

  await page.goto(`file://${DASHBOARD}`);
  await page.waitForSelector('canvas');
  await page.waitForTimeout(400);

  // Init: all off + pre-position person at starting room (Bedroom 1)
  await page.evaluate(() => {
    SENSORS.forEach(s => { occ[s.id] = 'off'; });
    // Teleport person to Bedroom 1 so they don't walk in from corridor on first appearance
    const start = ROOM_CENTERS['r2'];
    person.x  = start.x;
    person.y  = start.y;
    person.tx = start.x;
    person.ty = start.y;
  });

  const ref = { n: 0 };

  for (const [id, ms] of SEQ) {
    await page.evaluate(id => window.__setOcc(id), id);
    await frames(page, Math.ceil(ms / MS), ref);
  }

  await browser.close();
  console.log(`Captured ${ref.n} frames (${(ref.n / FPS).toFixed(1)}s).`);

  // ── MP4 ───────────────────────────────────────────────────────────────────────
  console.log('\nEncoding MP4...');
  execSync(
    `ffmpeg -y -framerate ${FPS} -pattern_type glob -i '${FRAMES}/f*.png' ` +
    `-c:v libx264 -pix_fmt yuv420p -crf 18 -preset slow "${MP4_OUT}"`,
    { stdio: 'inherit' }
  );

  // ── GIF ───────────────────────────────────────────────────────────────────────
  // Two-pass palette, scaled to 900px wide, dithered for quality
  console.log('\nEncoding GIF...');
  execSync(
    `ffmpeg -y -i "${MP4_OUT}" ` +
    `-vf "fps=${FPS},scale=900:-1:flags=lanczos,split[s0][s1];[s0]palettegen=max_colors=128[p];[s1][p]paletteuse=dither=bayer" ` +
    `"${GIF_OUT}"`,
    { stdio: 'inherit' }
  );

  console.log('\n✓ Done');
  console.log(`  MP4 → ${MP4_OUT}`);
  console.log(`  GIF → ${GIF_OUT}`);
  console.log('\nMarkdown embed:');
  console.log('  ![HeatSense](docs/screenshots/dashboard-demo.gif)');
})();
