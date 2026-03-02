// @ts-check
const { test, expect } = require('@playwright/test');
const path = require('path');

const SCREENSHOTS_DIR = path.join(__dirname, '../../../docs/screenshots/07-commissioning');
const HA_URL = 'http://192.168.1.111:8123';
const HA_USER = 'smartpi';
const HA_PASS = 'smartpi';

/**
 * Login to HA via the browser login page.
 * Handles both "already logged in" and "needs login" states.
 */
async function loginToHA(page) {
  await page.goto(HA_URL);
  await page.waitForTimeout(5000);
  const usernameInput = page.locator('input[name="username"]');
  if (await usernameInput.isVisible({ timeout: 3000 }).catch(() => false)) {
    await usernameInput.fill(HA_USER);
    await page.locator('input[name="password"]').fill(HA_PASS);
    await page.getByRole('button', { name: 'Log in' }).click();
    await page.waitForTimeout(4000);
  }
}

test.describe('Part 7 — Final screenshots (renamed devices)', () => {

  test('01 - Matter integration with 3 renamed devices', async ({ page }) => {
    await loginToHA(page);
    // Matter integration page — shows all 3 devices with user-assigned names
    await page.goto(`${HA_URL}/config/integrations/integration/matter`);
    await page.waitForTimeout(5000);
    await page.screenshot({
      path: path.join(SCREENSHOTS_DIR, '14-matter-3-renamed.png'),
      fullPage: false,
    });
  });

  test('02 - Living Room Sensor detail (SparkFun #1, 5 entities)', async ({ page }) => {
    await loginToHA(page);
    // Navigate to device page for SparkFun #1 (Living Room Sensor)
    await page.goto(`${HA_URL}/config/devices/device/ef0cd75e0b16b6a4cb6d09d5477243f9`);
    await page.waitForTimeout(5000);
    await page.screenshot({
      path: path.join(SCREENSHOTS_DIR, '15-device-livingroom-detail.png'),
      fullPage: false,
    });
  });

  test('03 - Bedroom Sensor detail (SparkFun #2, 3 entities)', async ({ page }) => {
    await loginToHA(page);
    // Navigate to device page for SparkFun #2 (Bedroom Sensor)
    await page.goto(`${HA_URL}/config/devices/device/d5078f16890f5bd73a1dbaf8412c021c`);
    await page.waitForTimeout(5000);
    await page.screenshot({
      path: path.join(SCREENSHOTS_DIR, '16-device-bedroom-detail.png'),
      fullPage: false,
    });
  });

  test('04 - Hallway Sensor detail (Nano Matter, 3 entities)', async ({ page }) => {
    await loginToHA(page);
    // Navigate to device page for Nano Matter (Hallway Sensor)
    await page.goto(`${HA_URL}/config/devices/device/c6f30fd90c79341f3c2274c8a7131f6a`);
    await page.waitForTimeout(5000);
    await page.screenshot({
      path: path.join(SCREENSHOTS_DIR, '17-device-hallway-detail.png'),
      fullPage: false,
    });
  });

  test('05 - Overview dashboard with all sensors', async ({ page }) => {
    await loginToHA(page);
    // Overview dashboard — shows all sensor cards
    await page.goto(`${HA_URL}/lovelace/0`);
    await page.waitForTimeout(5000);
    await page.screenshot({
      path: path.join(SCREENSHOTS_DIR, '18-dashboard-overview.png'),
      fullPage: false,
    });
  });

});
