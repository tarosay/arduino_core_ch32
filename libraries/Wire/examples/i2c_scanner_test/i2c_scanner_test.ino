// i2c_scanner_test - Full I2C scan repeated 5 times
//
// PURPOSE: Confirm that the slave at 0x33 is detected on every scan,
// and that HID stays connected throughout (the original bug was that
// the slave disappeared after scan #2).
//
// Run on MASTER UIAPduino. Slave runs i2c_slave_diag.ino.
// Connect SDA (PC1/D3) and SCL (PC2/D4) with 4.7kOhm pull-ups to 3.3V.
//
// Expected output: "Found: 0x33" appears in every scan, no FAIL lines.

#include <Wire.h>
#include <WebHID.h>
#include "Hid.h"

#define EXPECTED_ADDR  0x33
#define NUM_SCANS      5

void setup()
{
  WebHID.begin();
  delay(5000);

  hid.Clear();
  hid.Println("i2c_scanner_test");
  hid.Print("Scanning "); hid.Print(NUM_SCANS);
  hid.Println(" times, expect 0x33 each time");
  hid.Println("---");

  Wire.begin();
  delay(500);

  bool allPassed = true;

  for (int scan = 1; scan <= NUM_SCANS; scan++)
  {
    hid.Print("Scan #"); hid.Print(scan); hid.Print(": ");

    bool found = false;
    for (uint8_t addr = 1; addr < 127; addr++)
    {
      Wire.beginTransmission(addr);
      uint8_t err = Wire.endTransmission();
      if (err == 0)
      {
        const char *hex = "0123456789ABCDEF";
        char buf[5] = {'0','x', hex[(addr>>4)&0xF], hex[addr&0xF], 0};
        hid.Print(buf); hid.Print(" ");
        if (addr == EXPECTED_ADDR) found = true;
      }
    }

    if (found) {
      hid.Println("-> PASS");
    } else {
      hid.Println("-> FAIL (0x33 not found!)");
      allPassed = false;
    }

    delay(500);
  }

  hid.Println("---");
  if (allPassed) {
    hid.Println("ALL SCANS PASSED");
  } else {
    hid.Println("SOME SCANS FAILED");
  }
}

void loop() {}
