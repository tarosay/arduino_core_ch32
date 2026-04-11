// UIAPduino GPIO pin sequential test
// Tests board-exposed I/O pins in order:
//   0(PA1), 1(PA2), 2(PC0), 3(PC1), 4(PC2), 5(PC3),
//   6(PC4), 7(PC5), 8(PC6), 9(PC7), 10(PD0), 11(PD1),
//   12(PD2), 15(PD5/TX), 16(PD6/RX)
// Excluded: 13(PD3=D+), 14(PD4=D-), 17(PD7=RESET)
// Note: GPIO_PIN_11 (PD1) is the SWIO debug pin; debug disconnect
//       is handled automatically inside pinMode().

#include <stdio.h>

#define NUM_GPIO_PINS 15

static const uint8_t gpioPins[NUM_GPIO_PINS] = {
  GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3,
  GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7,
  GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_11,
  GPIO_PIN_12, GPIO_PIN_15, GPIO_PIN_16
};

// Send a null-terminated string via HIDuiap
static void hidPrint(const char *s) {
  HIDuiap.write((const uint8_t *)s, strlen(s));
}

// Test one pin: set OUTPUT, write HIGH then LOW, read back both.
// Returns true if both reads match expected values.
static bool testPin(uint8_t pin) {
  pinMode(pin, OUTPUT);

  digitalWrite(pin, HIGH);
  int rh = digitalRead(pin);

  digitalWrite(pin, LOW);
  int rl = digitalRead(pin);

  return (rh == HIGH && rl == LOW);
}

void setup() {
  HIDuiap.begin();
  delay(5000);

  hidPrint("=== GPIO Pin Test Start ===\n");

  for (int i = 0; i < NUM_GPIO_PINS; i++) {
    char msg[32];
    bool ok = testPin(gpioPins[i]);
    snprintf(msg, sizeof(msg), "GPIO_PIN_%02d: %s\n", gpioPins[i], ok ? "OK" : "NG");
    hidPrint(msg);
    delay(10);
  }

  hidPrint("=== GPIO Pin Test Done  ===\n");
}

void loop() {
  int failCount = 0;

  for (int i = 0; i < NUM_GPIO_PINS; i++) {
    if (!testPin(gpioPins[i])) {
      char msg[32];
      snprintf(msg, sizeof(msg), "GPIO_PIN_%02d: NG\n", gpioPins[i]);
      hidPrint(msg);
      failCount++;
    }
  }

  if (failCount == 0) {
    hidPrint("All pins OK\n");
  }

  delay(2500);
}
