// @ts-check
const { test, expect } = require('@playwright/test');
const path = require('path');

const SCREENSHOTS_DIR = path.join(__dirname, '../../../docs/screenshots/07-commissioning');
const HA_URL = 'http://192.168.1.111:8123';
const HA_USER = 'smartpi';
const HA_PASS = 'smartpi';

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

test.describe('Clean screenshots — project-only views', () => {

  test('01 - Matter integration with 3 devices', async ({ page }) => {
    await loginToHA(page);
    // Direct link to Matter integration — shows ONLY Matter devices
    await page.goto(`${HA_URL}/config/integrations/integration/matter`);
    await page.waitForTimeout(5000);
    await page.screenshot({
      path: path.join(SCREENSHOTS_DIR, '10-matter-3-devices.png'),
      fullPage: false,
    });
  });

  test('02 - Devices filtered by Matter', async ({ page }) => {
    await loginToHA(page);
    // Devices page filtered to Matter only
    await page.goto(`${HA_URL}/config/devices/dashboard?domain=matter`);
    await page.waitForTimeout(5000);
    await page.screenshot({
      path: path.join(SCREENSHOTS_DIR, '11-devices-matter-only.png'),
      fullPage: false,
    });
  });

  test('03 - Entities filtered by Matter', async ({ page }) => {
    await loginToHA(page);
    // Entities filtered to Matter
    await page.goto(`${HA_URL}/config/entities?domain=matter`);
    await page.waitForTimeout(5000);
    await page.screenshot({
      path: path.join(SCREENSHOTS_DIR, '12-entities-matter-only.png'),
      fullPage: false,
    });
  });

  test('04 - Individual device pages', async ({ page }) => {
    await loginToHA(page);
    // Go to Matter integration, click first device to see its detail
    await page.goto(`${HA_URL}/config/integrations/integration/matter`);
    await page.waitForTimeout(3000);

    // Click the first device (5 entities = SparkFun #1)
    await page.mouse.click(640, 335);
    await page.waitForTimeout(3000);
    await page.screenshot({
      path: path.join(SCREENSHOTS_DIR, '13-device-node1-detail.png'),
      fullPage: false,
    });
  });

});
