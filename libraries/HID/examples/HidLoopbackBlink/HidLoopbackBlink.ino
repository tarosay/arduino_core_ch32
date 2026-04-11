//#include <stdio.h>
#define LED_BUILTIN 2

void setup() {
  HIDuiap.begin();
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  uint8_t buf[64];
  volatile int n = HIDuiap.read(buf, sizeof(buf));
  if (n > 0) {
    HIDuiap.write(buf, n);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    digitalWrite(LED_BUILTIN, LOW);
  }
}
