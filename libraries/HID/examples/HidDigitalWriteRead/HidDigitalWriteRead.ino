#define LED_BUILTIN 2

void setup() {
  HIDuiap.begin();
  pinMode(LED_BUILTIN, OUTPUT);
  delay(5000);
  HIDuiap.write((const uint8_t*)"Hid Digital Write Read\n", 23);
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  int v1 = digitalRead(LED_BUILTIN);
  if (v1) {
    HIDuiap.write((const uint8_t*)"H\n", 2);
  } else {
    HIDuiap.write((const uint8_t*)"h0\n", 3);
  }
  delay(2500);

  digitalWrite(LED_BUILTIN, LOW);
  int v0 = digitalRead(LED_BUILTIN);
  if (v0) {
    HIDuiap.write((const uint8_t*)"l1\n", 3);
  } else {
    HIDuiap.write((const uint8_t*)"L\n", 2);
  }
  delay(2500);
}