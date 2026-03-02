// @ts-check
const { defineConfig } = require('@playwright/test');

/**
 * Smart Home Matter Starter — Playwright Configuration
 *
 * Used for:
 * 1. Taking screenshots of HA and Flask dashboard for the guide
 * 2. Testing that the guide steps actually work (verification)
 *
 * Run screenshots: npx playwright test --project=screenshots
 * Run tests:       npx playwright test --project=verify
 */
module.exports = defineConfig({
  testDir: './specs',
  timeout: 60000,
  retries: 0,
  reporter: 'list',

  use: {
    // RPi hostname (mDNS)
    baseURL: 'http://smartpi.local:8123',
    screenshot: 'off', // We take manual screenshots in tests
    video: 'off',
    trace: 'off',
  },

  projects: [
    {
      name: 'screenshots',
      testMatch: '**/*.screenshot.js',
      use: {
        viewport: { width: 1280, height: 800 },
        colorScheme: 'light',
      },
    },
    {
      name: 'verify',
      testMatch: '**/*.verify.js',
      use: {
        viewport: { width: 1280, height: 800 },
      },
    },
  ],
});
