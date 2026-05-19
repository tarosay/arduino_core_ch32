// i2c_probe_test - Diagnostic for I2C slave lockup
//
// PURPOSE: Instead of scanning all 126 addresses, probe ONLY address 0x33
// repeatedly. This determines:
//   (A) Does the 3rd probe of 0x33 fail? (not the 3rd scan)
//   (B) What error code does the master get when probe fails?
//   (C) Does a delay between probes help?
//
// Run on MASTER UIAPduino. Slave runs i2c_slave_test.ino.
// Connect SDA (PC1/D3) and SCL (PC2/D4) with 4.7kOhm pull-ups to 3.3V.

#include <Wire.h>
#include <WebHID.h>
#include "Hid.h"

#define SLAVE_ADDR  0x33

// How long to wait between probes (ms). Change to test timing sensitivity.
// 0 = no delay (like fast scanner)
// 10 = short delay
// 500 = 0.5 second between probes
#define PROBE_DELAY_MS  3000

// Returns Wire.endTransmission() code:
//   0 = success (ACK from slave)
//   2 = NACK on address
//   4 = other error (timeout, busy, etc.)
static uint8_t probe(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission();
}

void setup() {
  WebHID.begin();
  delay(5000);

  hid.Clear();
  hid.Println("i2c_probe_test starting...");
  hid.Println("Probing address: 0x33");
  hid.Print("Delay between probes: ");
  hid.Print(PROBE_DELAY_MS);
  hid.Println("ms");
  hid.Println("---");

  Wire.begin();

  uint32_t probeNum = 0;
  while (true) {
    probeNum++;
    uint8_t err = probe(SLAVE_ADDR);

    hid.Print("Probe #");
    hid.Print(probeNum);
    hid.Print(" -> ");
    if (err == 0) {
      hid.Println("OK");
    } else {
      hid.Print("FAIL  err=");
      hid.Println(err);
      hid.Println("--- STOPPED ---");
      while (1) { delay(1000); }
    }

    delay(PROBE_DELAY_MS);
  }
}

void loop() {}
