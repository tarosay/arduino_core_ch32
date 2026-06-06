# Wiremin

Minimal I2C driver for CH32V003 (UIAPduino).  
Wire.h の代替。Flash 使用量を大幅に削減します。

## Flash サイズ比較

同等スケッチでのビルド比較（CH32V003、`usb=nousb`）:

| ライブラリ | Flash | RAM |
|---|---|---|
| Wire.h | 10,620 バイト（64%） | 592 バイト |
| Wiremin.h | 4,444 バイト（27%） | 428 バイト |
| **削減量** | **▲ 6,176 バイト** | **▲ 164 バイト** |

## 動作確認済みデバイス

| デバイス | 種別 | Flash（参考） |
|---|---|---|
| BMP280 | 温度・気圧センサ | — |
| BMI270 | 6軸 IMU（コンフィグ込み） | 15,728 バイト（95%） |

## API

### マスター

```cpp
#include <Wiremin.h>

Wiremin_begin();                              // 100kHz で初期化
Wiremin_begin_fast();                         // 400kHz で初期化
Wiremin_end();                                // 解放

Wiremin_write(addr7, data, len);              // バイト列送信
Wiremin_read(addr7, buf, len);               // バイト列受信
Wiremin_write_reg(addr7, reg, data, len);    // レジスタ書き込み
Wiremin_read_reg(addr7, reg, buf, len);      // レジスタ読み出し（repeated START）
Wiremin_probe(addr7);                        // ACK 確認（スキャン用）
```

### スレーブ

```cpp
Wiremin_slave_begin(addr7);   // スレーブとして初期化
Wiremin_slave_set(reg, val);  // 共有レジスタに書き込む
Wiremin_slave_get(reg);       // 共有レジスタを読み出す
```

マスターからの読み書きは ISR が自動処理します。コールバック不要。

## Wire.h からの移植

### 初期化

```cpp
// Wire.h
Wire.begin();

// Wiremin.h
Wiremin_begin();
```

### レジスタ書き込み

```cpp
// Wire.h
Wire.beginTransmission(addr);
Wire.write(reg);
Wire.write(data, len);
Wire.endTransmission();

// Wiremin.h
Wiremin_write_reg(addr, reg, data, len);
```

### レジスタ読み出し（repeated START）

```cpp
// Wire.h
Wire.beginTransmission(addr);
Wire.write(reg);
Wire.endTransmission(false);
Wire.requestFrom(addr, len);
for (int i = 0; i < len; i++) buf[i] = Wire.read();

// Wiremin.h
Wiremin_read_reg(addr, reg, buf, len);
```

### アドレス確認（I2C スキャン）

```cpp
// Wire.h
Wire.beginTransmission(addr);
bool found = (Wire.endTransmission() == 0);

// Wiremin.h
bool found = Wiremin_probe(addr);
```

### OLED（SSD1306）例

```cpp
// コマンド送信
uint8_t cmd = 0xAE;  // display off
Wiremin_write_reg(0x3C, 0x00, &cmd, 1);

// データ送信（128バイト/ページ）
Wiremin_write_reg(0x3C, 0x40, buf, 128);
```

## 配線

```
UIAPduino CH32V003F4
  PC1 (D3) = SDA
  PC2 (D4) = SCL
  ※ SDA/SCL に 4.7kΩ プルアップ抵抗（to 3.3V）が必要
```

## 注意事項

- Wire.h と同時に使用不可（`I2C1_EV_IRQHandler` が競合）
- 1つの `.ino` ファイルにのみ `#include <Wiremin.h>` すること
- スレーブモードでは I2C センサを同時接続不可（I2C1 が1つのみ）
