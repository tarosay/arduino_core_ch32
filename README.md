# Arduino core for UIAPduino (HID ProMicro CH32V003)

Arduino IDE で UIAPduino を使うための Arduino コアです。  
[openwch/arduino_core_ch32](https://github.com/openwch/arduino_core_ch32) を fork し、UIAPduino 向けに独自に開発・改良しています。

---

## ボードについて

**UIAPduino (HID ProMicro CH32V003)**

- MCU: WCH CH32V003F4 (RISC-V, 48MHz)
- フラッシュ: 16KB / RAM: 2KB
- USB: Type-C（HID デバイスとして動作）
- ピン: 15本の GPIO をヘッダーに引き出し

### ピン配置

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

## HID API

USB 経由でホスト PC とデータを送受信します。  
`Serial.print()` の代わりに `HIDuiap` オブジェクトを使います。

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
