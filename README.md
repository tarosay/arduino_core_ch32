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
https://github.com/tarosay/board_manager_files/raw/main/package_ch32v_index.json
```

「ボードマネージャ」で `UIAPduino` を検索してインストールしてください。  
`Tools > Board > UIAP_HID > HID ProMicro CH32V003` を選択します。

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
Wiremin_write(addr7, data, len);             // バイト列送信
Wiremin_read(addr7, buf, len);              // バイト列受信
Wiremin_write_reg(addr7, reg, data, len);   // レジスタ書き込み
Wiremin_read_reg(addr7, reg, buf, len);     // レジスタ読み出し（repeated START）
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

> **注意:** Wire.h と同時に使用不可。スレーブモードでは I2C センサを同時接続不可（I2C1 が1つのみ）。

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
| Wiremin_size_test | Flash サイズ計測用（Wire.h との比較に使用） |

### SDmin

`ファイル` → `スケッチ例` → `SDmin` から開けます。  
`Tools > USB > No USB (SD log / UART only)` を選択してビルドしてください。

| スケッチ | 内容 |
|---------|------|
| SDLog | UART RX で受信したデータをマイクロSD カードに記録する OpenLog 互換ロガー |

---

## 対応 OS

| OS | 状況 |
|----|------|
| Windows | 動作確認済み（Arduino IDE 2.0 以上） |
| Linux | 動作確認中 |
| macOS | 動作確認中 |

---

## 更新履歴

### v1.2.4（最新）

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

## ライセンス

fork 元: [openwch/arduino_core_ch32](https://github.com/openwch/arduino_core_ch32)  
本リポジトリの改変部分は同ライセンスに従います。

## 連絡先・バグ報告

[GitHub Issues](https://github.com/tarosay/arduino_core_ch32/issues/new) にてお知らせください。
