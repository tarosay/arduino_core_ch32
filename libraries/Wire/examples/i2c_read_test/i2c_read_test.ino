// i2c_read_test - Test onRequest: master reads _blinkMs from slave
//
// PURPOSE: Verify that the slave's RequestEvent callback fires correctly
// and that the master can read back the current _blinkMs value.
//
// Run on MASTER UIAPduino. Slave runs i2c_slave_diag.ino.
// Connect SDA (PC1/D3) and SCL (PC2/D4) with 4.7kOhm pull-ups to 3.3V.
//
// Expected output (slave has default _blinkMs=500):
//   Read #1: _blinkMs=500  -> PASS
//   (then write 100ms, read back 100)
//   Read #2: _blinkMs=100  -> PASS

#include <Wire.h>
#include <WebHID.h>
#include "Hid.h"

#define SLAVE_ADDR  0x33

// Read 4-byte uint32_t from slave (little-endian, as sent by Wire.write(&val,4))
static uint32_t readBlinkMs()
{
  uint32_t val = 0;
  uint8_t n = Wire.requestFrom(SLAVE_ADDR, (uint8_t)4);
  for (int shift = 0; shift < 32 && Wire.available(); shift += 8)
  {
    val |= (uint32_t)(uint8_t)Wire.read() << shift;
  }
  return val;
}

// Write 4-byte uint32_t to slave (little-endian)
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
  hid.Println("i2c_read_test");
  hid.Println("Reading _blinkMs from slave 0x33");
  hid.Println("---");

  Wire.begin();
  delay(500);

  // Read #1: slave default _blinkMs should be 500
  uint32_t val = readBlinkMs();
  hid.Print("Read #1: _blinkMs=");
  hid.Print(val);
  if (val == 500) {
    hid.Println("  -> PASS");
  } else {
    hid.Print("  -> FAIL (expected 500)");
    hid.Println();
  }
  delay(1000);

  // Write 100ms to slave
  uint8_t err = writeBlinkMs(100);
  hid.Print("Write 100ms -> err="); hid.Println(err);
  delay(500);

  // Read #2: should now read back 100
  val = readBlinkMs();
  hid.Print("Read #2: _blinkMs=");
  hid.Print(val);
  if (val == 100) {
    hid.Println("  -> PASS");
  } else {
    hid.Print("  -> FAIL (expected 100)");
    hid.Println();
  }
  delay(1000);

  // Restore default
  writeBlinkMs(500);
  hid.Println("Restored 500ms.");
  hid.Println("---");
  hid.Println("DONE");
}

void loop() {}
