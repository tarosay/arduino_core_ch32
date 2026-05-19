// I2C Scanner for UIAPduino (CH32V003F4)
//
// Scans all 7-bit I2C addresses (0x08..0x77) and reports found devices
// via WebHID. Repeats the scan every second with a scan counter.
// Wire.end()/Wire.begin() is called each cycle to prevent I2C peripheral
// lockup caused by accumulated NACKs from non-existing addresses.
//
// Original concept by Nick Gammon (2011).
// Adapted for CH32V003 UIAPduino with WebHID output.
//
// Wiring:
//   SDA: PC1 (D3)
//   SCL: PC2 (D4)
//   Pull-up: 4.7kOhm from SDA and SCL to 3.3V

#include <Wire.h>
#include <WebHID.h>
#include "Hid.h"

static void printHex(uint8_t v)
{
  const char *hex = "0123456789ABCDEF";
  char buf[5] = {'0','x', hex[(v>>4)&0xF], hex[v&0xF], 0};
  hid.Print(buf);
}

void setup()
{
  WebHID.begin();
  delay(5000);

  hid.Clear();
  hid.Println("I2C Scanner (UIAPduino)");
  hid.Println("SDA=PC1(D3) SCL=PC2(D4)");
  hid.Println("---");

  uint32_t scanCount = 0;

  while (true)
  {
    // I2C ペリフェラルをサイクルごとにリセット（連続 NACK によるロックアップを防止）
    Wire.end();
    Wire.begin();

    scanCount++;
    uint8_t count = 0;

    for (uint8_t addr = 8; addr < 120; addr++)
    {
      Wire.beginTransmission(addr);
      if (Wire.endTransmission() == 0)
      {
        hid.Print("Found: ");
        printHex(addr);
        hid.Println();
        count++;
      }
    }

    hid.Print("Scan ");
    hid.Print((int)scanCount);
    hid.Print(": Found ");
    hid.Print(count);
    hid.Println(" device(s).");
    hid.Println("---");

    delay(1000);
  }
}

void loop() {}
