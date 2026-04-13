# Arduino core for UIAPduino (HID ProMicro CH32V003)

Arduino IDE で UIAPduino を使うための Arduino コアです。  
[openwch/arduino_core_ch32](https://github.com/openwch/arduino_core_ch32) を fork し、UIAPduino 向けに独自に開発・改良しています。

---

## ボードについて

### HID ProMicro CH32V003（ターミナル HID）

PC と USB HID 経由でデータを送受信する汎用ボードです。

- MCU: WCH CH32V003F4 (RISC-V, 48MHz)
- フラッシュ: 16KB / RAM: 2KB
- USB: Type-C（HID デバイスとして動作）
- ピン: 15本の GPIO をヘッダーに引き出し

### HID ProMicro CH32V003 KBD+Mouse（キーボード＋マウス HID）

USB キーボード・マウスとして PC を操作するボードです。  
**Board Version Select** メニューで機能を選択できます。

| Board Version | 機能 |
|--------------|------|
| V1.4 | キーボード (EP2) ＋ マウス (EP1) |
| V1.4 + WebHID (EP3) | キーボード ＋ マウス ＋ WebHID 双方向通信 (EP3) |

---

## ピン配置

| Arduino番号 | ポート | 備考 |
|:-----------:|:------:|------|
| GPIO_PIN_0  | PA1    | A1 |
| GPIO_PIN_1  | PA2    | A0 |
| GPIO_PIN_2  | PC0    | LED |
| GPIO_PIN_3  | PC1    | SDA |
| GPIO_PIN_4  | PC2    | SCL |
| GPIO_PIN_5  | PC3    | PWM |
| GPIO_PIN_6  | PC4    | A2 / SS |
| GPIO_PIN_7  | PC5    | SCK |
| GPIO_PIN_8  | PC6    | MOSI |
| GPIO_PIN_9  | PC7    | MISO |
| GPIO_PIN_10 | PD0    | |
| GPIO_PIN_11 | PD1    | SWIO デバッグピン（要 `pinDisconnectDebug()`） |
| GPIO_PIN_12 | PD2    | A3 |
| GPIO_PIN_15 | PD5    | TX / A5 |
| GPIO_PIN_16 | PD6    | RX / A6 |

> **注意:** GPIO_PIN_11 (PD1) は SWIO デバッグピンと共用です。  
> 通常の GPIO として使う場合は `pinDisconnectDebug(GPIO_PIN_11)` を最初に1回呼んでください。

---

## インストール方法

Arduino IDE の「環境設定」→「追加のボードマネージャのURL」に以下を追加します:

```
https://github.com/tarosay/board_manager_files/raw/main/package_ch32v_index.json
```

「ボードマネージャ」で `UIAPduino` を検索してインストールしてください。

---

## HID API（ターミナルモード）

`HID ProMicro CH32V003` で使用します。  
USB 経由でホスト PC とデータを送受信します。

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

## Keyboard / Mouse ライブラリ（KBD+Mouse モード）

`HID ProMicro CH32V003 KBD+Mouse` で使用します。  
ボード選択: `Tools > Board > HID ProMicro CH32V003 KBD+Mouse`  
バージョン選択: `Tools > Board Version Select > V1.4`

```cpp
#include <Keyboard.h>
#include <Mouse.h>

void setup() {
  Keyboard.begin();
  Mouse.begin();
  delay(2000);
}

void loop() {
  Keyboard.print("Hello");   // キー入力
  Mouse.move(10, 0);         // マウスを右に10移動
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

USB Low Speed のポーリング間隔（EP2: 10ms）の非同期性により、
矢印・BackSpace・Enter・Home などの特殊キーは **`Keyboard.write()`** を使い、
さらに呼び出しの後に **`delay(50)`** を入れると取りこぼしなく動作します。

```cpp
// ✅ 推奨: write() + delay(50)
for (int i = 0; i < 6; i++) {
  Keyboard.write(KEY_LEFT_ARROW);
  delay(50);
}

// ❌ 非推奨: press() + releaseAll() はポーリングを逃して無効になることがある
Keyboard.press(KEY_LEFT_ARROW);
Keyboard.releaseAll();
```

#### `Keyboard.write()` のタイミング

```
press → delay(20ms) → release → delay(20ms) → 次のキーへ
```

- 20ms の press 待機で EP2 ポーリングを 1〜2 回確実に通過させる
- 20ms の release 待機でホストが key-up を認識する
- 1文字あたり約 40ms（"Hello UIAPduino World." 22文字 ≈ 0.9 秒）
- 特殊キー後のスケッチ側 `delay(50)` と合わせると合計約 90ms の余裕

### Mouse API

| メソッド | 説明 |
|---------|------|
| `Mouse.begin()` | マウスを開始する |
| `Mouse.move(x, y, wheel)` | 相対移動（各 -127〜127） |
| `Mouse.press(btn)` | ボタンを押す |
| `Mouse.release(btn)` | ボタンを離す |
| `Mouse.click(btn)` | クリック |

---

## WebHID ライブラリ（KBD+Mouse + WebHID モード）

`HID ProMicro CH32V003 KBD+Mouse` + `V1.4 + WebHID (EP3)` で使用します。  
Chrome / Edge の **WebHID API** を使って、Web ブラウザと UIAPduino が双方向通信できます。  
キーボード・マウス機能と同時に使用可能です。

> **注意:** WebHID は Chrome / Edge のみ対応です。キーボード・マウス HID インターフェースは  
> ブラウザのセキュリティ制限で直接アクセスできないため、ベンダー定義 HID (EP3) を使用します。

```cpp
#include <WebHID.h>

void setup() {
  WebHID.begin();
  delay(2000);
}

void loop() {
  // Web からデータを受信してエコーバック
  if (WebHID.available()) {
    uint8_t buf[16];
    uint8_t len = WebHID.recv(buf, sizeof(buf));
    WebHID.send(buf, len);
  }

  // 1秒ごとにカウンタを送信
  static uint32_t last = 0;
  static uint8_t counter = 0;
  if (millis() - last >= 1000) {
    last = millis();
    WebHID.send(counter++, 0, 0, 0, 0, 0, 0, 0);
  }
}
```

### WebHID API

| メソッド | 説明 |
|---------|------|
| `WebHID.begin()` | USB を開始する |
| `WebHID.send(buf, len)` | EP3 Input Report で Web へ送信（最大 8 バイト） |
| `WebHID.send(b0,b1,...,b7)` | 個別バイト指定で送信（最大 8 バイト） |
| `WebHID.available()` | Web からのデータが届いているか |
| `WebHID.recv(buf, maxlen)` | Feature Report を受信（最大 16 バイト） |

### USB エンドポイント構成（V1.4 + WebHID）

| EP | 方向 | 用途 |
|----|------|------|
| EP1 IN | UIAPduino → PC | マウスレポート |
| EP2 IN | UIAPduino → PC | キーボードレポート |
| EP3 IN | UIAPduino → Web | WebHID Input Report (8 bytes) |
| EP0 Feature | Web → UIAPduino | WebHID Feature Report (最大 16 bytes) |

---

## コアの独自改良点

### `pinDisconnectDebug(uint32_t pin)`

Arduino ピン番号を渡してデバッグ機能を切り離す関数です。  
`PinName` 型への変換は内部で自動的に行われます。

```cpp
pinDisconnectDebug(GPIO_PIN_11);  // PD1 を通常 GPIO として使えるようにする
pinMode(GPIO_PIN_11, OUTPUT);
```

> **フラッシュ節約のため `pinMode()` 内では自動実行されません。**  
> 必要なピンに対してスケッチの `setup()` で明示的に呼んでください。

### `GPIO_PIN_N` マクロ

ピン番号を分かりやすく記述するためのエイリアスです。  
`GPIO_Pin_N`（小文字 `p`）は CH32 ペリフェラルライブラリのビットマスクとして既に使われているため、大文字 `GPIO_PIN_N` を採用しています。

```cpp
pinMode(GPIO_PIN_6, OUTPUT);   // PC4 を出力に設定
digitalWrite(GPIO_PIN_6, HIGH);
```

---

## サンプルスケッチ

### HID ProMicro CH32V003（ターミナル HID）

`ファイル` → `スケッチ例` → `HID` から開けます。

| スケッチ | 内容 |
|---------|------|
| Blink | LED (GPIO_PIN_2 / PC0) を点滅 |
| GPIO_test | ボード上の全 GPIO を順番にテスト |
| HidDigitalWriteRead | GPIO の書き込み・読み返しを HID で確認 |
| HidLoopbackBlink | HID 受信データに応じて LED を制御 |
| HidMillisTicker | `millis()` の値を定期送信 |
| HidMicrosTicker | `micros()` の値を定期送信 |
| HidAdcMonitor | ADC 値を定期送信 |
| PwmAndToneTest | PWM / Tone の動作確認 |

### HID ProMicro CH32V003 KBD+Mouse

`ファイル` → `スケッチ例` → `Keyboard` / `Mouse` / `WebHID` から開けます。

| ライブラリ | スケッチ | 内容 |
|-----------|---------|------|
| Keyboard / Mouse | KbdMouseTest | キー入力とマウス移動のサンプル |
| Keyboard | KeyboardPractice | キーボード HID 練習（Step ごとにコメントを外して書き込む） |
| Keyboard | KeyboardSwitch | キーボード HID 練習（switch 文で全 Step を 1 回の書き込みで切り替え） |
| WebHID | WebHIDTest | EP3 エコーバック ＋ 1秒ごとカウンタ送信 |

> **KeyboardPractice / KeyboardSwitch** は [UIAPduino WebHID Lab](https://tarosay.github.io/uiap-hid-web/) の練習ページと連携して使います。

---

## 対応 OS

| OS | 状況 |
|----|------|
| Windows | 動作確認済み（Arduino IDE 2.0 以上） |
| Linux | 動作確認中 |
| macOS | 動作確認中 |

---

## ライセンス

fork 元: [openwch/arduino_core_ch32](https://github.com/openwch/arduino_core_ch32)  
本リポジトリの改変部分は同ライセンスに従います。

## 連絡先・バグ報告

[GitHub Issues](https://github.com/tarosay/arduino_core_ch32/issues/new) にてお知らせください。
