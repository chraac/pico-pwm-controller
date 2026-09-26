// Fake INA226 power monitor @0x40 (byte-level i2c slave for rp2040js).
//
// Register map (docs/ina226_i2c.md):
//   0x00 config, 0x01 shunt, 0x02 bus, 0x03 power, 0x04 current,
//   0x05 calibration, 0x06 mask/enable, 0xFE mfr id ('TI'), 0xFF die id
//
// Scenario math: watts = powerReg * (20/32768) * 25  (calibration for 20 A)
'use strict';

const REG = {
    CONFIG: 0x00,
    SHUNT_VOLTAGE: 0x01,
    BUS_VOLTAGE: 0x02,
    POWER: 0x03,
    CURRENT: 0x04,
    CALIBRATION: 0x05,
    MASK_ENABLE: 0x06,
    MANUFACTURER_ID: 0xfe,
    DIE_ID: 0xff,
};

// calibration constants mirrored from exec/pwm_controller_pcie.cc
const MAX_CURRENT_AMPS = 20.0;
const CURRENT_LSB = MAX_CURRENT_AMPS / 32768;

function createFakeIna226({ powerWatts = 0, busVolts = 12, shuntMilliVolts = 0, idOk = true } = {}) {
    const regs = new Map();
    regs.set(REG.MANUFACTURER_ID, idOk ? 0x5449 : 0x0000); // 'TI'
    regs.set(REG.DIE_ID, idOk ? 0x2260 : 0x0000);
    const writes = [];

    const state = { powerWatts, busVolts, shuntMilliVolts };

    function refresh() {
        regs.set(REG.BUS_VOLTAGE, Math.round(state.busVolts / 1.25e-3) & 0xffff);
        const shunt = Math.round(state.shuntMilliVolts / 2.5e-3) & 0xffff;
        regs.set(REG.SHUNT_VOLTAGE, shunt);
        regs.set(REG.POWER, Math.round(state.powerWatts / (CURRENT_LSB * 25)) & 0xffff);
        regs.set(REG.CURRENT, 0);
        regs.set(REG.MASK_ENABLE, 0x0008); // conversion ready (CVRF)
    }
    refresh();

    let ptr = 0;
    let writeBuf = [];
    let readCount = 0;

    return {
        bus: 1, // i2c1, same as XiaoRp2040Ina226Device
        address: 0x40,
        writes,
        set({ powerWatts, busVolts, shuntMilliVolts }) {
            if (powerWatts !== undefined) state.powerWatts = powerWatts;
            if (busVolts !== undefined) state.busVolts = busVolts;
            if (shuntMilliVolts !== undefined) state.shuntMilliVolts = shuntMilliVolts;
            refresh();
        },
        // byte-level i2c slave protocol (see harness/rp2040_runner.js):
        // first write byte selects the register, a 3-byte write stores a
        // value, consecutive reads serve the register MSB first
        writeByte(value) {
            writeBuf.push(value);
            if (writeBuf.length === 1) {
                ptr = value; // register pointer select
            } else if (writeBuf.length === 3) {
                const reg = writeBuf[0];
                const regValue = (writeBuf[1] << 8) | writeBuf[2];
                regs.set(reg, regValue);
                if (reg === REG.CONFIG || reg === REG.CALIBRATION || reg === REG.MASK_ENABLE) {
                    writes.push([reg, regValue]);
                }
                writeBuf = [];
            }
            return true; // ACK
        },
        startWrite() {
            writeBuf = [];
            readCount = 0;
        },
        readByte() {
            const value = regs.get(ptr) ?? 0;
            const byte = readCount % 2 === 0 ? (value >> 8) & 0xff : value & 0xff;
            readCount++;
            return byte;
        },
    };
}

module.exports = { createFakeIna226, REG };
