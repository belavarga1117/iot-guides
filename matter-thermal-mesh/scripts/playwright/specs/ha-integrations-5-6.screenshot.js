// @ts-check
const { test } = require('@playwright/test');
const path = require('path');

const SCREENSHOTS_DIR = path.join(__dirname, '../../../docs/screenshots/05-ha-setup');
const HA_URL = 'http://smartpi.local:8123';

async function loginToHA(page) {
  await page.goto(HA_URL);
  await page.waitForTimeout(5000);
  const u = page.locator('input[name="username"]');
  if (await u.isVisible({ timeout: 3000 }).catch(() => false)) {
    await u.fill('smartpi');
    await page.locator('input[name="password"]').fill('smartpi');
    await page.getByRole('button', { name: 'Log in' }).click();
    await page.waitForTimeout(4000);
  }
}

test('5.6 - Integrations page showing Matter, OTBR, Thread', async ({ page }) => {
  await loginToHA(page);
  await page.goto(`${HA_URL}/config/integrations`);
  await page.waitForTimeout(5000);

  // HA uses Shadow DOM — window/document scroll doesn't work.
  // Use mouse wheel over the main content area to scroll down to Matter/OTBR/Thread cards.
  await page.mouse.move(640, 400);
  await page.mouse.wheel(0, 800);  // scroll down past Discovered section
  await page.waitForTimeout(800);
  await page.mouse.wheel(0, 400);  // scroll a bit more to center on Matter row
  await page.waitForTimeout(500);

  await page.screenshot({
    path: path.join(SCREENSHOTS_DIR, '16-ha-all-integrations.png'),
    fullPage: false,
  });
});
