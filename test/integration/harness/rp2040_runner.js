// rp2040_runner: the ONLY file that talks to the rp2040js API directly, so
// upstream API changes stay local. Everything below was verified against
// vendor/rp2040js-1.3.4.tgz typings and the upstream demo sources.
//
//   createRunner({ fwPath, i2cDevices }) ->
//     { mcu, lines, stop(), waitLines(), setGpio() }
//
// - Loads a .uf2 (via the vendored `uf2` decoder) or a flash-linked .elf
//   (minimal ELF32 program-header loader), boots at 0x10000000.
// - Captures stdout from BOTH UART0 and USB CDC, whichever the firmware uses.
// - Attaches byte-level fake i2c devices ({bus, address, writeByte,
//   readByte, startWrite?}) to mcu.i2c[bus].
'use strict';

const fs = require('fs');
const path = require('path');

// Deps live in the sibling simulator/ package (vendored, offline). Node does
// not look in sibling dirs, so resolve them explicitly.
function requireSim(moduleName) {
    return require(path.join(__dirname, '..', 'simulator', 'node_modules', moduleName));
}

const { Simulator, USBCDC, ConsoleLogger, LogLevel } = requireSim('rp2040js');
const { decodeBlock } = requireSim('uf2');
const { bootromB1 } = require('../simulator/vendor/bootrom.js');

const FLASH_BASE = 0x10000000;

function loadUf2(mcu, buf) {
    if (buf.length % 512 !== 0) {
        throw new Error(`uf2 size ${buf.length} is not a multiple of 512`);
    }
    let blocks = 0;
    for (let off = 0; off < buf.length; off += 512) {
        const block = decodeBlock(buf.subarray(off, off + 512));
        if (block && block.payload && block.flashAddress >= FLASH_BASE) {
            mcu.flash.set(block.payload, block.flashAddress - FLASH_BASE);
            blocks++;
        }
    }
    if (blocks === 0) {
        throw new Error('no flash blocks found in uf2 file');
    }
    return blocks;
}

function loadElf(mcu, buf) {
    const dv = new DataView(buf.buffer, buf.byteOffset, buf.byteLength);
    const magic = dv.getUint32(0, true);
    if (magic !== 0x464c457f) {
        throw new Error('not an ELF file');
    }
    if (dv.getUint16(18, true) !== 40 /* EM_ARM */) {
        throw new Error('ELF is not ARM');
    }
    const phoff = dv.getUint32(28, true);
    const phentsize = dv.getUint16(42, true);
    const phnum = dv.getUint16(44, true);
    let segments = 0;
    for (let i = 0; i < phnum; i++) {
        const ph = phoff + i * phentsize;
        const type = dv.getUint32(ph, true);
        const offset = dv.getUint32(ph + 4, true);
        const paddr = dv.getUint32(ph + 12, true);
        const filesz = dv.getUint32(ph + 16, true);
        if (type !== 1 /* PT_LOAD */ || filesz === 0 || paddr < FLASH_BASE) {
            continue;
        }
        mcu.flash.set(buf.subarray(offset, offset + filesz), paddr - FLASH_BASE);
        segments++;
    }
    if (segments === 0) {
        throw new Error('no flash PT_LOAD segments in elf file');
    }
    return segments;
}

