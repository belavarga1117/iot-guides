const { chromium } = require('playwright');

const HA_URL = 'http://smartpi.local:8123';
const OLD_DEVICE_IDS = [
  'c6f30fd90c79341f3c2274c8a7131f6a',
  'ef0cd75e0b16b6a4cb6d09d5477243f9',
];

async function loginToHA(page) {
  await page.goto(HA_URL);
  await page.waitForTimeout(5000);
  const usernameInput = page.locator('input[name="username"]');
  if (await usernameInput.isVisible({ timeout: 3000 }).catch(() => false)) {
    await usernameInput.fill('smartpi');
    await page.locator('input[name="password"]').fill('smartpi');
    await page.getByRole('button', { name: 'Log in' }).click();
    await page.waitForTimeout(4000);
  }
}

(async () => {
  const browser = await chromium.launch({ headless: true });
  const page = await browser.newPage({ viewport: { width: 1280, height: 800 } });
  page.setDefaultTimeout(15000);
  
  await loginToHA(page);
  console.log('Logged in');

  for (const deviceId of OLD_DEVICE_IDS) {
    console.log('\nOpening device', deviceId);
    await page.goto(`${HA_URL}/config/devices/device/${deviceId}`);
    await page.waitForTimeout(4000);
    
    // Click the ⋮ button in the top-right page header (x≈1246, y≈27)
    await page.mouse.click(1246, 27);
    await page.waitForTimeout(1500);
    await page.screenshot({ path: `/tmp/menu-${deviceId.slice(0,8)}.png` });
    
    // Find "Delete" menu item
    const deleteItem = page.getByText('Delete', { exact: true });
    if (await deleteItem.isVisible({ timeout: 3000 }).catch(() => false)) {
      console.log('  Found Delete menu item!');
      await deleteItem.click();
      await page.waitForTimeout(1500);
      await page.screenshot({ path: `/tmp/confirm-${deviceId.slice(0,8)}.png` });
      
      // Click confirm button in dialog
      const confirmBtn = page.getByRole('button', { name: 'Delete' });
      if (await confirmBtn.isVisible({ timeout: 3000 }).catch(() => false)) {
        await confirmBtn.click();
        await page.waitForTimeout(2000);
        console.log('  Deleted!');
      } else {
        console.log('  No confirm button found');
      }
    } else {
      console.log('  Delete not visible, check screenshot');
    }
    await page.screenshot({ path: `/tmp/result-${deviceId.slice(0,8)}.png` });
  }

  await browser.close();
  console.log('\nDone');
})();
