// i2c_slave_diag - I2C slave with register dump via WebHID
//
// PURPOSE: Diagnose why the slave stops responding after 2 probes.
// Prints I2C1 register values every 2 seconds so we can see
// if CTLR1, CTLR2, or OADDR1 changes after probe #2.
//
// Run on SLAVE UIAPduino alongside i2c_probe_test on master.
// Set PROBE_DELAY_MS=3000 in i2c_probe_test to give time to observe.
//
// Expected register values (slave healthy):
//   CTLR1  = 0x0401  (PE=1, ACK=1)
//   CTLR2  = 0x0630  (FREQ=48, ITEVTEN=1, ITERREN=1)
//   OADDR1 = 0x0066  (own addr=0x33<<1=0x66; bit14 NOT set by I2C_Init)
//   STAR1  = 0x0000  (no pending flags)
//   STAR2  = 0x0000  (idle)

#include <Wire.h>
#include <WebHID.h>
#include "Hid.h"

#define MY_I2C_ADDRESS  0x33
#define LED_PIN         2   // PC0 = LED_BUILTIN

static uint32_t _blinkMs = 500;

static volatile bool _rxFlag = false;
static volatile int  _rxCount = 0;

void ReceiveEvent(int nNumBytes)
{
  // NOTE: called from ISR context — do NOT call hid.Print() here.
  // Just update state; loop() will print it.
  //
  // Wire.write(&val, 4) sends bytes in little-endian (memory) order: LSB first.
  // Reconstruct with LSB at shift=0, MSB at shift=24.
  uint32_t val = 0;
  for (int shift = 0; nNumBytes > 0 && shift < 32; shift += 8, nNumBytes--)
  {
    val |= (uint32_t)(uint8_t)Wire.read() << shift;
  }
  _blinkMs = val;
  _rxCount++;
  _rxFlag = true;
}

void RequestEvent()
{
  Wire.write((const uint8_t *)&_blinkMs, sizeof(_blinkMs));
}

// Print a 16-bit value in hex (4 digits)
static void printHex(uint16_t v)
{
  const char *hex = "0123456789ABCDEF";
  char buf[5];
  buf[0] = hex[(v >> 12) & 0xF];
  buf[1] = hex[(v >>  8) & 0xF];
  buf[2] = hex[(v >>  4) & 0xF];
  buf[3] = hex[(v      ) & 0xF];
  buf[4] = 0;
  hid.Print(buf);
}

static void printRegs()
{
  hid.Print("blink="); hid.Print(_blinkMs);
  hid.Print(" rxN="); hid.Print(_rxCount);
  hid.Print(" C1="); printHex(I2C1->CTLR1);
  hid.Print(" C2="); printHex(I2C1->CTLR2);
  hid.Print(" S1="); printHex(I2C1->STAR1);
  hid.Print(" S2="); printHex(I2C1->STAR2);
  hid.Println();
}

void setup()
{
  WebHID.begin();
  delay(5000);

  hid.Clear();
  hid.Println("i2c_slave_diag");
  hid.Println("Expected healthy:");
  hid.Println(" C1=0401 C2=0630");
  hid.Println(" OA=0066");
  hid.Println("---");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Wire.begin(MY_I2C_ADDRESS);
  Wire.onReceive(ReceiveEvent);
  Wire.onRequest(RequestEvent);

  hid.Print("After begin: ");
  printRegs();
}

void loop()
{
  // Print immediately when onReceive fires (safe: printed from loop, not ISR)
  if (_rxFlag) {
    _rxFlag = false;
    hid.Print("onReceive! _blinkMs=");
    hid.Println(_blinkMs);
  }

  // LED blink
  static uint32_t tLed = millis();
  if (millis() - tLed > _blinkMs) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    tLed = millis();
  }

  // Print registers every 2 seconds
  static uint32_t tPrint = millis();
  if (millis() - tPrint > 2000) {
    printRegs();
    tPrint = millis();
  }
}
