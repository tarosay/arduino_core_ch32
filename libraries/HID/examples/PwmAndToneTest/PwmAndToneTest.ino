static void send_text(const char* s) {
  const char* p = s;
  while (*p) p++;
  HIDuiap.write((const uint8_t*)s, (int)(p - s));
}

void setup() {
  HIDuiap.begin();
  delay(5000);
  send_text("Pwm And Tone Test\n");
}

void loop() {
  send_text("before analogWrite\n");
  analogWrite(0, 128);
  send_text("after analogWrite\n");
  delay(2500);

  send_text("before tone\n");
  tone(6, 1000);
  send_text("after tone\n");
  delay(2500);

  send_text("before noTone\n");
  noTone(6);
  send_text("after noTone\n");
  delay(2500);

  send_text("---\n");
  delay(2500);
}