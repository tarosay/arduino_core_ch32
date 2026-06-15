/* analogWrite 比較用（PWMmin との Flash サイズ比較） */
void setup() {}
void loop() {
  for (int duty = 0; duty <= 255; duty++) { analogWrite(5, duty); delay(5); }
  for (int duty = 255; duty >= 0; duty--) { analogWrite(5, duty); delay(5); }
}
