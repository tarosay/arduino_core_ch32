void setup() {
  HIDuiap.begin();
  delay(5000);
  HIDuiap.write((const uint8_t*)"Hid Millis Ticker\n", 18);
}

void loop() {
  char c = '0' + ((millis() / 1000) % 10);
  HIDuiap.write((const uint8_t*)&c, 1);
  HIDuiap.write((const uint8_t*)"\n", 1);
  delay(2500);
}