// Wire the byte-level i2c slave hooks (verified against rp2040js
// src/peripherals/i2c.ts): onStart/onConnect/onWriteByte/onReadByte/onStop,
// each completing through the peripheral's complete*() methods.
function attachI2cDevice(i2c, devices, name, debug) {
    let selected = null;
    i2c.onStart = () => i2c.completeStart();
    i2c.onConnect = (address) => {
        selected = devices.find((d) => d.address === address) ?? null;
        if (debug) console.error(`[i2c${name}] connect 0x${address.toString(16)} -> ${selected ? 'ACK' : 'NACK'}`);
        if (selected?.startWrite) selected.startWrite();
        i2c.completeConnect(!!selected);
    };
    i2c.onWriteByte = (value) => {
        if (debug) console.error(`[i2c${name}] write 0x${value.toString(16)}`);
        i2c.completeWrite(selected ? selected.writeByte(value) !== false : false);
    };
    i2c.onReadByte = (ack) => {
        const value = selected ? selected.readByte() : 0xff;
        if (debug) console.error(`[i2c${name}] read -> 0x${value.toString(16)}`);
        i2c.completeRead(value);
    };
    i2c.onStop = () => {
        if (debug) console.error(`[i2c${name}] stop`);
        selected = null;
        i2c.completeStop();
    };
}

function createRunner({ fwPath, i2cDevices = [] } = {}) {
    const simulator = new Simulator();
    const mcu = simulator.rp2040;
    mcu.logger = new ConsoleLogger(LogLevel.Error);

    mcu.loadBootrom(bootromB1);

    const fw = fs.readFileSync(fwPath);
    if (fwPath.endsWith('.uf2')) {
        loadUf2(mcu, fw);
    } else {
        loadElf(mcu, fw);
    }

    const byBus = new Map();
    for (const dev of i2cDevices) {
        const bus = dev.bus ?? 1;
        if (!byBus.has(bus)) byBus.set(bus, []);
        byBus.get(bus).push(dev);
    }
    const debug = !!process.env.PWM_TEST_DEBUG;
    // Hook BOTH buses even when empty: our hooks serve 0xff for unknown
    // addresses (like a pull-up-only bus), while the rp2040js defaults
    // wedge the SDK driver after an address NACK (read never completes).
    for (const bus of [0, 1]) {
        attachI2cDevice(mcu.i2c[bus], byBus.get(bus) ?? [], bus, debug);
    }

    // stdout capture: UART0 and USB CDC both feed the same line buffer
    const lines = [];
    let pending = '';
    const subscribers = new Set();
    function feedByte(byte) {
        const ch = String.fromCharCode(byte);
        if (ch === '\n') {
            lines.push(pending);
            pending = '';
            for (const fn of subscribers) fn(lines[lines.length - 1]);
        } else if (ch !== '\r') {
            pending += ch;
        }
    }
    mcu.uart[0].onByte = feedByte;
    const cdc = new USBCDC(mcu.usbCtrl);
    cdc.onSerialData = (buffer) => {
        for (const byte of buffer) feedByte(byte);
    };

    // Boot into the flash image. Pico images begin with the 256-byte boot2
    // stage as RAW CODE (its entry point is the flash base; the app vector
    // table lives after it), so PC=0x10000000 enters boot2 correctly and it
    // brings up XIP before jumping to the app -- same as the upstream
    // emulator-run demo. (Do NOT jump through word[1] as a "reset vector":
    // that is mid-boot2 code, and the image never boots.)
    mcu.core.PC = FLASH_BASE;
    simulator.execute();

    return {
        mcu,
        simulator,
        lines,
        cdc,
        onLine(fn) {
            subscribers.add(fn);
        },
        // drive a gpio input (e.g. tach edges)
        setGpio(pin, value) {
            mcu.gpio[pin].setInputValue(value);
        },
        // resolve when predicate(lines) turns true, or false on timeout
        waitLines(predicate, timeoutMs) {
            return new Promise((resolve) => {
                if (predicate(lines)) {
                    resolve(true);
                    return;
                }
                const timer = setTimeout(() => {
                    subscribers.delete(check);
                    resolve(false);
                }, timeoutMs);
                const check = () => {
                    if (predicate(lines)) {
                        clearTimeout(timer);
                        subscribers.delete(check);
                        resolve(true);
                    }
                };
                subscribers.add(check);
            });
        },
        stop() {
            simulator.stop();
        },
    };
}

module.exports = { createRunner, FLASH_BASE };
