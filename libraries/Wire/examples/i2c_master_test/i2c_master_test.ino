// I2C Master Test for UIAPduino (CH32V003F4)
// Pair with i2c_slave_test running on another UIAPduino
//
// Slave address: 0x33 (i2c_slave_test.ino)
// Slave behavior:
//   receive uint32_t  -> set LED blink interval (ms)
//   request uint32_t  -> return current blink interval
//
// LED blink pattern on this master board:
//   2 blinks (200ms) -> sent fast interval (100ms) to slave
//   1 blink  (500ms) -> sent slow interval (1000ms) to slave
//   rapid 5 blinks   -> slave not found
//
// Wiring:
//   SDA: PC1 (D3) <-> PC1 (D3)
//   SCL: PC2 (D4) <-> PC2 (D4)
//   GND: GND      <-> GND
//   Pull-up: 4.7kOhm from SDA/SCL to 3.3V

#include <Wire.h>

#define SLAVE_ADDR  0x33
#define LED_PIN     2   // PC0 = LED_BUILTIN

static void led(bool on) { digitalWrite(LED_PIN, on ? HIGH : LOW); }

static void blinkN(int n, int onMs, int offMs)
{
  for (int i = 0; i < n; i++) {
    led(true);  delay(onMs);
    led(false); delay(offMs);
  }
}

// Send 32-bit blink interval to slave
static bool sendInterval(uint32_t ms)
{
  Wire.beginTransmission(SLAVE_ADDR);
  Wire.write((uint8_t *)&ms, sizeof(ms));
  return Wire.endTransmission() == 0;
}

// Read back 32-bit blink interval from slave
static uint32_t readInterval()
{
  uint32_t val = 0xFFFFFFFF;
  if (Wire.requestFrom(SLAVE_ADDR, (uint8_t)4) == 4) {
    Wire.readBytes((uint8_t *)&val, 4);
  }
  return val;
}

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  led(false);
  delay(500);
  Wire.begin();
}

void loop()
{
  // --- Phase 1: send fast blink (100ms) ---
  if (sendInterval(100)) {
    delay(200);
    uint32_t v = readInterval();
    // 2 fast blinks = success, slave blinking fast
    blinkN(2, 200, 200);
    (void)v;
  } else {
    // slave not found
    blinkN(5, 50, 50);
  }

  delay(3000);  // observe slave blinking fast

  // --- Phase 2: send slow blink (1000ms) ---
  if (sendInterval(1000)) {
    delay(200);
    uint32_t v = readInterval();
    // 1 slow blink = success, slave blinking slow
    blinkN(1, 800, 200);
    (void)v;
  } else {
    blinkN(5, 50, 50);
  }

  delay(4000);  // observe slave blinking slow
}
