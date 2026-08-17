# Arduino core for UIAPduino (HID ProMicro CH32V003)

Arduino IDE で UIAPduino を使うための Arduino コアです。  
[openwch/arduino_core_ch32](https://github.com/openwch/arduino_core_ch32) を fork し、UIAPduino 向けに独自に開発・改良しています。

---

## ボードについて

### HID ProMicro CH32V003

| 項目 | 仕様 |
|------|------|
| MCU | WCH CH32V003F4（RISC-V, 48MHz） |
| Flash | 16KB |
| RAM | 2KB |
| USB | Type-C（HID デバイスとして動作） |
| GPIO | 15本（ヘッダーに引き出し）+ PD3/PD4（USB 専用） |

---

## ピン配置

| Arduino 番号 | ポート | 機能 |
|:---:|:---:|---|
| GPIO_PIN_0  | PA1 | A1 |
| GPIO_PIN_1  | PA2 | A0 |
| GPIO_PIN_2  | PC0 | **LED** |
| GPIO_PIN_3  | PC1 | SDA / **SPI SS** |
| GPIO_PIN_4  | PC2 | SCL |
| GPIO_PIN_5  | PC3 | PWM |
| GPIO_PIN_6  | PC4 | A2 / CC1（USB-C） |
| GPIO_PIN_7  | PC5 | **SPI SCK** |
| GPIO_PIN_8  | PC6 | **SPI MOSI** |
| GPIO_PIN_9  | PC7 | **SPI MISO** |
| GPIO_PIN_10 | PD0 | |
| GPIO_PIN_11 | PD1 | SWIO（要 `pinDisconnectDebug()`） |
| GPIO_PIN_12 | PD2 | A3 / CC2（USB-C） |
| GPIO_PIN_13 | PD3 | A4 / USB D+（ヘッダー無し） |
| GPIO_PIN_14 | PD4 | A7 / USB D-（ヘッダー無し） |
| GPIO_PIN_15 | PD5 | A5 / **UART TX** |
| GPIO_PIN_16 | PD6 | A6 / **UART RX** |
| GPIO_PIN_17 | PD7 | RESET（ヘッダー無し） |

> **PD1 (SWIO):** 通常の GPIO として使う場合は `setup()` 内で `pinDisconnectDebug(GPIO_PIN_11)` を1回呼んでください。

---

## インストール方法

Arduino IDE の「環境設定」→「追加のボードマネージャのURL」に以下を追加します:

```
https://github.com/tarosay/board_manager_files/raw/main/package_uiap_hid_index.json
```

「ボードマネージャ」で `UIAPduino` を検索してインストールしてください。  
`Tools > Board > UIAP_HID > HID ProMicro CH32V003` を選択します。

---

## 書き込み方法

### 1. 基板を書き込みモードにする

**基板のボタンを押しながら USB ケーブルを接続し、すぐにボタンを離します。**

これで基板がブートローダとして起動します。通常の HID デバイスではなく、
ブートローダの USB デバイスとして PC から見える状態になります。

### 2. Arduino IDE から書き込む

スケッチを開いて、いつもどおり「書き込み」を実行するだけです。

**書き込み器（WCH-LinkE など）は要りません。** USB ケーブル 1 本で書けます。

### 仕組み

`Tools > Upload method` は既定の `minichlink` のままで構いません。
`platform.txt` の recipe は次のようになっていて、プログラマの指定（`-C`）がありません。

```
tools.minichlink.upload.pattern="{path}{cmd}" -w "{build.path}/{build.project_name}.bin" flash
```

指定が無いとき minichlink は対応する書き込み先を順に探し、
**この基板のブートローダを名前で見つけます。**

```
Found WCH Link
Found ESP32S2 Programmer
Found NHC-Link042 Programmer
Found B003Fun Bootloader
Found Ardulink Programmer
Found UIAPduino Pro Micro CH32V003 V1.4 Bootloader   ← これに当たる
```

見つかったブートローダへ、ビルドした `.bin` を USB 経由で流し込みます。

`WCH-SWD`（WCH-LinkE）や `WCH-ISP` を選ぶこともできますが、
そちらは外部の書き込み器が必要です。ブートローダ経由で書けない場合の代替です。

---

## Tools メニュー

### USB（USB モード選択）

USB の動作モードを選択します。**デフォルトは WebHID Only** です。

| 選択肢 | 内容 | 主な用途 |
|--------|------|----------|
| **WebHID Only**（デフォルト） | ブラウザ（Chrome/Edge）と双方向通信 | センサーモニタ、デバッグ出力 |
| Keyboard+Mouse | USB キーボード＋マウスとして動作 | PC 操作の自動化 |
| Keyboard+Mouse+WebHID | 上記＋ブラウザ通信（EP3）を追加 | KBD/Mouse ＋ WebHID 同時使用 |
| Terminal HID | hidapitester 等のツールと通信 | PC ネイティブアプリとの連携 |
| No USB (SD log / UART only) | USB スタックを除外（約 484B 節約） | SD ログ・UART 専用スケッチ |

### U(S)ART support（シリアル選択）

UART の使い方を選択します。**デフォルトは None（UIAPSerial 推奨）** です。

| 選択肢 | 内容 | Flash コスト |
|--------|------|-------------|
| **None (use UIAPSerial)**（デフォルト） | 軽量な独自 UART ラッパーを使用 | 最小 |
| HardwareSerial (Serial / USART1) | 標準 `Serial` オブジェクトを使用 | **+約 4,748B**（未使用でも消費） |

> Flash が 16KB しかないため、SD ライブラリ等と併用する場合は **None (UIAPSerial)** を推奨します。

### Optimize（最適化レベル）

すべての選択肢で **LTO（リンク時最適化）が有効** です。Flash 節約のため LTO は必須です。

| 選択肢 | フラグ | 用途 |
|--------|--------|------|
| **Smallest (-Os) with LTO**（デフォルト） | `-Os -flto` | 通常使用 |
| Fast (-O1) with LTO | `-O1 -flto` | 速度重視 |
| Faster (-O2) with LTO | `-O2 -flto` | より速度重視 |
| Fastest (-O3) with LTO | `-O3 -flto` | 最速（Flash 増加の可能性あり） |

---

## WebHID Only モード（デフォルト）

Chrome / Edge の WebHID API を使って、UIAPduino とブラウザが直接双方向通信します。  
追加ライブラリ不要で、最も Flash を節約できます。

```cpp
#include <WebHID.h>

void setup() {
  WebHID.begin();
  delay(2000);  // USB 接続待ち
}

void loop() {
  // ブラウザからデータを受信してエコーバック
  if (WebHID.available()) {
    uint8_t buf[32];
    uint8_t len = WebHID.recv(buf, sizeof(buf));
    WebHID.send(buf, len);
  }

  // 1秒ごとにカウンタを送信
  static uint32_t last = 0;
  static uint8_t cnt = 0;
  if (millis() - last >= 1000) {
    last = millis();
    WebHID.send(cnt++, 0, 0, 0, 0, 0, 0, 0);
  }
}
```

### WebHID API

| メソッド | 説明 |
|---------|------|
| `WebHID.begin()` | USB を開始する |
| `WebHID.send(buf, len)` | Input Report でブラウザへ送信（最大 8 バイト） |
| `WebHID.send(b0,b1,...,b7)` | 個別バイト指定で送信（最大 8 バイト） |
| `WebHID.available()` | ブラウザからのデータが届いているか |
| `WebHID.recv(buf, maxlen)` | Feature Report を受信（最大 32 バイト） |

### USB エンドポイント構成（WebHID Only）

| EP | 方向 | 用途 |
|----|------|------|
| EP1 IN | UIAPduino → ブラウザ | Input Report（8 バイト） |
| EP0 Feature | ブラウザ → UIAPduino | Feature Report（最大 32 バイト） |

> **注意:** WebHID は Chrome / Edge のみ対応です。ブラウザ側の実装は [UIAPduino WebHID Lab](https://tarosay.github.io/uiap-hid-web/) を参照してください。

---

## Keyboard+Mouse モード

USB キーボード・マウスデバイスとして PC を操作します。  
`Tools > USB > Keyboard+Mouse` を選択してください。

```cpp
#include <Keyboard.h>
#include <Mouse.h>

void setup() {
  Keyboard.begin();
  Mouse.begin();
  delay(2000);
}

void loop() {
  Keyboard.print("Hello");  // キー入力
  Mouse.move(10, 0);        // マウスを右に10移動
  delay(1000);
}
```

### Keyboard API

| メソッド | 説明 |
|---------|------|
| `Keyboard.begin()` | キーボードを開始する |
| `Keyboard.print(str)` | 文字列をタイプする |
| `Keyboard.println(str)` | 文字列をタイプして改行する |
| `Keyboard.write(key)` | 1キーを押して離す（特殊キーに推奨） |
| `Keyboard.press(key)` | キーを押したままにする |
| `Keyboard.release(key)` | キーを離す |
| `Keyboard.releaseAll()` | 全キーを離す |

#### 特殊キー定数

`KEY_LEFT_ARROW` / `KEY_RIGHT_ARROW` / `KEY_UP_ARROW` / `KEY_DOWN_ARROW` /
`KEY_BACKSPACE` / `KEY_DELETE` / `KEY_RETURN` / `KEY_HOME` / `KEY_END` /
`KEY_PAGE_UP` / `KEY_PAGE_DOWN` / `KEY_ESC` / `KEY_TAB` /
`KEY_F1` 〜 `KEY_F12` など

#### 特殊キーを使う場合の注意

USB Low Speed のポーリング間隔（EP2: 10ms）により、矢印・BackSpace 等の特殊キーは
**`Keyboard.write()`** を使い、呼び出しの後に **`delay(50)`** を入れると確実に動作します。

```cpp
// ✅ 推奨
for (int i = 0; i < 6; i++) {
  Keyboard.write(KEY_LEFT_ARROW);
  delay(50);
}

// ❌ 非推奨: ポーリングを逃して無効になることがある
Keyboard.press(KEY_LEFT_ARROW);
Keyboard.releaseAll();
```

### Mouse API

| メソッド | 説明 |
|---------|------|
| `Mouse.begin()` | マウスを開始する |
| `Mouse.move(x, y, wheel)` | 相対移動（各 -127〜127） |
| `Mouse.moveLarge(x, y, wheel, steps)` | 大きな相対移動（-127〜127 超を steps 分割で送信） |
| `Mouse.press(btn)` | ボタンを押す |
| `Mouse.release(btn)` | ボタンを離す |
| `Mouse.click(btn)` | クリック |

```cpp
// 500px 右・300px 上を 20 ステップに分割して移動
Mouse.moveLarge(500, -300, 0, 20);
```

---

## Keyboard+Mouse+WebHID モード

Keyboard+Mouse 機能に加えて、WebHID（EP3）でブラウザとの双方向通信も使えます。  
`Tools > USB > Keyboard+Mouse+WebHID` を選択してください。

```cpp
#include <Keyboard.h>
#include <WebHID.h>

void setup() {
  Keyboard.begin();
  WebHID.begin();
  delay(2000);
}

void loop() {
  if (WebHID.available()) {
    uint8_t buf[32];
    uint8_t len = WebHID.recv(buf, sizeof(buf));
    // 受信内容に応じてキー操作なども可能
  }
}
```

### USB エンドポイント構成（Keyboard+Mouse+WebHID）

| EP | 方向 | 用途 |
|----|------|------|
| EP1 IN | UIAPduino → PC | マウスレポート |
| EP2 IN | UIAPduino → PC | キーボードレポート |
| EP3 IN | UIAPduino → ブラウザ | WebHID Input Report（8 バイト） |
| EP0 Feature | ブラウザ → UIAPduino | WebHID Feature Report（最大 32 バイト） |

---

## Terminal HID モード

hidapitester 等の PC ネイティブツールと HID で通信します。  
`Tools > USB > Terminal HID` を選択してください。

```cpp
void setup() {
  HIDuiap.begin();
  delay(5000);  // USB 接続待ち
}

void loop() {
  HIDuiap.write((const uint8_t*)"Hello UIAPduino\n", 16);
  delay(1000);
}
```

| メソッド | 説明 |
|---------|------|
| `HIDuiap.begin()` | HID 通信を開始する |
| `HIDuiap.write(buf, len)` | データを送信する |
| `HIDuiap.read(buf, maxlen)` | データを受信する |
| `HIDuiap.available()` | 受信データのバイト数を返す |

---

## UART / シリアル通信

### UIAPSerial（推奨・デフォルト）

Flash を節約する軽量な UART ラッパーです。`Tools > U(S)ART support > None (use UIAPSerial)` のときに使います。  
スケッチに `UIAPSerial.h` / `UIAPSerial.cpp` を同梱してください（サンプルスケッチ参照）。

```cpp
#include "UIAPSerial.h"  // スケッチと同じフォルダに配置

void setup() {
  uart.begin(9600);        // USART1 初期化（TX=PD5, RX=PD6）
}

void loop() {
  if (uart.available()) {
    uint8_t b = uart.read();
    uart.write(b);              // 1 バイト送信
  }
}
```

#### UIAPSerial API

| メソッド | 説明 |
|---------|------|
| `uart.begin(baud)` | USART1 初期化 |
| `uart.available()` | 受信バイト数 |
| `uart.read()` | 1 バイト受信 |
| `uart.write(b)` | 1 バイト送信 |
| `uart.write(buf, n)` | n バイト送信 |
| `uart.print(str)` | 文字列送信 |
| `uart.println(str)` | 文字列送信 + CRLF |

### HardwareSerial（標準 Arduino 互換）

標準の `Serial` オブジェクトを使いたい場合は `Tools > U(S)ART support > HardwareSerial (Serial / USART1)` を選択します。

> **注意:** `Serial` を一度も呼ばなくても **約 4,748B の Flash を消費します。**  
> SD ライブラリや WebHID と併用する場合は Flash が不足する可能性があります。

```cpp
void setup() {
  Serial.begin(9600);
  Serial.println("Hello UIAPduino");
}

void loop() {}
```

---

## Tone ライブラリ

`tone()` / `noTone()` で圧電ブザーや小型スピーカーに音を鳴らせます。  
タイマーチャネルが割り当てられたピンでは**ハードウェア PWM**で動作します。

```
PC4 (GPIO_PIN_6 / TIM1_CH4) --- ブザー(+) --- GND
```

```cpp
#define BUZZER_PIN GPIO_PIN_6

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  tone(BUZZER_PIN, 440);   // 440Hz でノンブロッキング再生開始
}

void loop() {
  delay(1200);
}
```

| 関数 | 説明 |
|------|------|
| `tone(pin, freq)` | 指定周波数で再生開始（ノンブロッキング） |
| `tone(pin, freq, duration)` | `duration` ms 鳴らして自動停止 |
| `noTone(pin)` | 再生を停止して LOW に戻す |

---

## Wire (I2C) ライブラリ

`Wire.h` を使って I2C マスター・スレーブ通信ができます。  
**v1.2.0 で初めて動作するようになりました**（v1.1.5 以前は完全に動作不可）。

```
SDA: PC1 (D3)  ---[4.7kΩ]--- 3.3V
SCL: PC2 (D4)  ---[4.7kΩ]--- 3.3V
```

### マスター

```cpp
#include <Wire.h>

Wire.begin();

Wire.beginTransmission(0x33);
Wire.write(data, length);
uint8_t err = Wire.endTransmission(); // 0=OK, 2=NACK, 4=timeout

uint8_t n = Wire.requestFrom(0x33, (uint8_t)4);
while (Wire.available()) {
    uint8_t b = Wire.read();
}
```

### スレーブ

```cpp
#include <Wire.h>

void onReceive(int n) {
    // ISR コンテキスト — hid.Print() 等のブロッキング呼び出し禁止
    while (n--) Wire.read();
}

void onRequest() {
    Wire.write(data, length);
}

Wire.begin(0x33);
Wire.onReceive(onReceive);
Wire.onRequest(onRequest);
```

> **注意:** `onReceive` / `onRequest` は割り込みハンドラから呼ばれます。  
> `hid.Print()` 等は呼ばず、フラグを立てて `loop()` から出力してください。

詳細は `libraries/Wire/README.md` を参照してください。

---

## Wiremin ライブラリ

Wire.h の代替となる最小限の I2C ドライバです。Flash 使用量を大幅に削減します。

### Flash サイズ比較

| ライブラリ | Flash | RAM |
|---|---|---|
| Wire.h | 10,620 バイト（64%） | 592 バイト |
| Wiremin.h | 4,444 バイト（27%） | 428 バイト |
| **削減量** | **▲ 6,176 バイト** | **▲ 164 バイト** |

### API

```cpp
#include <Wiremin.h>

// マスター
Wiremin_begin();                              // 100kHz で初期化
Wiremin_begin_fast();                        // 400kHz で初期化
Wiremin_write(addr7, data, len);             // バイト列送信
Wiremin_read(addr7, buf, len);              // バイト列受信
Wiremin_write_reg(addr7, reg, data, len);   // レジスタ書き込み（アドレス 8bit）
Wiremin_read_reg(addr7, reg, buf, len);     // レジスタ読み出し（アドレス 8bit・repeated START）
Wiremin_write_reg16(addr7, reg, data, len); // 同上、アドレス 16bit（EEPROM 等）
Wiremin_read_reg16(addr7, reg, buf, len);   // 同上、アドレス 16bit（repeated START）
Wiremin_probe(addr7);                       // ACK 確認（スキャン用）

// スレーブ
Wiremin_slave_begin(addr7);  // スレーブとして初期化
Wiremin_slave_set(reg, val); // 共有レジスタに書き込む
Wiremin_slave_get(reg);      // 共有レジスタを読み出す
```

### Wire.h からの移植

| Wire.h | Wiremin.h |
|---|---|
| `Wire.begin()` | `Wiremin_begin()` |
| `Wire.beginTransmission(addr)` + `Wire.write(reg)` + `Wire.write(data, len)` + `Wire.endTransmission()` | `Wiremin_write_reg(addr, reg, data, len)` |
| `Wire.beginTransmission(addr)` + `Wire.endTransmission(false)` + `Wire.requestFrom(addr, len)` + `Wire.read()` × n | `Wiremin_read_reg(addr, reg, buf, len)` |
| `Wire.beginTransmission(addr)` + `Wire.endTransmission()` → 0=ACK | `Wiremin_probe(addr)` |

**OLED（SSD1306）例:**

```cpp
// コマンド送信
uint8_t cmd = 0xAE;
Wiremin_write_reg(0x3C, 0x00, &cmd, 1);

// データ送信（128バイト/ページ）
Wiremin_write_reg(0x3C, 0x40, buf, 128);
```

**EEPROM（24FC256）例:**

メモリアドレスが 16bit のデバイスには `_reg16` 版を使います。

```cpp
// 0x0040 番地から 64 バイト書き込む
Wiremin_write_reg16(0x50, 0x0040, buf, 64);

// 0x0040 番地から 64 バイト読み出す
Wiremin_read_reg16(0x50, 0x0040, buf, 64);
```

> **ページ境界と書き込み待ちはスケッチ側の担当です。** EEPROM はページ
> （24FC256 なら 64 バイト）を越えた分をそのページの先頭に折り返して上書きします。
> また書き込み中（最大 5 ms）はアドレスに応答しないため、`Wiremin_probe()` が
> ACK を返すまで待ってから次の書き込みへ進んでください。
> 実装はスケッチ例 `Wiremin_EEPROM_24FC256` を参照してください。

> **注意:** Wire.h と同時に使用不可。スレーブモードでは I2C センサを同時接続不可（I2C1 が1つのみ）。

---

## PWMmin ライブラリ

CH32V003 専用の軽量 PWM ライブラリです。`analogWrite()` の代わりに TIM1・TIM2 を直接制御し、Flash 使用量を最小化します。ヘッダーオンリーで、未使用の関数は LTO によりビルド時に自動削除されます。

### CH32V003 では `analogWrite()` を使わないでください

`analogWrite()` は `HardwareTimer` を丸ごと引き込むため、16KB Flash の CH32V003 には重すぎます。スケッチ例 `PwmAndToneTest` を `analogWrite()` から `Pwm_write()` に置き換えた実測値です。

| | Flash | グローバル変数 |
|------|-------|--------------|
| `analogWrite()` | 8,728 B (53%) | 696 B (33%) |
| `Pwm_write()` | 6,496 B (39%) | 380 B (18%) |
| 差 | **−2,232 B** | **−316 B** |

さらに以下の制限があります。

| 症状 | 原因 |
|------|------|
| TIM1 と TIM2 の両方に `analogWrite()` すると無言でフリーズする | `HardwareTimer` 1 個が 144 バイト消費するのに対し、`operator new` のプールが 256 バイトしかない。2 個目の確保に失敗し、`operator new` が無限ループに入る |
| `analogWrite()` → `pinMode()` → `analogWrite()` と往復するたびに RAM が減る | `pinMode()` が `pwm_stop()` → `delete` を呼ぶが `operator delete` は領域を回収しない。次の `analogWrite()` がさらに 136 バイト消費する |
| 同じタイマーのピンで `analogWrite()` と `tone()` を併用すると PWM が崩れる | `tone()` は鳴らす周波数に合わせてタイマーの PSC / ATRLR を書き換えるため |

PWMmin は動的確保を一切しないため、いずれの問題も起きません。

| 用途 | 使う関数 |
|------|---------|
| PWM 出力 | `Pwm_write(pin, duty)` |
| 周波数変更 | `Pwm_freq()` / `Pwm_freq_TIM1()` / `Pwm_freq_TIM2()` |
| PWM 停止 | `Pwm_stop(pin)` |
| ブザー | `Pwm_tone()`、または Tone ライブラリの `tone()` / `noTone()` |

`tone()` は `HardwareTimer` を使わないので上記の問題はありません。ただし PWM とは**別のタイマーのピン**を選んでください（TIM1 = pin 0/5/6/12、TIM2 = pin 2 または 3/9/15/16）。

### Tools > PWM 設定

| 設定 | TIM2 使用ピン |
|------|--------------|
| **TIM2 Default**（デフォルト） | pin 2 (PC0 / TIM2-CH3) |
| **TIM2 Remap3** | pin 3 (PC1), 9 (PC7), 15 (PD5), 16 (PD6) |

TIM1 ピン（pin 0 / 5 / 6 / 12）はどちらの設定でも使用できます。

### API

```cpp
#include <PWMmin.h>
PWMMIN_REQUIRE_DEFAULT();  // または PWMMIN_REQUIRE_REMAP3()
```

| 関数 | 説明 |
|------|------|
| `Pwm_write(pin, duty)` | PWM 出力（duty: 0=0%, 128=50%, 255=100%） |
| `Pwm_freq(hz)` | TIM1・TIM2 両方の周波数を設定（デフォルト 1000Hz） |
| `Pwm_freq_TIM1(hz)` | TIM1 のみ周波数設定 |
| `Pwm_freq_TIM2(hz)` | TIM2 のみ周波数設定 |
| `Pwm_stop(pin)` | PWM 停止・ピンを入力フロートに復元 |
| `Pwm_tone(pin, hz, ms)` | ノンブロッキング tone（ms 経過後に自動停止） |
| `Pwm_tone_update()` | Pwm_tone の停止処理を実行（loop() で定期呼び出し） |
| `Pwm_servo_begin()` | TIM1・TIM2 を 500Hz（サーボ用）に設定 |
| `Pwm_servo_begin_TIM1()` | TIM1 のみ 500Hz に設定 |
| `Pwm_servo_begin_TIM2()` | TIM2 のみ 500Hz に設定 |
| `Pwm_servo(pin, angle)` | サーボ角度制御（0〜180°） |

### サンプルスケッチ

`ファイル` → `スケッチ例` → `PWMmin` から開けます。

| スケッチ | 設定 | 内容 |
|----------|------|------|
| PWMminBasic | TIM2 Default | 基本 API・5ピン（pin 0/2/5/6/12）確認 |
| PWMminRemap3 | TIM2 Remap3 | 基本 API・7ピン（pin 0/5/6/9/12/15/16）確認 |
| PWMminTone | TIM2 Default | ブザー2個で全 API 確認（ドレミ・和音・交互・音量スイープ） |
| PWMminServo | TIM2 Default | サーボ2軸制御（1軸・2軸逆方向） |
| PWMminServoRemap3 | TIM2 Remap3 | サーボ2軸制御（Remap3 ピン使用） |

### 誤設定の検出

スケッチの先頭に記述することで、Tools > PWM の設定が合っていない場合にコンパイルエラーを出します。

```cpp
PWMMIN_REQUIRE_DEFAULT();  // TIM2 Default 必須のスケッチ
PWMMIN_REQUIRE_REMAP3();   // TIM2 Remap3 必須のスケッチ
```

---

## SDmin ライブラリ

Flash サイズ節約に特化した最小限の FAT32 SD ライブラリです。  
LFN（最大 26 文字のファイル名）とサブディレクトリに対応しています。  
`Tools > USB > No USB (SD log / UART only)` と組み合わせると Flash を最大限節約できます。

```cpp
#include "SDmin.h"

#define PIN_SS GPIO_PIN_3  // PC1

void setup() {
  sm_init(PIN_SS);
  sm_mount();
}
```

### SDmin API

| 関数 | 説明 |
|------|------|
| `sm_init(cs)` | SPI + SD カード初期化 |
| `sm_mount()` | FAT32 パーティションのマウント |
| `sm_open_w(path)` | ファイルを書き込み用にオープン（上書き） |
| `sm_open_a(path)` | ファイルを追記用にオープン（存在しない場合は新規作成） |
| `sm_write(buf, len)` | データを書き込み（最大 255 バイト） |
| `sm_sync_w()` | 現在のセクタをフラッシュしディレクトリのファイルサイズを更新（ファイルは開いたまま） |
| `sm_close_w()` | ファイルサイズを更新してクローズ |
| `sm_open_r(path)` | ファイルを読み込み用にオープン |
| `sm_read(buf, len)` | データを読み込み（最大 255 バイト） |
| `sm_seek(pos)` | 読み位置を pos へ移動（読み込みオープン中のみ） |
| `sm_write_at(path, pos, buf, len)` | 既存ファイルの途中を部分上書き（1 セクタ内・サイズ拡張なし・非オープン時のみ） |
| `sm_close_r()` | ファイルクローズ |
| `sm_del(path)` | ファイルを削除 |
| `sm_rmdir(path)` | 空のディレクトリを削除 |
| `sm_mkdir(path)` | ディレクトリを作成 |
| `sm_list_open(ctx, path)` | ディレクトリ列挙を開始（`path=""` / `NULL` = ルート） |
| `sm_list_next(ctx, name)` | 次のエントリを取得（戻り値: 1=ファイル / 2=ディレクトリ / 0=終端） |

> **パス指定**: `"FILE.TXT"` または `"DIR/FILE.TXT"` 形式のサブディレクトリパスを指定できます。

> **Flash サイズ**: LTO が有効なため、未使用の関数はビルド時に自動削除されます。  
> 全関数を使用した場合の増加量は約 5KB 程度です。

---

## NeoPixelmin ライブラリ

Adafruit_NeoPixel.h の代替となる最小限の WS2812B（NeoPixel）ドライバです。  
波形を GPIO のサイクル数え打ちではなく **SPI1 で生成する**ため、`show()` 中に割り込みを止める必要がありません。ソフトウェア USB を使うこのボードでは、これが効きます。

### Flash サイズ比較

LED 12 個のリング制御スケッチ（`UIAP_HID:ch32v:CH32V003` 既定オプション）で実測。

| ライブラリ | Flash | RAM |
|---|---|---|
| Adafruit_NeoPixel 1.15.5 | 8,500 バイト（52%） | 276 バイト |
| NeoPixelmin.h | 3,532 バイト（21%） | 212 バイト |
| **削減量** | **▲ 4,968 バイト** | **▲ 64 バイト** |

> Adafruit 版はピクセルバッファを `malloc()` で確保するため、ビルド結果の「グローバル変数」に  
> 表示される 232 バイトにバッファが含まれません。上表の 276 バイトは、払い出し 36 バイトと  
> ヒープのブロックヘッダ 8 バイトを加えた実使用量です。RAM 差が小さいのは、削っているのが  
> ヒープ管理のオーバーヘッドだけだからです（バッファ 36 バイト自体はどちらも必要）。

### 配線

**DIN は pin 8（PC6 / SPI1 MOSI）に固定です。** SCK・MISO・NSS は使いません（SPI1 を 1 線送信専用モードで動かすため、消費するのは PC6 だけです）。`SPI.h` および `SDmin` とは同時に使えません。

### API

```cpp
#define LED_COUNT 12
#define NEOPIXELMIN_MAX_LEDS LED_COUNT   // 定義必須。未定義はコンパイルエラー
#include <NeoPixelmin.h>

NeoPixelmin pixels(LED_COUNT, NEOPIXELMIN_PIN, NEO_GRB + NEO_KHZ800);

pixels.begin();                          // SPI1 / PC6 初期化
pixels.show();                           // バッファをテープへ送信
pixels.clear();                          // 全消灯（送信はしない）
pixels.setPixelColor(i, color);          // color は Color() の戻り値
pixels.setPixelColor(i, r, g, b);
pixels.getPixelColor(i);                 // 0x00RRGGBB（明るさ補正前）
pixels.fill(color, first, count);        // count 0 = 末尾まで
pixels.setBrightness(b);                 // 0〜255（バッファは破壊しない）
pixels.numPixels();                      // 確保できた個数
pixels.ok();                             // 個数超過・ピン誤りなら false
NeoPixelmin::Color(r, g, b);             // 0x00RRGGBB に詰める
```

`NEO_RGB` / `NEO_GRB` / `NEO_BRG` などの色順指定は Adafruit_NeoPixel と同じ値です。

### 設定マクロ

`#include` より前に定義します。

| マクロ | 既定 | 説明 |
|---|---|---|
| `NEOPIXELMIN_MAX_LEDS` | **なし（必須）** | ピクセルバッファの個数。1 個 3 バイト |
| `NEOPIXELMIN_RESET_US` | 300 | フレーム前に線を Low に保つ時間（µs）。`SysTick` で測る |
| `NEOPIXELMIN_T0H_BITS` | 2 | "0" の High 幅（SPI ビット数・333ns） |
| `NEOPIXELMIN_T1H_BITS` | 5 | "1" の High 幅（SPI ビット数・833ns） |
| `NEOPIXELMIN_ATOMIC` | 未定義 | 定義すると `show()` 中の割り込みを禁止 |

`NEOPIXELMIN_MAX_LEDS` に既定値はありません。小さすぎれば黙って切り捨て、大きすぎれば RAM を無駄にし、どちらも実機に載せるまで気づけないためです。

**リセット待ちは `micros()` ではなく `SysTick->CNT` で測っています。** CH32V003 の `micros()` は
`millis()` と同じ `uint64_t` の除算を引き込むため、この待ちのためだけに Flash が **2,224 バイト**
増えます（実測）。ドライバ本体の 2.6 倍です。`SysTick->CNT` は `micros()` 自身が読んでいる
free-running カウンタなので、µs へ割らずカウントのまま比べれば、同じものを測りながら除算だけが消えます。
32bit の折り返し（48MHz で約 89 秒）に当たった場合の影響は、`show()` が 1 回分のリセット時間
（既定 300µs）を余分に待つことだけです。折り返しに届いた時点で 300µs はとうに過ぎているので、
待ちが短くなることはありません。

### Adafruit_NeoPixel.h からの移植

| Adafruit_NeoPixel.h | NeoPixelmin.h |
|---|---|
| `Adafruit_NeoPixel pixels(n, pin, type);` | `NeoPixelmin pixels(n, NEOPIXELMIN_PIN, type);` |
| （バッファは自動確保） | `#define NEOPIXELMIN_MAX_LEDS n` が必要 |
| 任意のピンに出力できる | **pin 8（PC6）固定** |
| `gamma8()` / `ColorHSV()` / RGBW / `NEO_KHZ400` | 非対応 |

`begin` / `show` / `clear` / `setPixelColor` / `getPixelColor` / `fill` / `setBrightness` / `Color` / `numPixels` は同じ意味で動きます。`setBrightness()` の計算式も Adafruit と同一のため、見た目の明るさも変わりません。

> **必要なクロック**: SPI を 6MHz にできる必要があるため、`Tools > Clock Select` は 48MHz / 24MHz / 12MHz のいずれかにしてください。それ以外はコンパイルエラーになります。

---

## コアの独自改良点

### `pinDisconnectDebug(uint32_t pin)`

PD1（SWIO デバッグピン）を通常の GPIO として使えるようにします。

```cpp
void setup() {
  pinDisconnectDebug(GPIO_PIN_11);  // PD1 を通常 GPIO に切り替え
  pinMode(GPIO_PIN_11, OUTPUT);
}
```

### `GPIO_PIN_N` マクロ

CH32 ペリフェラルライブラリの `GPIO_Pin_N`（ビットマスク）と区別するため、大文字の `GPIO_PIN_N` を採用しています。

```cpp
pinMode(GPIO_PIN_6, OUTPUT);    // PC4 を出力に設定
digitalWrite(GPIO_PIN_6, HIGH);
```

---

## サンプルスケッチ

### HID 共通（WebHID / Terminal HID）

`ファイル` → `スケッチ例` → `HID` から開けます。

| スケッチ | 内容 |
|---------|------|
| Blink | LED (PC0) を点滅 |
| GPIO_test | 全 GPIO を順番にテスト |
| HidDigitalWriteRead | GPIO の書き込み・読み返しを HID で確認 |
| HidLoopbackBlink | HID 受信データに応じて LED を制御 |
| HidMillisTicker | `millis()` の値を定期送信 |
| HidMicrosTicker | `micros()` の値を定期送信 |
| HidAdcMonitor | ADC 値を定期送信 |
| PwmAndToneTest | PWM / Tone の動作確認 |

### Keyboard / Mouse

`ファイル` → `スケッチ例` → `Keyboard` / `Mouse` から開けます。

| スケッチ | 内容 |
|---------|------|
| KbdMouseTest | キー入力とマウス移動のサンプル |
| KeyboardPractice | キーボード HID 練習（Step ごとにコメントを外す） |
| KeyboardSwitch | キーボード HID 練習（switch 文で Step 切り替え） |

> **KeyboardPractice / KeyboardSwitch** は [UIAPduino WebHID Lab](https://tarosay.github.io/uiap-hid-web/) の練習ページと連携して使えます。

### WebHID

`ファイル` → `スケッチ例` → `WebHID` から開けます。

| スケッチ | 内容 |
|---------|------|
| WebHIDTest | エコーバック＋1秒ごとカウンタ送信 |

### Tone

`ファイル` → `スケッチ例` → `Tone` から開けます。

| スケッチ | 内容 |
|---------|------|
| ToneBasic | 440Hz をノンブロッキングで鳴らし続ける |
| ToneDuration | ドレミスケールを順番に演奏 |
| ToneNoTone | `tone()` / `noTone()` を繰り返す停止・再開 |

### Wire (I2C)

`ファイル` → `スケッチ例` → `Wire` から開けます。

| スケッチ | 内容 |
|---------|------|
| i2c_scanner | 全 I2C アドレスをスキャンして WebHID に表示 |
| i2c_slave_test | スレーブ: LED 点滅間隔を受信・返答 |
| i2c_master_test | マスター: スレーブに点滅間隔を書き込み・読み返し |
| i2c_BMP280_test | BMP280 センサの温度・気圧を 250ms ごとに WebHID へ出力 |

### Wiremin

`ファイル` → `スケッチ例` → `Wiremin` から開けます。

| スケッチ | 内容 |
|---------|------|
| Wiremin_scanner | 全 I2C アドレスをスキャンして WebHID に表示 |
| Wiremin_slave_test | スレーブ: LED 点滅間隔を受信・返答 |
| Wiremin_master_test | マスター: スレーブに点滅間隔を書き込み・読み返し |
| Wiremin_BMP280 | BMP280 センサの温度・気圧を WebHID へ出力 |
| Wiremin_bmi270 | BMI270 6軸 IMU の加速度を WebHID へ出力 |
| Wiremin_EEPROM_24FC256 | I2C EEPROM 24FC256 の読み書きテスト（16bit アドレス・ページ境界・不揮発の確認） |
| Wiremin_size_test | Flash サイズ計測用（Wire.h との比較に使用） |

### SDmin

`ファイル` → `スケッチ例` → `SDmin` から開けます。  
`Tools > USB > No USB (SD log / UART only)` を選択してビルドしてください。

| スケッチ | 内容 |
|---------|------|
| SDLog | UART RX で受信したデータをマイクロSD カードに記録する OpenLog 互換ロガー |
| SeekWriteAt | ランダムアクセス API（`sm_seek` / `sm_write_at`）の最小デモ。TEST.TXT の途中だけを部分上書きし LED で検証結果を表示（v1.2.5） |

### NeoPixelmin

`ファイル` → `スケッチ例` → `NeoPixelmin` から開けます。  
WS2812B の DIN を pin 8（PC6）に接続してください。

| スケッチ | 内容 |
|---------|------|
| NeoPixelmin_ring | WS2812B 12 連リングを赤・緑・青で順に流し、全体をフェードさせるデモ |

---

## 対応 OS

| OS | 状況 |
|----|------|
| Windows | 動作確認済み（Arduino IDE 2.0 以上） |
| Linux | 動作確認中 |
| macOS | 動作確認中 |

---

## 更新履歴

### v1.2.13（最新）

- **NeoPixelmin に `getPixels()` を追加** — 画素バッファへの参照を返す。追加のみで、既存 API の変更はない
  - 回転や減光は、やりたいことが**バイト列の操作**であって色として解釈し直す必要がない。それを `getPixelColor()` で 32bit に組み立て `setPixelColor()` で分解し直すと、往復だけで数百バイトの Flash を使う。16KB しかないこの石ではそれが効く
  - UIAPruby EE の配布ファームが **16,476 バイト**となり上限 16,384 を 92 バイト超過して入らなくなったのが発端。`SHIFT` が 372 → 116 バイト、`DIM` が 300 → 124 バイトになり、上限内に収まった
  - シグネチャは Adafruit_NeoPixel と同じ `uint8_t *getPixels(void) const`。const メソッドから非 const ポインタを返す形も本家に合わせている。非 const 版と**生成コードは同一**（同じスケッチで 3,356 バイトで一致）
  - ヘッダオンリーの inline メソッド 1 つなので、使わなければコードは生成されない。`NeoPixelmin_ring` は追加前後で **Flash 3,532 バイト / RAM 212 バイトのまま変化なし**
  - 使うときの約束は 3 つ。**バイト並びは色順に従う**（`NEO_GRB` なら G, R, B の順で、`getPixelColor()` のような R, G, B への並べ替えはされない）、**入っているのは輝度スケール前の素の値**（`setBrightness()` は非破壊で、スケーリングは `show()` の中）、**境界チェックは無い**（有効なのは `numPixels() * 3` バイトまで）
- **スケッチ例 `NeoPixelmin_getPixels` を追加** — `getPixels()` でバッファを直接さわるデモ（Flash 3,768 バイト / 22%）
  - 前半で 0〜5 を `setPixelColor`、6〜11 を `getPixels` で同じ色に塗り、**両者が一致すること**を目で確かめられるようにした。バイト順を取り違えると右半分だけ色が変わるので、すぐ分かる
  - 後半は回転と減光を、実際に使う形のバイト操作で行う
  - 実機の 12 連リングで 4 パターンとも動作確認済み

### v1.2.12

- **NeoPixelmin のリセット待ちを `micros()` から `SysTick->CNT` に変更** — Flash を **2,224 バイト**削減
  - `micros()` は `millis()` と同じ `uint64_t` の除算を引き込む。フレーム間のリセット待ち 1 か所のためだけに、ドライバ本体（836 バイト）の 2.6 倍の Flash を使っていた
  - `SysTick->CNT` は `micros()` 自身が読んでいる free-running カウンタ。µs へ割らずカウントのまま比べるので、測っているものは変わらず除算だけが消える
  - `NEOPIXELMIN_RESET_US` はこれまでどおり µs で指定する（内部で `_NP_RESET_TICKS` に換算）
  - 32bit の折り返し（48MHz で約 89 秒）に当たっても、`show()` がリセット時間 1 回分を余分に待つだけで、待ちが短くなることはない
  - `NeoPixelmin_ring` で **Flash 5,932 → 3,532 バイト（36% → 21%）**、RAM 220 → 212 バイト。実機の 12 連リングで動作確認済み
- **スケッチ例 `NeoPixelmin_ring` から `sketch.yaml` を削除** — Tools メニューの設定は `.ino` の先頭コメントに移した
  - `sketch.yaml` があると arduino-cli も IDE も profile build に入り、**インストール済みのコアを見なくなる**。ライブラリを手元で直して実機で試すとき、直す前のコードが使われてしまう

### v1.2.11

- **NeoPixelmin ライブラリを追加** — WS2812B（NeoPixel）を SPI1 で駆動する最小限のドライバ
  - 波形を GPIO のサイクル数え打ちではなく **SPI1 で生成**する。1 ビット = 1 SPI バイト（6MHz）で `"0"` = `0b11000000`、`"1"` = `0b11111000`。High 幅は 333ns / 833ns で WS2812B の規定内
  - このため **`show()` 中に割り込みを止めない**。Adafruit_NeoPixel の CH32 実装は `noInterrupts()` で囲むため 12 個で 7.7ms 割り込みが停止するが、ソフトウェア USB を使うこのボードではそれが問題になる
  - LED 12 個で **Flash 8,500 → 5,932 バイト（▲2,568）**、実 RAM 276 → 220 バイト（▲56）
  - `begin` / `show` / `clear` / `setPixelColor` / `getPixelColor` / `fill` / `setBrightness` / `Color` / `numPixels` は Adafruit_NeoPixel と同じ意味で動く。`setBrightness()` の計算式も同一
  - **DIN は pin 8（PC6 / SPI1 MOSI）固定。** `SPI.h` / `SDmin` とは併用不可
  - `NEOPIXELMIN_MAX_LEDS` は**定義必須**（未定義はコンパイルエラー）。既定値を置くと、小さすぎれば黙って切り捨て、大きすぎれば RAM を無駄にし、どちらも実機まで気づけないため
  - **SPI1 はアイドル時に MOSI を High にする**ため、PC6 は普段 GPIO 出力の Low で保持し、データを流す間だけ SPI に渡している。これをしないとフレーム間がリセットにならず、先頭の LED だけが表示されない（後段は先頭 LED が整形し直した信号を見るため正常に光ってしまい、原因が分かりにくい）
  - 非対応: RGBW / SK6812、`NEO_KHZ400`、`gamma8()`、`ColorHSV()`
- **スケッチ例 `NeoPixelmin_ring` を追加** — WS2812B 12 連リングのデモ（Flash 5,932 バイト / 36%）

### v1.2.10

- **Wiremin に 16bit アドレス版の API を追加** — `Wiremin_write_reg16()` / `Wiremin_read_reg16()`
  - メモリアドレスが 16bit の I2C EEPROM（24FC256 など）を、スケッチ側でアドレス 2 バイトを組み立てずに読み書きできる
  - 特に読み出しは「アドレス送信 → repeated START → 読み出し」を STOP を挟まずに繋ぐ必要があり、従来は既存 API の組み合わせでは書けなかった
- **マスタ転送の実装を 1 本に統合**（`libraries/Wiremin/Wiremin.h`）
  - `write` / `read` / `write_reg` / `read_reg` / `_reg16` がすべて内部の `_wm_xfer()` を呼ぶだけになった。START/ADDR/TXE/BTF/STOP の手順が 1 箇所にまとまり、アドレス長は 0〜4 バイトで指定する形になった
  - スケッチ例 9 本の合計で **▲40 バイト**。最も Flash が厳しい `Wiremin_bmi270` は 15,696 → **15,616 バイト**（95%）
  - **挙動変更**: `Wiremin_read(addr, buf, 0)` の戻り値が `true` → `false`（読むものがないためバスを操作しない）
- **スケッチ例 `Wiremin_EEPROM_24FC256` を追加** — 24FC256（32KB）の書き込み・読み出しテスト
  - 16bit アドレス、64 バイトのページ境界をまたぐ分割、ACK ポーリングによる書き込み完了待ち、シーケンシャル読み出し、最終アドレス 0x7FFF、電源断をまたぐ保持までを実機で確認（Flash 5,996 バイト / 36%）
  - ページ境界の分割と書き込み待ちはスケッチ側の担当。Wiremin はバス転送のみを行う
- **README に「書き込み方法」を追記** — 基板のボタンを押しながら USB を接続してブートローダで起動し、そのまま IDE から書き込む手順（書き込み器不要）

### v1.2.9

- **HardwareSerial に USART 受信割り込みとリングバッファを追加** — `Serial.available()` / `read()` / `peek()` が使えるようになった（既定 64 バイト、`SERIAL_RX_BUFFER_SIZE` で変更可）
- **`realloc()` が内容を保持しない問題を修正**（`cores/arduino/ch32_heap.c`）
  - 新しいブロックを返すだけで旧データをコピーしていなかった。`String` の連結（`s += ...`）でバッファが伸びるたびに先頭がゴミになり、`Serial.println(s)` が壊れた文字列を出力していた
  - `free()` が領域を回収するようになった（LIFO 順で巻き戻し）。`loop()` 内で `String` を使ってもヒープが枯渇しない
  - 最新ブロックの `realloc()` はコピーせずその場で拡張する
  - `calloc()` の乗算オーバーフロー、確保上限判定のポインタ演算オーバーフロー（未定義動作）、不正ポインタの `free()` も修正
- **`dtostrf()` の修正**（`cores/arduino/avr/dtostrf.c`）
  - `strdup()` を呼んでいたが、このコアは `-nostdlib` で `strdup` が存在しない。`String(1.5)` などを書くと `undefined reference to 'strdup'` でリンクに失敗していた（未使用時のみ `--gc-sections` で消えて通っていた）
  - 小数の先頭ゼロが落ちていた（`dtostrf(1.05, 0, 2)` → `"1.5"`）
  - 負の値が 0.x に丸まると符号が消えていた（`dtostrf(-0.25, 0, 2)` → `"0.25"`）
  - 129 バイトのスタックバッファと `sprintf` 依存を削除
  - **挙動変更**: `prec == 0` のとき小数点を付けなくなった（`"1.0"` → `"1"`。avr-libc に準拠）
- **`pwm_start()` / `pwm_stop()` の NULL 参照を修正**（`cores/arduino/ch32/analog.cpp`） — `HardwareTimer_Handle[index]` が NULL のときにその先へ代入していた。C++17 未満では代入の評価順が未規定のため、コンパイラ次第で NULL 参照になる
- **CH32V003 で `analogWrite()` を非推奨に** — 理由と代替を README に明記し、スケッチ例 `PwmAndToneTest` を PWMmin (`Pwm_write`) に書き換え

### v1.2.8

- **Zicsr 拡張の明示指定に対応** — `noInterrupts()` / `interrupts()` を使うライブラリでビルドが通らない問題を修正
  - 現行の RISC-V 仕様では `csrr` / `csrw` などの CSR 命令が `Zicsr` 拡張に分離されており、GCC 11 以降は `-march=rv32ec` のままだとアセンブラが認識できず `Error: unrecognized opcode 'csrr ...', extension 'zicsr' required` になっていた
  - 各 CSR 命令の直前に `.option arch, +zicsr` を挿入するマクロ（`ADD_ARCH_ZICSR`）を追加。`cores/arduino/ch32fun.h` で既に使われていた手法に合わせた
  - **`-march` は変更していない**ため、既存の割り込み設定・ABI・コード生成には影響なし
  - 対象は全 7 MCU シリーズ（CH32V00x / V10x / V20x / V30x / L10x / VM00X / X035）の `core_riscv.h` / `core_riscv.c` / `*_dbgmcu.c` 計 19 ファイル
  - Adafruit NeoPixel の `show()` が `noInterrupts()` を呼ぶため、LTO 展開時に初めて表面化していた。CH32V003 + Adafruit NeoPixel 1.15.5 で動作確認済み（Flash 8,168 バイト / 49%）

### v1.2.7

- **フォーク元（YuukiUmeta-UIAP/arduino_core_ch32）の main をマージ** — openwch 本家の以下の修正を取り込み
  - **Print: `print(0)` が `"0"` を出力するように修正**（本家 #189）  
    従来は 0 を print すると空文字になっていた（ArduinoCore-API #178 と同件）
  - **platform.txt: Arduino IDE 1.8.x 互換の修正**  
    minichlink の Windows パス区切りを `\` → `/` に修正、`upload.params.verbose` / `upload.params.quiet` を追加
  - **PeripheralPins.c（CH32V003F4）: ADC1_IN6 / ADC1_IN7 のピン割り当てを修正**（本家 #160）  
    誤って PA6 / PA4 になっていたものを PD6 / PD4 に修正（analogRead の A6 / A7 が正しいピンで動作する）
  - **tools/platformio-build.py 更新**（本家 #147、Arduino IDE でのビルドには影響なし）
- **マージ後の全ビルド確認済み** — メンテ対象の全スケッチ例 40 本がビルド成功（16KB Flash 内）。退行なし  
  ※ SPI/SPIFlash は openwch 本家由来の残置サンプルで、`Serial.printf`（`vdprintf`）が ch32fun ランタイムに無いため従来からビルド不可（メンテ対象外）

### v1.2.6

- **PWMmin ライブラリ追加** — CH32V003 専用の軽量 PWM ライブラリ（ヘッダーオンリー）
  - TIM1・TIM2 独立周波数制御（`Pwm_freq_TIM1` / `Pwm_freq_TIM2`）
  - ノンブロッキング tone 相当（`Pwm_tone` / `Pwm_tone_update`）
  - サーボ制御 API（`Pwm_servo_begin` / `Pwm_servo`）。500Hz・duty 128〜255 で 0°〜180°
  - 誤設定コンパイルエラーマクロ（`PWMMIN_REQUIRE_DEFAULT` / `PWMMIN_REQUIRE_REMAP3`）
  - サンプルスケッチ 5 つ（Basic / Remap3 / Tone / Servo / ServoRemap3）
- **Tools > PWM メニュー追加** — TIM2 Default（pin 2）/ TIM2 Remap3（pin 9/15/16）を切り替え
- **USB VID/PID 注記追加** — `usb_config.h` コメントと README に「開発・評価用」と明記。製造・配布・販売時は正規 VID/PID を設定するよう案内

### v1.2.5

- **SDmin: `sm_seek(pos)` 追加** — 読み取りオープン中のファイルの読み位置を任意位置へ移動（ランダムアクセス読み）。TinyVM のジャンプ処理などで「開き直して先頭から読み飛ばす」必要がなくなる
- **SDmin: `sm_write_at(path, pos, buf, len)` 追加** — 既存ファイルの途中だけを部分上書き（セクタ単位の読み・修正・書き戻し）。ファイル作り直しが不要になり、高速かつ FAT・ディレクトリを書き換えないため SD カードの摩耗が少ない。1 セクタ内限定・サイズ拡張なし
- 追加 Flash コスト: 両関数で約 +312 バイト（未使用ならリンク時に自動削除され 0）
- **SDmin: SeekWriteAt サンプルスケッチ追加** — 新 API の最小デモ（ファイル作成 → seek 読み → 部分上書き → 検証、結果は LED と TEST.TXT で確認）

### v1.2.4

- **Wiremin ライブラリ追加** — Wire.h の代替となる最小 I2C ドライバ。Flash を **▲6,176 バイト**削減。BMI270（6軸 IMU）が 16KB Flash 内で動作確認済み（15,728 バイト）
- **HcSr04 ライブラリ追加** — HC-SR04 超音波距離センサ対応。`pulseIn` で ECHO パルス幅を計測し距離（cm）を算出。計測範囲 約 2〜400 cm

### v1.2.3

- **SDmin: `sm_open_a()` 追加** — 既存ファイルへの追記オープン（ファイルが存在しない場合は新規作成）
- **SDmin: `sm_sync_w()` 追加** — ファイルを開いたまま現在のセクタをフラッシュしディレクトリのファイルサイズを更新（電源断対策）
- **SDmin: SDLog サンプルスケッチ追加** — UART 受信データを microSD に記録する OpenLog 互換ロガー
- **EEPROM: サンプルスケッチ削除** — `Serial.print` を使用しており UIAPduino HID 設定でビルドエラーになるため削除

### v1.2.2

- **SDmin: `sm_rmdir(path)` 追加** — 空ディレクトリを削除する関数を追加  
  `sm_del()` と共通実装 `_sm_del_entry()` に統合し、Flash 増加を約 20 バイトに最小化

### v1.2.1

- **修正**: variant ディレクトリ名を `CH32V003F4_SD` → `CH32V003F4` にリネーム（他の型番の命名規則に統一）
- **修正**: `variant_CH32V003F4.h` で `I2C_MODULE_ENABLED` を有効化  
  コメントアウトされていたため `PinMap_I2C_SDA` / `PinMap_I2C_SCL` が未定義となり、Wire を使うスケッチすべてでリンクエラーが発生していた（v1.2.0 の不具合）
- **修正**: `variant_CH32V003F4.h` に `PIN_WIRE_SDA=PC1`・`PIN_WIRE_SCL=PC2` を追加  
  未定義のままでは `Wire.begin()` が誤ったピン（USB D-/UART TX）を使いハングしていた
- **修正**: `boards.txt` に `-DCPLUSPLUS` フラグを追加  
  `__libc_init_array()` を実行させることで `TwoWire` のグローバルコンストラクタが動作し、`Wire.begin()` が確実に初期化される
- **修正**: `cores/arduino/abi.cpp` の `__cxa_pure_virtual` / `__cxa_deleted_virtual` に `__weak__` 属性を追加  
  `-DCPLUSPLUS` 追加時に `ch32fun.c` との重複シンボルリンクエラーを回避
- **修正**: `cores/arduino/uiapusb.c` に `GetTick()` オーバーライドを追加  
  `SysTick->CNT ÷ 48000` でミリ秒を計算（割り込みなし）。SysTick 割り込みを有効化すると rv003usb のビットバンギングが破壊されるため、割り込みを使わない方式を採用。`millis()` および I2C タイムアウトが正常動作する
- **修正**: `Wire.h` / `Wire.cpp` に `begin(int, int)` オーバーロードを追加  
  `Wire.begin(PC1, PC2)` が C++ オーバーロード解決で `begin(int addr, bool generalCall)` に誤解決されてスレーブモードになる問題を修正
- **Wire examples 追加**: `i2c_scanner`, `i2c_slave_test`, `i2c_master_test`, `i2c_BMP280_test`  
  BMP280（温度・気圧センサ）の読み取りサンプルを新規追加。`<math.h>` の `powf()` は 16KB Flash を超過するため高度計算はコメントのみ収録

### v1.2.0
- **Wire (I2C) ライブラリを初めて動作させた**（マスター・スレーブともに v1.1.5 以前は完全に動作不可）
- **根本原因修正**: I2C ISR の `WCH-Interrupt-fast` 属性を標準割り込み属性に変更  
  `WCH-Interrupt-fast` は MIE=0 でも割り込みを発火させる WCH PFIC HPE 機構を使用しており、  
  rv003usb（ソフトウェア USB）のビットサンプリング処理を横取りして USB HID を切断していた
- **追加対策**: ITBUFEN=0（割り込みストーム防止）、NVIC_EnableIRQ を I2C_Init() 後に移動（スプリアス ISR 防止）
- **Wire/README.md 追加**: 配線・API・バイトオーダー・ISR コールバック注意事項・rv003usb 互換性の解説

### v1.1.5
- **Feature Report サイズを 16 → 32 バイトに拡張**（`usb_config.h` / `uiapusb.c`）  
  長いファイル名（LFN）転送に対応
- **`uiapwebhid_send()` にタイムアウト追加**（`uiapusb.c`）  
  USB ホスト未接続（モバイルバッテリー等）時の無限ループを防止し、スタンドアロン動作を実現
- **SDmin: SD 書き込みタイムアウトを延長**（`SDmin.h`）  
  6MHz SPI クロック時の書き込みタイムアウトを ~43ms → ~266ms に修正（SD スペック 250ms 準拠）

### v1.1.4
- WebHID Only モードの USB product name を `"UIAPduino WebHID"` に修正

### v1.1.2
- WebHID Only モードのビルドエラーを修正

### v1.1.0
- ボード定義を 1 エントリに統合（USB モード / UART / 最適化を Tools メニューで選択）
- No USB（SD ログ・UART 専用）モードを追加
- `None (use UIAPSerial)` をデフォルト UART 設定として追加（HardwareSerial 約 4,748B 節約）

---

## USB VID/PID について

本 core に含まれる VID/PID は**開発・評価用**です。

```
VID: 0x1209  (pid.codes — オープンソース向け共有 VID)
PID: 0xD004  (UIAPduino 向けサンプル値)
```

この core を使用して USB デバイスを**製造・配布・販売**する場合は、
利用者または製造者の責任で正規に利用可能な VID/PID を設定してください。
`cores/arduino/usb_config.h` の該当行を変更してください。

---

## ライセンス

fork 元: [openwch/arduino_core_ch32](https://github.com/openwch/arduino_core_ch32)  
本リポジトリの改変部分は同ライセンスに従います。

## 連絡先・バグ報告

[GitHub Issues](https://github.com/tarosay/arduino_core_ch32/issues/new) にてお知らせください。
