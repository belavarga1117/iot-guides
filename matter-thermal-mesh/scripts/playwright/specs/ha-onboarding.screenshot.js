// @ts-check
const { test, expect } = require('@playwright/test');
const path = require('path');

const SCREENSHOTS_DIR = path.join(__dirname, '../../../docs/screenshots/05-ha-setup');
const HA_URL = 'http://192.168.1.111:8123';

// Standard credentials from project rules
const HA_NAME = 'Smart Pi';
const HA_USER = 'smartpi';
const HA_PASS = 'smartpi';

test.describe.serial('Home Assistant Onboarding', () => {

  test('01 - Welcome page', async ({ page }) => {
    await page.goto(HA_URL);
    await page.waitForLoadState('networkidle');
    await page.waitForTimeout(3000);

    await expect(page.getByText('Welcome!')).toBeVisible();
    await page.screenshot({
      path: path.join(SCREENSHOTS_DIR, '01-ha-welcome.png'),
      fullPage: false,
    });
    console.log('Screenshot: 01-ha-welcome.png');
  });

  test('02 - Create account and complete onboarding', async ({ page }) => {
    await page.goto(HA_URL);
    await page.waitForLoadState('networkidle');
    await page.waitForTimeout(2000);

    // Step 1: Click "Create my smart home"
    await page.getByText('Create my smart home').click();
    await page.waitForTimeout(2000);

    await page.screenshot({
      path: path.join(SCREENSHOTS_DIR, '02-ha-create-account-empty.png'),
      fullPage: false,
    });
    console.log('Screenshot: 02-ha-create-account-empty.png');

    // Step 2: Fill in the account form using name selectors
    await page.locator('input[name="name"]').fill(HA_NAME);
    await page.locator('input[name="username"]').fill(HA_USER);
    await page.locator('input[name="password"]').fill(HA_PASS);
    await page.locator('input[name="password_confirm"]').fill(HA_PASS);
    await page.waitForTimeout(500);

    await page.screenshot({
      path: path.join(SCREENSHOTS_DIR, '03-ha-create-account-filled.png'),
      fullPage: false,
    });
    console.log('Screenshot: 03-ha-create-account-filled.png');

    // Step 3: Submit — Create account
    await page.getByText('Create account').click();
    await page.waitForTimeout(5000);

    await page.screenshot({
      path: path.join(SCREENSHOTS_DIR, '04-ha-home-settings.png'),
      fullPage: false,
    });
    console.log('Screenshot: 04-ha-home-settings.png');

    // Step 4: Home settings page — click Next (accept defaults)
    const nextBtn = page.getByText('Next');
    if (await nextBtn.isVisible()) {
      await nextBtn.click();
      await page.waitForTimeout(3000);
      await page.screenshot({
        path: path.join(SCREENSHOTS_DIR, '05-ha-analytics.png'),
        fullPage: false,
      });
      console.log('Screenshot: 05-ha-analytics.png');
    }

    // Step 5: Analytics page — click Next (opt out by default)
    const nextBtn2 = page.getByText('Next');
    if (await nextBtn2.isVisible()) {
      await nextBtn2.click();
      await page.waitForTimeout(3000);
      await page.screenshot({
        path: path.join(SCREENSHOTS_DIR, '06-ha-finish.png'),
        fullPage: false,
      });
      console.log('Screenshot: 06-ha-finish.png');
    }

    // Step 6: Finish page — click Finish
    const finishBtn = page.getByText('Finish');
    if (await finishBtn.isVisible()) {
      await finishBtn.click();
      await page.waitForTimeout(5000);
    }

    // Final: Dashboard
    await page.waitForTimeout(3000);
    await page.screenshot({
      path: path.join(SCREENSHOTS_DIR, '07-ha-dashboard.png'),
      fullPage: false,
    });
    console.log('Screenshot: 07-ha-dashboard.png');
    console.log('Final URL:', page.url());
  });

});
