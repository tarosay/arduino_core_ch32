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
    uint8_t buf[16];
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
| `WebHID.recv(buf, maxlen)` | Feature Report を受信（最大 16 バイト） |

### USB エンドポイント構成（WebHID Only）

| EP | 方向 | 用途 |
|----|------|------|
| EP1 IN | UIAPduino → ブラウザ | Input Report（8 バイト） |
| EP0 Feature | ブラウザ → UIAPduino | Feature Report（最大 16 バイト） |

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
    uint8_t buf[16];
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
| EP0 Feature | ブラウザ → UIAPduino | WebHID Feature Report（最大 16 バイト） |

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

> **KeyboardPractice / KeyboardSwitch** は [UIAPduino WebHID Lab](https://tarosay.github.io/uiap-hid-web/) の練習ページと連携して使います。

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

---

## 対応 OS

| OS | 状況 |
|----|------|
| Windows | 動作確認済み（Arduino IDE 2.0 以上） |
| Linux | 動作確認中 |
| macOS | 動作確認中 |

---

## 更新履歴

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
