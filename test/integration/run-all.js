// CI mode: run every scenarios/*.test.js, aggregate, exit 0/1.
// Each scenario exports: async function run() that throws on failure
// (assert helpers below).
'use strict';

const fs = require('fs');
const path = require('path');

function assert(condition, message) {
    if (!condition) {
        throw new Error(message);
    }
}

function assertEq(actual, expected, label) {
    if (actual !== expected) {
        throw new Error(`${label}: expected ${expected}, got ${actual}`);
    }
}

function assertNear(actual, expected, tolerance, label) {
    if (Math.abs(actual - expected) > tolerance) {
        throw new Error(`${label}: expected ~${expected} (+-${tolerance}), got ${actual}`);
    }
}

// parse a "TEST: it=%d w=%f sw=%f pwm0=%u rpm0=%u pwm1=%u rpm1=%u" line
function parseTestLine(line) {
    if (!line.startsWith('TEST: ')) {
        return null;
    }
    const fields = {};
    for (const token of line.slice(6).split(/\s+/)) {
        const eq = token.indexOf('=');
        if (eq > 0) {
            const key = token.slice(0, eq);
            const value = token.slice(eq + 1);
            fields[key] = key.startsWith('rpm') || key.startsWith('pwm') || key === 'it'
                ? Number(value)
                : value;
        }
    }
    ['w', 'sw'].forEach((k) => {
        if (fields[k] !== undefined) fields[k] = parseFloat(fields[k]);
    });
    return fields;
}

async function main() {
    const scenariosDir = path.join(__dirname, 'scenarios');
    const files = fs.readdirSync(scenariosDir).filter((f) => f.endsWith('.test.js')).sort();

    const fwPath = process.env.PWM_TEST_FW;
    if (!fwPath) {
        console.error('PWM_TEST_FW not set (use ./run.sh)');
        process.exit(1);
    }

    let failed = 0;
    for (const file of files) {
        const name = file.replace('.test.js', '');
        process.stdout.write(`[${name}] running... `);
        try {
            const scenario = require(path.join(scenariosDir, file));
            await scenario.run({ fwPath, assert, assertEq, assertNear, parseTestLine });
            console.log('PASS');
        } catch (error) {
            failed++;
            console.log('FAIL');
            console.log(`  ${error.message}`);
            if (error.stack) {
                console.log(error.stack.split('\n').slice(1, 3).join('\n'));
            }
        }
    }

    console.log(`\n${files.length - failed}/${files.length} scenarios passed`);
    process.exit(failed === 0 ? 0 : 1);
}

main();
