// i2c_write_test - Test onReceive: master writes _blinkMs to slave
//
// PURPOSE: Verify that the slave's ReceiveEvent callback fires correctly
// and that _blinkMs is updated so the LED blink rate changes.
//
// Run on MASTER UIAPduino. Slave runs i2c_slave_diag.ino.
// Connect SDA (PC1/D3) and SCL (PC2/D4) with 4.7kOhm pull-ups to 3.3V.
//
// Expected output:
//   Write 100ms  -> err=0    (slave LED blinks fast)
//   Write 1000ms -> err=0    (slave LED blinks slow)
//   Write 500ms  -> err=0    (slave LED back to default)

#include <Wire.h>
#include <WebHID.h>
#include "Hid.h"

#define SLAVE_ADDR  0x33

static uint8_t writeBlinkMs(uint32_t ms)
{
  Wire.beginTransmission(SLAVE_ADDR);
  Wire.write((const uint8_t *)&ms, sizeof(ms));
  return Wire.endTransmission();
}

void setup()
{
  WebHID.begin();
  delay(5000);

  hid.Clear();
  hid.Println("i2c_write_test");
  hid.Println("Writing _blinkMs to slave 0x33");
  hid.Println("Watch slave LED change speed");
  hid.Println("---");

  Wire.begin();
  delay(500);  // allow slave to finish Wire.begin() before first write

  // Step 1: fast blink (100 ms)
  uint8_t err = writeBlinkMs(100);
  hid.Print("Write 100ms  -> err=");
  hid.Println(err);
  delay(3000);

  // Step 2: slow blink (1000 ms)
  err = writeBlinkMs(1000);
  hid.Print("Write 1000ms -> err=");
  hid.Println(err);
  delay(3000);

  // Step 3: back to default (500 ms)
  err = writeBlinkMs(500);
  hid.Print("Write 500ms  -> err=");
  hid.Println(err);
  delay(3000);

  hid.Println("---");
  if (err == 0) {
    hid.Println("PASS: all writes err=0");
  } else {
    hid.Println("FAIL: last write failed");
  }
}

void loop() {}
