// Power-to-duty curve table through the whole chain: fake INA226 watts ->
// EMA smoother -> per-fan-type pwr->pwm curves. Fan0 uses the default
// table (145 W -> 10000), fan1 the type-1 table (100 W -> 10000).
//
// watts = powerReg * (20/32768) * 25 (calibration for 20 A)
'use strict';

const { createRunner } = require('../harness/rp2040_runner.js');
const { createFakeIna226 } = require('../harness/fake_ina226.js');
const { createFakeSsd1306 } = require('../harness/fake_ssd1306.js');

// [watts, pwm0 (default table), pwm1 (type-1 table)] -- from temp_helper.hh
// kDefaultPwrToPwmCurve / kPwrToPwmCurveFanType1 in pwm_controller_pcie.cc
const CASES = [
    [0, 1500, 1500],    // below first point clamps
    [5, 1500, 1500],    // exact first point
    [50, 3100, 3600],   // the fan-type divergence
    [90, 5500, 8100],
    [145, 10000, 10000], // above type-1's last point
    [200, 10000, 10000], // above both tables' last point
];

async function run({ fwPath, assert, assertNear, parseTestLine }) {
    const ina226 = createFakeIna226({ powerWatts: 0, busVolts: 12 });
    const runner = createRunner({
        fwPath,
        i2cDevices: [ina226, createFakeSsd1306()],
    });

    try {
        // wait for the loop to start before driving the sensor
        const started = await runner.waitLines(
            (lines) => lines.some((l) => l.startsWith('TEST: ')),
            30_000,
        );
        assert(started, 'timed out waiting for the first TEST: line');

        for (const [watts, wantPwm0, wantPwm1] of CASES) {
            ina226.set({ powerWatts: watts });

            // EMA converges at rate 0.4/iteration (12+ iterations to <0.5%);
            // below the 5 W idle floor sw pins at 5 (it never decays under it)
            const settleTarget = Math.max(watts, 5);
            const settled = await runner.waitLines((lines) => {
                const parsed = lines
                    .map(parseTestLine)
                    .filter(
                        (f) =>
                            f &&
                            Math.abs(f.w - watts) < 0.15 &&
                            Math.abs(f.sw - settleTarget) < 0.05,
                    );
                return parsed.length >= 2;
            }, 40_000);
            assert(settled, `watts ${watts}: EMA did not settle at ${settleTarget}W in time`);

            // +-5: w quantizes to 0.015 W (INA226 LSB), the EMA approaches
            // from below, and GetCurveValue truncates -- on steep segments
            // that combination reads a few cycles low. Still unambiguous
            // against curve mix-ups (3100 vs 3600) and clamps.
            const last = parseTestLine(
                runner.lines.filter((l) => l.startsWith('TEST: ')).pop(),
            );
            assertNear(last.pwm0, wantPwm0, 5, `watts ${watts} pwm0`);
            assertNear(last.pwm1, wantPwm1, 5, `watts ${watts} pwm1`);
        }
    } finally {
        runner.stop();
    }
}

module.exports = { run };
