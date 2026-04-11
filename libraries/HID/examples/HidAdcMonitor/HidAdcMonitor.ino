const uint8_t adcPins[] = { A0, A1, A2, A3, A5 };  //A6は使えなかった
const int adcPinCount = sizeof(adcPins) / sizeof(adcPins[0]);

static void send_text(const char* s) {
  const char* p = s;
  while (*p) p++;
  HIDuiap.write((const uint8_t*)s, (int)(p - s));
}

static void send_u32(uint32_t v) {
  char buf[16];
  int n = 0;

  if (v == 0) {
    buf[n++] = '0';
  } else {
    char tmp[16];
    int t = 0;
    while (v > 0) {
      tmp[t++] = '0' + (v % 10);
      v /= 10;
    }
    while (t > 0) {
      buf[n++] = tmp[--t];
    }
  }

  HIDuiap.write((const uint8_t*)buf, n);
}

static void send_adc_name_and_value(uint8_t index, uint32_t value) {
  HIDuiap.write((const uint8_t*)"A", 1);

  char c = '0' + index;
  HIDuiap.write((const uint8_t*)&c, 1);

  HIDuiap.write((const uint8_t*)"=", 1);
  send_u32(value);
  HIDuiap.write((const uint8_t*)"\n", 1);
}

void setup() {
  HIDuiap.begin();
  delay(5000);
  HIDuiap.write((const uint8_t*)"Hid Adc Monitor\n", 16);
}

void loop() {
  for (int i = 0; i < adcPinCount; i++) {
    uint32_t v = analogRead(adcPins[i]);

    if (adcPins[i] == A0) send_adc_name_and_value(0, v);
    else if (adcPins[i] == A1) send_adc_name_and_value(1, v);
    else if (adcPins[i] == A2) send_adc_name_and_value(2, v);
    else if (adcPins[i] == A3) send_adc_name_and_value(3, v);
    else if (adcPins[i] == A5) send_adc_name_and_value(5, v);

    delay(2500);
  }

  send_text("---\n");
  delay(2500);
}