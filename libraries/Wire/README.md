# Wire (I2C) library for UIAPduino (CH32V003F4)

Arduino-compatible Wire library for the UIAPduino board based on the WCH CH32V003F4 RISC-V microcontroller.

## Wiring

| Signal | Pin | Arduino name |
|--------|-----|-------------|
| SDA    | PC1 | D3          |
| SCL    | PC2 | D4          |
| GND    | GND | GND         |

Add a **4.7 kΩ pull-up resistor** from SDA to 3.3 V and from SCL to 3.3 V.

```
UIAPduino A          UIAPduino B
  D3 (SDA) ---[4k7]---+--- (SDA) D3
  D4 (SCL) ---[4k7]---+--- (SCL) D4
                       |
                      3.3V
```

## Master mode

```cpp
#include <Wire.h>

Wire.begin();                        // master (no address)

// Write to slave
Wire.beginTransmission(0x33);
Wire.write(data, length);
uint8_t err = Wire.endTransmission(); // 0 = OK, 2 = NACK, 4 = timeout

// Read from slave
uint8_t n = Wire.requestFrom(0x33, (uint8_t)4);
while (Wire.available()) {
    uint8_t b = Wire.read();
}
```

## Slave mode

```cpp
#include <Wire.h>

void onReceive(int n) {
    // Called from ISR context — keep short, no blocking calls.
    // Wire.read() is safe here.
    while (n--) Wire.read();
}

void onRequest() {
    // Called from ISR context — keep short.
    Wire.write(data, length);
}

Wire.begin(0x33);           // slave at address 0x33
Wire.onReceive(onReceive);
Wire.onRequest(onRequest);
```

### Byte order

`Wire.write(&val, sizeof(val))` sends bytes in **little-endian** (memory) order
(LSB first), which matches the CH32V003's native endianness.
Reconstruct on the receiving side accordingly:

```cpp
void onReceive(int n) {
    uint32_t val = 0;
    for (int shift = 0; n > 0 && shift < 32; shift += 8, n--)
        val |= (uint32_t)(uint8_t)Wire.read() << shift;
}
```

### Callbacks and HID

The `onReceive` and `onRequest` callbacks are called from the I2C interrupt
service routine (ISR). Do **not** call `hid.Print()` or any other blocking
function inside these callbacks. Set a flag and print from `loop()` instead:

```cpp
volatile bool rxReady = false;

void onReceive(int n) {
    // ... read data ...
    rxReady = true;        // set flag; print from loop()
}

void loop() {
    if (rxReady) {
        rxReady = false;
        hid.Println("received");
    }
}
```

## I2C Scanner

The `i2c_scanner` example scans all 7-bit addresses (0x08–0x77) and prints
found devices via WebHID. It repeats every second.

## Notes on rv003usb (software USB) compatibility

UIAPduino uses **rv003usb** — a software bit-banging USB stack that handles
full-speed USB (12 Mbps) in the `EXTI7_0_IRQHandler` interrupt.

### Why `WCH-Interrupt-fast` must not be used for I2C ISRs

WCH's PFIC "HPE" (High Performance Entry) mechanism fires fast interrupts even
when the machine-level interrupt enable bit (MIE) is cleared. This means an I2C
ISR declared with `__attribute__((interrupt("WCH-Interrupt-fast")))` can
preempt the USB handler mid-execution, corrupting the bit-sampling loop and
causing USB HID to disconnect.

The Wire library uses `__attribute__((interrupt))` (standard RISC-V interrupt)
for all I2C ISR handlers. Standard interrupts respect MIE, so they cannot fire
while the USB handler is running.

### ITBUFEN must remain disabled

With `ITBUFEN=1`, the TXE flag fires an interrupt immediately after ADDR is
cleared. This creates an ISR storm that starves the ADDR handler and causes the
slave to stop responding. The library sets `ITBUFEN=0`; only event (`ITEVTEN`)
and error (`ITERREN`) interrupts are enabled.

### Interrupt setup order

The I2C peripheral performs a PE (Peripheral Enable) disable/enable cycle
during `I2C_Init()`. If NVIC interrupts are armed before `I2C_Init()`, the PE
rising edge fires a spurious ISR before `OADDR1` and `ACK` are configured.
The library enables `CTLR2` interrupt bits and `NVIC_EnableIRQ` **after**
`I2C_Init()` and `I2C_Cmd(ENABLE)`.

## Examples

| Example | Description |
|---------|-------------|
| `i2c_scanner` | Scan all I2C addresses, print results via WebHID |
| `i2c_slave_test` | Slave: receive blink interval, send it back on request |
| `i2c_master_test` | Master: write/read blink interval to/from slave |
| `i2c_probe_test` | Repeatedly probe one address; confirm stability |
| `i2c_slave_diag` | Slave with register dump and onReceive logging for debugging |
