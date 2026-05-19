// I2C Slave Test for UIAPduino (CH32V003F4)
// Pair with i2c_master_test running on another UIAPduino
//
// Slave address: 0x33
// Behavior:
//   receive uint32_t  -> set LED blink interval (ms)
//   request uint32_t  -> return current blink interval
//
// LED blinks at _blinkMs interval (default 500ms) from the start.
// Master can change the speed by sending a 4-byte uint32_t value.
//
// Wiring:
//   SDA: PC1 (D3) <-> PC1 (D3)
//   SCL: PC2 (D4) <-> PC2 (D4)
//   GND: GND      <-> GND
//   Pull-up: 4.7kOhm from SDA/SCL to 3.3V

#include <Wire.h>

#define MY_I2C_ADDRESS  0x33
#define LED_PIN         2   // PC0 = LED_BUILTIN on UIAPduino

static uint32_t _blinkMs = 500;  // default blink interval

void ReceiveEvent(int nNumBytes)
{
  // Wire.write(&val, 4) sends bytes in little-endian (memory) order.
  // Reconstruct with LSB at shift=0.
  uint32_t val = 0;
  for (int shift = 0; nNumBytes > 0 && shift < 32; shift += 8, nNumBytes--)
    val |= (uint32_t)(uint8_t)Wire.read() << shift;
  _blinkMs = val;
}

void RequestEvent()
{
  Wire.write((const uint8_t *)&_blinkMs, sizeof(_blinkMs));
}

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Wire.begin(MY_I2C_ADDRESS);
  Wire.onReceive(ReceiveEvent);
  Wire.onRequest(RequestEvent);
}

void loop()
{
  static uint32_t uStart = millis();
  if (millis() - uStart > _blinkMs) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    uStart = millis();
  }
}
