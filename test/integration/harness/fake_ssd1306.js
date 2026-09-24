// Fake SSD1306 OLED @0x3C: absorbs every write with an ACK so the firmware's
// LCD drawing never stalls the bus. Records the byte count per transaction.
'use strict';

function createFakeSsd1306() {
    let byteCount = 0;
    let buf = [];
    return {
        bus: 0, // pcie main uses CustomLcdDrawer0<...> = CustomSsd1306Device0 = i2c0
        address: 0x3c,
        transactions: 0,
        get byteCount() {
            return byteCount;
        },
        writeByte(value) {
            byteCount++;
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
