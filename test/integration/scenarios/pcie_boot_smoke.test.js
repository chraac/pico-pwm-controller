// Boot smoke: firmware boots and the control loop runs against the fake
// INA226. NOTE: rp2040js does not reliably capture the first ~0.5 s of
// stdout (the boot log lines), so all assertions target loop-phase output
// (TEST: lines from the INTEGRATION_TEST build), which is fully captured.
//
// Negative variant: with no INA226 attached, reads return 0xffff, so the
// firmware sees ~1000 W and both fan curves clamp to full duty.
'use strict';

const { createRunner } = require('../harness/rp2040_runner.js');
const { createFakeIna226 } = require('../harness/fake_ina226.js');
const { createFakeSsd1306 } = require('../harness/fake_ssd1306.js');

async function run({ fwPath, assert, assertNear, parseTestLine }) {
    // positive: 50 W load -> EMA settles -> per-fan-type duty (3100/3600)
    {
        const ina226 = createFakeIna226({ powerWatts: 50, busVolts: 12 });
        const runner = createRunner({
            fwPath,
            i2cDevices: [ina226, createFakeSsd1306()],
        });
        try {
            const ok = await runner.waitLines(
                (lines) => lines.filter((l) => l.startsWith('TEST: ')).length >= 3,
                30_000,
            );
            assert(ok, 'timed out waiting for 3 TEST: lines');

            const settled = await runner.waitLines((lines) => {
                const parsed = lines
                    .map(parseTestLine)
                    .filter((f) => f && Math.abs(f.sw - 50) < 0.02);
                return parsed.length >= 2;
            }, 40_000);
            assert(settled, 'EMA did not settle at 50 W');

            const last = parseTestLine(
                runner.lines.filter((l) => l.startsWith('TEST: ')).pop(),
            );
            assertNear(last.w, 50.003, 0.15, 'watts echo');
            assertNear(last.pwm0, 3100, 2, 'pwm0 (default fan table @50W)');
            assertNear(last.pwm1, 3600, 2, 'pwm1 (fan type-1 table @50W)');
            assert(last.rpm0 >= 0 && last.rpm1 >= 0, 'rpm fields present');
        } finally {
            runner.stop();
        }
    }

    // negative: wrong device at 0x40 (ACKs, but not INA226 ids) -> Probe()
    // fails, power reads 0 -> EMA sits on the 5 W idle floor -> minimum duty.
    // (A fully absent device can't be tested here: rp2040js wedges the SDK
    // i2c driver on address NACK, unlike real hardware.)
    {
        const wrongDevice = createFakeIna226({ powerWatts: 0, idOk: false });
        const runner = createRunner({
            fwPath,
            i2cDevices: [wrongDevice, createFakeSsd1306()],
        });
        try {
            const ok = await runner.waitLines(
                (lines) => lines.filter((l) => l.startsWith('TEST: ')).length >= 3,
                30_000,
            );
            assert(ok, 'timed out waiting for TEST: lines (probe-failed variant)');

            const settled = await runner.waitLines((lines) => {
                const parsed = lines
                    .map(parseTestLine)
                    .filter((f) => f && Math.abs(f.sw - 5) < 0.1);
                return parsed.length >= 2;
            }, 40_000);
            assert(settled, 'EMA did not sit on the 5 W idle floor');

            const last = parseTestLine(
                runner.lines.filter((l) => l.startsWith('TEST: ')).pop(),
            );
            assertNear(last.w, 0.0, 0.15, 'watts read as zero from the wrong device');
            assert(last.pwm0 === 1500 && last.pwm1 === 1500,
                `expected both fans at idle duty 1500, got ${last.pwm0}/${last.pwm1}`);
        } finally {
            runner.stop();
        }
    }
}

module.exports = { run };
