void setup() {
  HIDuiap.begin();
  delay(5000);
  HIDuiap.write((const uint8_t*)"Hid Micros Ticker\n", 18);
}

void loop() {
  static uint32_t last = 0;
  uint32_t now = micros();

  if ((uint32_t)(now - last) >= 2500000UL) {
    last = now;
    HIDuiap.write((const uint8_t*)"u\n", 2);
  }
}