# MIDI Synth Controller Core 機能仕様書

## 1. 目的とスコープ
- 本書は、Core配下ソースコードの現状実装を根拠に、機能仕様を整理したもの。
- 対象は主に以下ファイル。
	- Core/Src/main.c
	- Core/Src/mode.c
	- Core/Src/stm32_synth_midi.c
	- Core/Src/neopixel.c
	- Core/Src/stm32_synth_dfu.c
	- Core/Src/stm32f4xx_hal_msp.c
	- Core/Src/stm32f4xx_it.c
	- Core/Inc/*.h
- USBホストのMIDIクラス処理本体はUSB_HOST配下にあり、本書ではCoreから見える連携点のみ記載する。

## 2. システム概要
- 本ファームは、MIDI入力を受けてSPI経由でシンセへ送信し、同時に8x32 RGB LEDマトリクスへ状態を表示する。
- ユーザー操作はロータリーエンコーダ2個(各ボタン付き)で行う。
- 内部にメトロノームとステップシーケンサ機能を持つ。
- 設定モードからDFUブートローダへ遷移可能。

## 3. ハードウェアI/O仕様

### 3.1 外部インタフェース
- USB
	- USB Hostスタックを起動し、メインループで `MIDI_Application()` と `MX_USB_HOST_Process()` を呼ぶ。
	- USB MIDI受信データは最終的にCore側MIDIバッファへ投入される想定(実処理はUSB_HOST配下)。
- SPI
	- SPI5: MIDI入力受信(スレーブ、RX専用、DMA受信)
		- 4バイトUSB-MIDI風パケットをDMAで受信し、完了コールバックでリングバッファへ格納。
	- SPI3: シンセサイザ向けMIDI送信(マスタ、DMA送信)
	- SPI2: LEDマトリクス(Neopixel系)向け送信(マスタ、DMA送信)

### 3.2 GPIO入力
- RE1/RE2ボタン: 押下/離上をEXTIで検出、タイマハンドラで状態遷移管理。
- RE1/RE2回転相(A/B): EXTIで変化順を記録し、タイマハンドラで回転方向とカウントに変換。
- bleConnect: BLE接続状態入力。表示のインジケータに使用。
- LED_TYPE: LED配線タイプ選択入力。表示座標変換(蛇行配線反転)に使用。

### 3.3 GPIO出力
- bleInitMode: 起動時条件で1秒パルス出力しBLE初期モード切替を要求。
- BOARD_LED: 初期化のみ(現状、常時点灯/点滅制御は未実装)。

## 4. クロック・周期仕様
- SYSCLK: HSE+PLL設定により96MHz。
- TIM11割り込み周期(概算):
	- Prescaler=20000, Period=100 より約47.5Hz(約21ms周期)。
- TIM11割り込み用途:
	- ボタン状態の `PUSHED -> PUSHING` 遷移処理
	- 回転エンコーダ位相の確定とインクリメント/デクリメント処理

## 5. ソフトウェア構成

### 5.1 main.c
- 初期化:
	- HAL/Clock/GPIO/DMA/SPI2/USB/TIM11/SPI5/SPI3
	- `re_init()` でエンコーダ管理初期化
	- TIM11割り込み開始
	- 起動時にRE1ボタン押下ならBLE初期モード切替パルスを出力
	- MIDI mode controlでCable Number filter ONを送信
	- SPI5 DMA受信開始(4バイト)
- メインループ:
	- `MIDI_Application()`
	- `MX_USB_HOST_Process()`
	- `stm32synth_hundle(&hspi2, &hspi3, tick)`
	- `re_hundle(tick)`

### 5.2 mode.c
- UI入力(ボタン・回転)をモード/パラメータ変更へ変換。
- エンコーダ割当:
	- RE1: MODE / VOLUME / PROGRAM を巡回
	- RE2: CHANNEL / REVERB (通常時)
	- RE1がMODE選択中はRE2をモード専用パラメータ操作に切替
- パラメータ範囲制約(主なもの):
	- BPM: 30-360, ステップ5
	- シーケンサ小節: 1-16
	- チャンネル: 0-15, 16はALL扱い
- モード固有操作:
	- METRONOME: RE2ボタンでタップテンポ
	- SEQUENCER_PARA: RE2ボタンで選択項目(BPM/BEAT/BAR/CH/METRO)巡回
	- SEQUENCER_INPUT: RE2ボタンでUNDO/CH CLEAR実行
	- SEQUENCER_PLAY: RE2ボタンで再生トグルまたはミュート切替
	- SETTING: RE2ボタンで設定実行(現状はDFU遷移)

### 5.3 stm32_synth_midi.c
- 中核機能:
	- MIDIリングバッファ管理(4byteパケット)
	- MIDIイベント解析(Note/CC/Program等)
	- 表示状態マシン制御
	- メトロノーム生成
	- シーケンサ記録/再生/ミュート
	- 各種MIDI送信API(Volume/Program/Reverb/Mode Control等)

### 5.4 neopixel.c
- 8x32フレームバッファ管理。
- RGB24をNeopixelシリアルビット列へ変換しSPI DMA送信。
- 文字描画、バー描画、ロゴ、アイコン描画を提供。
- `LED_TYPE` に応じて奇数列の走査方向を反転。

### 5.5 stm32_synth_dfu.c
- 割り込み停止、SysTick停止、RCC初期化、NVICクリア後にSystem Memoryへジャンプ。

## 6. データフロー仕様

### 6.1 MIDI入力からシンセ出力まで
1. SPI5 DMA受信完了割り込みで4バイト受信。
2. 受信データを `stm32synth_midi_writebuff()` でリングバッファへ書込。
3. DMA受信を再アーム。
4. メインループの `stm32synth_hundle()` が1件読出。
5. イベント解析・内部処理(表示更新、シーケンサ記録等)を実施。
6. 最終パケットをSPI3 DMAでシンセへ送信。

### 6.2 表示更新
- `stm32synth_hundle()` 内で現在表示状態に応じてフレームバッファを再描画。
- 最後に `neopixel_flush()` でSPI2 DMA送信。

## 7. 表示状態仕様
- 主状態:
	- HELLO
	- MODESEL
	- NOTE(フリープレイ)
	- METRONOME
	- SEQUENCER_PARA
	- SEQUENCER_INPUT
	- SEQUENCER_PLAY
	- CHANNEL
	- MIDIIN
	- SETTING
- 一時表示:
	- MODESEL/CHANNEL/MIDIIN/SETTINGなどは一定時間経過で `base_code` に復帰。
- BLE接続表示:
	- 状態に応じて隅ピクセルを点灯。

## 8. モード別機能仕様

| モード | 表示状態(base_code) | 主な機能 | 主な操作(ロータリーエンコーダ) |
|---|---|---|---|
| FREEPLAY | NOTE | Note On/Off可視化(ノート位置+残光)。freeplay mode=1でチャンネルアクティビティ表示。 | RE2 MODEPARAで freeplay mode(0/1) 切替。 |
| METRONOME | METRONOME | BPM周期でドラムノート(37)発音、針アニメーションとBPM表示。 | RE2回転でBPM変更(30-360, step=5)。RE2ボタンでタップテンポ。 |
| SEQUENCER_PARA | SEQUENCER_PARA | シーケンサ基本パラメータ編集。 | RE2ボタンで項目巡回(BPM/BEAT/BAR/CHANNEL/METRONOME)。RE2回転で選択項目値を変更。 |
| SEQUENCER_INPUT | SEQUENCER_INPUT | ノート録音(4095 step/bar)、レイヤ管理、編集。 | RE2回転で対象バー/機能選択。RE2ボタンでUNDO(最終レイヤ削除)またはCH CLEAR(選択チャンネル削除)。 |
| SEQUENCER_PLAY | SEQUENCER_PLAY | 再生カーソル表示、再生/停止、チャンネルミュート。 | RE2回転で再生制御選択(0:PLAY/STOP, 1-16:MUTE CH)。RE2ボタンで選択機能を実行。 |
| SETTING | SETTING | 設定実行。現状はDFU遷移のみ。 | RE2ボタンで実行。現在の項目は GOTODFU。 |

補足:
- RE1は全モード共通で MODE / VOLUME / PROGRAM の制御選択に使用。
- RE1がMODE選択中は、RE2がモード専用パラメータ操作(MODEPARA)として動作。

## 9. MIDI仕様(実装準拠)
- パケット長: 常に4バイト。
- バッファサイズ: 256エントリ。
- 送信系関数:
	- `midi_spi_*`: cIndex上位ニブル0系で送信(外部入力相当)
	- `midi_play_*`: cIndex上位ニブル1系で送信(内部再生相当)
- シーケンサ入力時のチャンネル置換:
	- 外部入力(内部再生でないメッセージ)は選択チャンネルへ置換可能。
- モード制御CC:
	- 起動時にCable Number filter ONを送信。

## 10. 割り込み仕様
- EXTI:
	- RE1/RE2ボタン・回転相入力を捕捉し `re_exti_callback()` に集約。
- TIM11:
	- `re_tim_callback()` を呼び、エンコーダ確定処理。
- DMA2 Stream3(SPI5 RX):
	- 受信完了時にMIDIバッファ投入。

## 11. 制約・既知挙動
- MIDIリングバッファ書込時のオーバーフロー検出は未実装。
- `state.code == HELLO` 中はMIDI読出/転送を実行しない。
- シーケンサ録音容量は `SEQUENCER_MAXBAR=16`, `SEQUENCER_MAXCHORDPARBAR=80` に依存。
- USB MIDI具体処理はCore外(USB_HOST配下)のため、本書は連携点まで。

## 12. 将来の構成書き換えに向けた分割候補
- 入力層:
	- rotary/button入力(state machine)
	- MIDI入力統合(SPI/USB)
- アプリ層:
	- mode管理
	- sequencer engine
	- metronome engine
- 出力層:
	- MIDI出力トランスポート
	- display renderer
- 共通層:
	- event queue/ring buffer
	- time utility

