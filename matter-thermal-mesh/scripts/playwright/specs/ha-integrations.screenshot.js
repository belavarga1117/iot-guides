// @ts-check
const { test, expect } = require('@playwright/test');
const path = require('path');

const SCREENSHOTS_DIR = path.join(__dirname, '../../../docs/screenshots');
const HA_URL = 'http://smartpi.local:8123';
const HA_USER = 'smartpi';
const HA_PASS = 'smartpi';

/**
 * Helper: Login to HA and return authenticated page
 */
async function loginToHA(page) {
  await page.goto(HA_URL);
  await page.waitForLoadState('networkidle');
  await page.fill('input[name="username"]', HA_USER);
  await page.fill('input[name="password"]', HA_PASS);
  await page.click('button[type="submit"]');
  await page.waitForLoadState('networkidle');
  await page.waitForTimeout(2000);
}

test.describe('HA Integration Screenshots', () => {

  test('01 - Integrations Page', async ({ page }) => {
    await loginToHA(page);
    await page.goto(`${HA_URL}/config/integrations`);
    await page.waitForLoadState('networkidle');
    await page.waitForTimeout(2000);
    await page.screenshot({
      path: path.join(SCREENSHOTS_DIR, '06-ha-integrations.png'),
      fullPage: false,
    });
  });

  test('02 - Matter Integration', async ({ page }) => {
    await loginToHA(page);
    await page.goto(`${HA_URL}/config/integrations/integration/matter`);
    await page.waitForLoadState('networkidle');
    await page.waitForTimeout(2000);
    await page.screenshot({
      path: path.join(SCREENSHOTS_DIR, '07-ha-matter-setup.png'),
      fullPage: false,
    });
  });

  test('03 - OTBR Integration', async ({ page }) => {
    await loginToHA(page);
    await page.goto(`${HA_URL}/config/integrations/integration/otbr`);
    await page.waitForLoadState('networkidle');
    await page.waitForTimeout(2000);
    await page.screenshot({
      path: path.join(SCREENSHOTS_DIR, '08-ha-otbr-setup.png'),
      fullPage: false,
    });
  });

  test('04 - Entities List', async ({ page }) => {
    await loginToHA(page);
    await page.goto(`${HA_URL}/config/entities`);
    await page.waitForLoadState('networkidle');
    await page.waitForTimeout(2000);
    await page.screenshot({
      path: path.join(SCREENSHOTS_DIR, '10-ha-entities-list.png'),
      fullPage: false,
    });
  });

});
