// @ts-check
const { test, expect } = require('@playwright/test');
const path = require('path');

const SCREENSHOTS_DIR = path.join(__dirname, '../../../docs/screenshots');
const DASHBOARD_URL = 'http://smartpi.local:5000';

test.describe('Flask Dashboard Screenshots', () => {

  test('01 - Full Dashboard', async ({ page }) => {
    await page.goto(DASHBOARD_URL);
    await page.waitForLoadState('networkidle');
    await page.waitForTimeout(3000); // Let D3.js render

    await page.screenshot({
      path: path.join(SCREENSHOTS_DIR, '15-flask-heatmap.png'),
      fullPage: true,
    });
  });

  test('02 - Mesh Topology Section', async ({ page }) => {
    await page.goto(DASHBOARD_URL);
    await page.waitForLoadState('networkidle');
    await page.waitForTimeout(3000);

    // Scroll to mesh topology section
    const meshSection = page.locator('#mesh-topology, .mesh-section, [data-section="mesh"]');
    if (await meshSection.count() > 0) {
      await meshSection.scrollIntoViewIfNeeded();
      await page.waitForTimeout(1000);
      await meshSection.screenshot({
        path: path.join(SCREENSHOTS_DIR, '16-flask-mesh-topology.png'),
      });
    } else {
      // Fallback: screenshot the bottom half of the page
      await page.screenshot({
        path: path.join(SCREENSHOTS_DIR, '16-flask-mesh-topology.png'),
        fullPage: false,
        clip: { x: 0, y: 400, width: 1280, height: 400 },
      });
    }
  });

  test('03 - Floor Plan Section', async ({ page }) => {
    await page.goto(DASHBOARD_URL);
    await page.waitForLoadState('networkidle');
    await page.waitForTimeout(3000);

    // Scroll to floor plan section
    const floorSection = page.locator('#floor-plan, .floorplan-section, [data-section="floorplan"]');
    if (await floorSection.count() > 0) {
      await floorSection.scrollIntoViewIfNeeded();
      await page.waitForTimeout(1000);
      await floorSection.screenshot({
        path: path.join(SCREENSHOTS_DIR, '17-flask-floorplan.png'),
      });
    } else {
      await page.screenshot({
        path: path.join(SCREENSHOTS_DIR, '17-flask-floorplan.png'),
        fullPage: false,
        clip: { x: 0, y: 0, width: 1280, height: 400 },
      });
    }
  });

});
