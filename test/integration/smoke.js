// Smoke run: boot the real firmware in the simulator, attach fake
// peripherals, print every captured stdout line live. No assertions --
// this is the interactive debugging console (use run-all.js --ci for tests).
'use strict';

const path = require('path');
const { createRunner } = require('./harness/rp2040_runner.js');
const { createFakeIna226 } = require('./harness/fake_ina226.js');
const { createFakeSsd1306 } = require('./harness/fake_ssd1306.js');

const fwPath = process.env.PWM_TEST_FW;
const durationMs = Number(process.env.PWM_TEST_SMOKE_MS ?? 20_000);
const watts = Number(process.env.PWM_TEST_WATTS ?? 50);

if (!fwPath) {
    console.error('PWM_TEST_FW not set (use ./run.sh)');
    process.exit(1);
}

const ina226 = createFakeIna226({ powerWatts: watts, busVolts: 12 });
const ssd1306 = createFakeSsd1306();

console.log(`[smoke] firmware: ${fwPath}`);
console.log(`[smoke] fake INA226 @0x40 -> ${watts} W @ 12 V, SSD1306 @0x3c attached`);
console.log('[smoke] booting...');

const runner = createRunner({
    fwPath,
    i2cDevices: [ina226, ssd1306],
});

runner.onLine((line) => console.log(`[fw] ${line}`));

setTimeout(() => {
    console.log(`[smoke] ${runner.lines.length} lines captured, stopping`);
    console.log(`[smoke] i2c: INA226 writes=${JSON.stringify(ina226.writes)} ` +
        `SSD1306 bytes=${ssd1306.byteCount}`);
    runner.stop();
    process.exit(0);
}, durationMs);
