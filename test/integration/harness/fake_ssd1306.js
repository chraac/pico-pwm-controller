// Fake SSD1306 OLED @0x3C: absorbs every write with an ACK so the firmware's
// LCD drawing never stalls the bus. Records the byte count per transaction.
'use strict';

function createFakeSsd1306() {
    let byteCount = 0;
    let buf = [];
    return {
        bus: 1, // shares i2c1 with the INA226 on the pcie board
        address: 0x3c,
        transactions: 0,
        get byteCount() {
            return byteCount;
        },
        writeByte(value) {
            buf.push(value);
            return true; // ACK everything
        },
        startWrite() {
            if (buf.length > 0) this.transactions++;
            buf = [];
        },
        readByte() {
            return 0x00;
        },
    };
}

module.exports = { createFakeSsd1306 };
