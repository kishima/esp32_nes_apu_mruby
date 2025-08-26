# esp32_nes_apu_mruby

## 機能

PicoRubyで、ファミコンエミュレータのAPU音源を鳴らすためのプロジェクトです

## ターゲットデバイス

M5StickC plus2 向けに設定しています。

PSRAMは必須です。

PSRAM搭載のESP32シリーズであれば、FlashのサイズやPSRAMのサイズを調整して動作できるかと思いますが、確認してません。

### 必要な外部デバイス

I2Sのスレーブデバイスが必要です。
PCM5102で動作確認してます。
apu_emuの`#define USE_I2S`をコメントアウトすると、PWMでも動作するはずです。

### 配線

以下の通り。`apu_if.h`を編集して変更可能です。

```
#define PIN_BCK   GPIO_NUM_26
#define PIN_WS    GPIO_NUM_25
#define PIN_DOUT  GPIO_NUM_33
```

## ESP32向けのビルド

.devcontainerフォルダをおいてますが、devcontainerとしては動作確認しておらず、私は以下のようなコマンドで動作確認してます。

### 初回だけ

```bash
cd .devcotainer
./build.sh
./.devcontainer/run_devcontainer.sh idf.py set-target esp32
```

### 通常のビルド

```bash
./.devcontainer/run_devcontainer.sh idf.py build
./.devcontainer/run_devcontainer.sh idf.py flash
```

## 使い方

`fatfs/home/` 以下に `logformat.txt` に記載した形式のバイナリファイルを配置してR2P2のシェルから、以下のコマンドで再生できます。

バイナリファイルの拡張子は、reglogです。

```
$> play sample
```

## クレジット

sample.reglogファイルは以下のNSFファイルから生成したものです。

元の作品: "crimmy buzz.nsf" by big lumby, CC BY-NC-SA 3.0

https://battleofthebits.com/arena/Entry/crimmy+buzz.nsf/50518/
