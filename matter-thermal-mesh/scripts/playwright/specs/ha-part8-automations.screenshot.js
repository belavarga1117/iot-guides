// @ts-check
const { test, expect } = require('@playwright/test');
const path = require('path');
const fs = require('fs');

const SCREENSHOTS_DIR = path.join(__dirname, '../../../docs/screenshots/08-automations');
const HA_URL = 'http://192.168.1.111:8123';
const HA_USER = 'smartpi';
const HA_PASS = 'smartpi';
const HA_TOKEN = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiIyODZjNDQ0OWE2OGM0N2FlYTcyMDhiMjE1NGFiZGU2MCIsImlhdCI6MTc3MjQ0ODk1NCwiZXhwIjoyMDg3ODA4OTU0fQ.PS9pd6Dj8qJKxOEAdoKWL_sVz9EvZLBgxCgv7pyr-nc';

fs.mkdirSync(SCREENSHOTS_DIR, { recursive: true });

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

/**
 * Create the presence automation via HA REST API (storage format).
 * Returns the automation ID so we can navigate to it.
 */
async function createPresenceAutomation() {
  const { default: fetch } = await import('node-fetch');
  const automation = {
    alias: 'Presence Detected — Notify',
    description: 'Sends a mobile notification when any thermal sensor detects someone',
    triggers: [
      {
        trigger: 'state',
        entity_id: [
          'binary_sensor.matter_device_occupancy',
          'binary_sensor.matter_device_occupancy_4',
          'binary_sensor.matter_device_occupancy_5',
        ],
        to: 'on',
      },
    ],
    conditions: [],
    actions: [
      {
        action: 'notify.notify',
        data: {
          title: 'Presence detected',
          message: 'A sensor just detected someone!',
        },
      },
    ],
    mode: 'single',
  };

  const res = await fetch(`${HA_URL}/api/config/automation/config`, {
    method: 'POST',
    headers: {
      Authorization: `Bearer ${HA_TOKEN}`,
      'Content-Type': 'application/json',
    },
    body: JSON.stringify(automation),
  });
  const data = await res.json();
  return data.result;   // automation ID
}

test.describe('Part 8 — Automations', () => {

  test('01 - Automations dashboard (empty)', async ({ page }) => {
    await loginToHA(page);
    await page.goto(`${HA_URL}/config/automation/dashboard`);
    await page.waitForTimeout(4000);
    await page.screenshot({
      path: path.join(SCREENSHOTS_DIR, '01-automations-dashboard.png'),
      fullPage: false,
    });
  });

  test('02 - Entities page — search occupancy', async ({ page }) => {
    await loginToHA(page);
    await page.goto(`${HA_URL}/config/entities`);
    await page.waitForTimeout(4000);
    // Type in the search box
    const searchBox = page.locator('input[placeholder*="Search"]').first();
    if (await searchBox.isVisible({ timeout: 3000 }).catch(() => false)) {
      await searchBox.fill('occupancy');
      await page.waitForTimeout(2000);
    }
    await page.screenshot({
      path: path.join(SCREENSHOTS_DIR, '02-entities-occupancy.png'),
      fullPage: false,
    });
  });

  test('03 - New automation editor (visual)', async ({ page }) => {
    await loginToHA(page);
    await page.goto(`${HA_URL}/config/automation/edit/new`);
    await page.waitForTimeout(4000);
    await page.screenshot({
      path: path.join(SCREENSHOTS_DIR, '03-new-automation-editor.png'),
      fullPage: false,
    });
  });

  test('04 - New automation YAML editor', async ({ page }) => {
    await loginToHA(page);
    await page.goto(`${HA_URL}/config/automation/edit/new`);
    await page.waitForTimeout(4000);

    // Switch to YAML mode via the three-dot menu (ha-button-menu)
    // In HA 2024+, there's a "Edit in YAML" option in the overflow menu
    const menuBtn = page.locator('ha-button-menu').first();
    if (await menuBtn.isVisible({ timeout: 3000 }).catch(() => false)) {
      await menuBtn.click();
      await page.waitForTimeout(1000);
      // Look for "Edit in YAML" menu item
      const yamlOption = page.locator('mwc-list-item, ha-list-item').filter({ hasText: /yaml/i });
      if (await yamlOption.isVisible({ timeout: 2000 }).catch(() => false)) {
        await yamlOption.click();
        await page.waitForTimeout(2000);
      }
    }

    await page.screenshot({
      path: path.join(SCREENSHOTS_DIR, '04-yaml-editor.png'),
      fullPage: false,
    });
  });

  test('05 - Automation saved in list', async ({ page }) => {
    // Create the automation via API first
    try {
      await createPresenceAutomation();
      await new Promise(r => setTimeout(r, 2000));
    } catch (e) {
      console.log('API create failed (may already exist):', e.message);
    }

    await loginToHA(page);
    await page.goto(`${HA_URL}/config/automation/dashboard`);
    await page.waitForTimeout(5000);
    await page.screenshot({
      path: path.join(SCREENSHOTS_DIR, '05-automation-in-list.png'),
      fullPage: false,
    });
  });

  test('05b - Automation detail page', async ({ page }) => {
    await loginToHA(page);
    // Navigate directly to the automation edit page (ID is in the HA config)
    await page.goto(`${HA_URL}/config/automation/edit/1709427600001`);
    await page.waitForTimeout(4000);
    await page.screenshot({
      path: path.join(SCREENSHOTS_DIR, '05b-automation-detail.png'),
      fullPage: false,
    });
  });

  test('06 - Entities page — CO2 entity', async ({ page }) => {
    await loginToHA(page);
    // Show the CO2 / air quality entity on the Living Room device
    await page.goto(`${HA_URL}/config/entities?search=air_quality`);
    await page.waitForTimeout(4000);
    await page.screenshot({
      path: path.join(SCREENSHOTS_DIR, '06-entities-co2.png'),
      fullPage: false,
    });
  });

});
