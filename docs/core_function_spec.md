# MIDI Synth Controller Core 機能仕様書

## 1. 目的とスコープ
- 本書は、Core配下ソースコードの現状実装を根拠に、機能仕様を整理したもの。
- 対象は主に以下ファイル。
	- Core/Src/main.c
	- Core/Src/app_*.c
	- Core/Src/domain_*.c
	- Core/Src/io_*.c
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
	- USB MIDI受信データはUSB_HOST配下で解析し、`io_midi_transport_push_rx_packet()` を介してRXキューへ投入する。
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
- BOARD_LED: USB Hostの接続・MIDIクラス使用可能状態を表示する。

## 4. クロック・周期仕様
- SYSCLK: HSE+PLL設定により96MHz。
- TIM11割り込み周期(概算):
	- Prescaler=20000, Period=100 より約47.5Hz(約21ms周期)。
- TIM11割り込み用途:
	- `io_input_driver_timer_callback()` によるボタン状態の更新
	- `io_input_driver_handle()` による回転・ボタンイベント生成

## 5. ソフトウェア構成

### 5.1 main.c
- 初期化:
	- HAL/Clock/GPIO/DMA/SPI2/USB/TIM11/SPI5/SPI3
	- Display/Input/MIDI Transport各I/Oモジュール初期化
	- `app_runtime_init()` / `app_runtime_input_init()` によるアプリケーション状態初期化
	- TIM11割り込み開始
	- 起動時にRE1ボタン押下ならBLE初期モード切替パルスを出力
	- SPI5 DMA受信開始(4バイト)
	- `app_runtime_start()` で初期表示とCable Number filter ONを送信
- メインループ:
	- `MIDI_Application()`
	- `MX_USB_HOST_Process()`
	- Input Adapterのイベントを `app_runtime_post_event()` へ投入
	- `app_runtime_step(tick)` で状態遷移、MIDI処理、シーケンサ処理を実行
	- `io_midi_transport_service()` でSPI3 DMA送信を実行
	- Display ModelをRendererへ渡してLEDフレームを描画・送信

### 5.2 Input Driver / Adapter / Router
- `io_input_driver` がGPIOとタイマからボタン・回転状態を確定する。
- `io_input_adapter` が入力状態を `app_event_t` に変換してキューへ投入する。
- `app_input_router` が入力元とイベント種別に応じてRuntimeのハンドラへ配送する。
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
	- SEQUENCER_PROGRAM: RE2回転で入力NoteNum変更(変更時にNoteOnし100ms後にNoteOff)、RE2押下でNoteOn、RE2離上でNoteOff(同時に録音)
	- SEQUENCER_REC: RE2ボタンでUNDO/CH CLEAR実行
	- SEQUENCER_PLAY: RE2ボタンで再生トグルまたはミュート切替
	- SETTING: RE2ボタンで設定実行(現状はDFU遷移)。GOTODFU実行時はシンセサイザへMIDIモード制御CC119、value=100 (`IO_MIDI_MODECONTROL_GOTODFU`) をSPI3で送信してからローカルDFUへ遷移する。

### 5.3 Application層
- `app_runtime` がイベント消費、モード状態、オーバーレイ期限、シーケンサ操作を統括する。
- `app_dispatcher` がイベントとtickをCommandへ変換する。
- `app_midi_utils` が4バイトMIDIパケット生成とチャンネル展開ポリシーを担当する。

### 5.4 Domain層
- `domain_mode_def` がモード定義と基本表示を管理する。
- `domain_sequencer_engine` がテンポ、進行、録音、再生、ミュートの正本を保持する。
- `domain_display_model` がRendererへ渡す表示状態とMIDI表示コンテキストを保持する。Note On/Offは単一コンテキストとは別のイベントFIFOで順序を保持する。

### 5.5 I/O層
- `io_midi_transport`: SPI5割り込み受信キューとSPI3 DMA送信キューを管理する。
- `io_display_renderer`: Display Modelを8x32描画命令へ変換する。
- `io_display_driver`: フレームバッファをLEDシリアルデータへ変換しSPI2 DMA送信する。
	- `io_system_control`: BLE初期モード切替パルスを出力し、割り込みとクロックを停止してSystem MemoryのDFUへ遷移する。遷移前のシンセサイザ向けGOTODFU MIDI送信は `app_runtime` が実行する。

## 6. データフロー仕様

### 6.1 MIDI入力からシンセ出力まで
1. SPI5 DMA受信完了割り込みで4バイト受信。
2. 受信データを `io_midi_transport_push_rx_packet()` でRXキューへ書込。SPI5割り込みとUSB Host処理が共有するため、書込み区間を排他する。
3. DMA受信を再アーム。
4. メインループの `app_runtime_step()` が受信キューを読出す。
5. MIDIイベント解析、表示更新、シーケンサ記録、必要なチャンネル置換を実施する。
6. `io_midi_transport_send_packet()` で送信キューへ投入する。
7. `io_midi_transport_service()` がSPI3 DMAでシンセへ送信する。

USB MIDI入力は `USBH_MIDI_ReceiveCallback()` で実受信長を基準に4バイト単位で解析し、対応するChannel Voice MessageをRXキューへ投入する。メインループの `app_runtime_process_rx_midi()` が表示更新とSPI3送信を行う。Note On/Off表示イベントは表示モデル内のFIFOで保持し、複数イベントの上書きを防ぐ。

### 6.2 表示更新
- `domain_display_model` が現在の基本表示、オーバーレイ、MIDI表示コンテキストを保持する。
- メインループで `io_display_renderer_render()` がフレームバッファを再描画する。
- 最後に `io_display_renderer_flush()` からSPI2 DMA送信する。

## 7. 表示状態仕様
- 主状態:
	- HELLO
	- MODESEL
	- NOTE(フリープレイ)
	- METRONOME
	- SEQUENCER_PARA
	- SEQUENCER_PROGRAM
	- SEQUENCER_REC
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
| FREEPLAY | NOTE | SETTINGでMODE1/MODE2を選択可能。MODE1は縦リング、MODE2は多色波形を表示し、同一channel/noteの重複NoteOnをカウント集約する。 | RE2回転でCHANNEL/REVERBを操作。RE2ボタンでCHANNEL/REVERB切替。 |
| METRONOME | METRONOME | BPM周期でドラムノート(37)発音、針アニメーションとBPM表示。 | RE2回転でBPM変更(30-360, step=5)。RE2ボタンでタップテンポ。 |
| SEQUENCER_PARA | SEQUENCER_PARA | シーケンサ基本パラメータ編集。 | RE2ボタンで項目巡回(BPM/BEAT/BAR/CHANNEL/METRONOME)。RE2回転で選択項目値を変更。 |
| SEQUENCER_PROGRAM | SEQUENCER_PROGRAM | RE2でノートを手入力して録音。 | RE2回転で入力NoteNumを変更(変更時にNoteOnし100ms後にNoteOff)。RE2押下でNoteOn、RE2離上でNoteOff(押下/離上タイミングで録音)。 |
| SEQUENCER_REC | SEQUENCER_REC | 録音内容の編集。 | RE2回転で対象バー/機能選択。RE2ボタンでUNDO(最終レイヤ削除)またはCH CLEAR(選択チャンネル削除)。 |
| SEQUENCER_PLAY | SEQUENCER_PLAY | 再生カーソル表示、再生/停止、チャンネルミュート。 | RE2回転で再生制御選択(0:PLAY/STOP, 1-16:MUTE CH)。RE2ボタンで選択機能を実行。 |


ANIMATION選択の動作 (実装済み):
- 入場時は `ANIMATION` を選択し、RE2回転で `ANIMATION` / `GOTODFU` を巡回する。
- `ANIMATION` 選択中のRE2短押しで `MODE1` / `MODE2` を切り替える。選択値はRAM内に保持し、起動時は `MODE1`。
- `GOTODFU` 選択中のRE2短押しは既存のDFU遷移を実行する。

MODE2共振波形:
- アクティブな(channel,note)ごとに波形レイヤーを持ち、同じ組み合わせのNoteOnはheld countに集約する。NoteOffでcountが0になると180msで減衰する。
- voice開始順に0.75/1.0/1.25/1.5/2.0周期の視覚presetを割り当て、1 voiceを周期の異なう2レイヤーで同時に描く。周期presetはMIDI pitchに対応させず、`noteNum`は位相と包絡中心の偏りに使う。
- 各レイヤーは中心が強いGaussian包絡付きの定在波 `sin(kx)*cos(ωt)` とし、振幅が0を跨いで上下に振動する(基本周期約0.6秒)。線はsmoothstepで画素に吸着させて細く描く。Apple系8色RGBグラデーションは時間変化し、重なりは波形値ではなくpixel RGBを加算してsoft clipで圧縮する。縦方向にのみ極めて弱いglowを加え、白との混合はしない。
- MODE2はRGB各成分を1/6に減光する。NoteOnは60msで立ち上がる。Note中はidle線を表示せず、全voiceが180msのreleaseで減衰すると輝度約11の白い水平線へ戻る。
- 対象はFREEPLAYのNOTE表示に限り、CC/Program ChangeのMIDIIN表示とMIDI転送は変更しない。

補足(SEQUENCER_PLAY overlay):
- `seqpara_playmode=0` では `PLAY` 文字は表示しない。
- `seqpara_playmode>0` のときは、`MUTE%u` 文字を表示せず `xbar(x=0, y=((playmode-1)%8), len=32)` を表示する。
- `playmode` が `1..8` か `9..16` かは、区切り線左(`x=7`)の識別ドット(`y=0` / `y=1`)で示す。
| SETTING | SETTING | ANIMATION選択とGOTODFU実行。 | 入場時はANIMATIONを選択。RE2回転でANIMATION/GOTODFUを巡回し、ANIMATION短押しでMODE1/MODE2を切り替える。GOTODFU短押しで `CC119, value=100` をSPI3送信後、ローカルDFUへ遷移する。 |

補足(シーケンサのメトロノーム):
- シーケンサの初期状態は `play=1` とし、`SEQUENCER_PARA` 表示の初期アイコンとメトロノーム動作を一致させる。
- シーケンサの再生状態の正本は `domain_sequencer_engine.playing` とし、legacy 表示状態での二重管理は行わない。
- シーケンサの進行状態 (`beat_count/bar_count/beat_elapsed/beat_period`) も `domain_sequencer_engine` を正本とし、legacy MIDI state には複製しない。
- PLAY モードの選択チャンネル表示も legacy state に重複保持せず、現在の表示メッセージ/アプリ状態から決定する。
- `app_midi_middleware` / `domain_midi_state` / `app_midi_display` の旧状態保持は廃止し、これらは未使用互換スタブとしてのみ残す。
- `SEQUENCER_PARA/CLEAR/PLAY` のパラメータ変更オーバーレイ中は、区切り線(`x=8`)を表示しない。
- ただし `SEQUENCER_PLAY` の `seqpara_playmode=0` (PLAY操作) オーバーレイ時は、区切り線(`x=8`)と右側表示を維持する。
- `SEQUENCER_PARA/PROGRAM/CLEAR/PLAY` では、`MTRON` が ON のときメトロノームクリック音を出す。
- `SEQUENCER_PLAY` では再生中(`play=1`)のときのみクリック音を出し、STOP中(`play=0`)は鳴らさない。
- シーケンサ針は1小節の経過で左端から右端まで1回進む。
- シーケンサのクリック音は、小節頭を強く、それ以外の拍頭を弱く鳴らす。
- 小節インジケータの現在位置は `bar_count` を基準に更新する。
- 小節インジケータの現在位置色は、1..8 小節では緑系、9..16 小節では黄色系とする。
- 小節インジケータの更新タイミングは、クリック音大(小節頭)と同じタイミングに合わせる。

補足:
- RE1は全モード共通で MODE / VOLUME / PROGRAM の制御選択に使用。
- RE1がMODE選択中は、RE2がモード専用パラメータ操作(MODEPARA)として動作する（FREEPLAYを除く）。
- RE1制御選択時の `MODE`/`VOL`/`PRG` オーバーレイ表示は行わない。
- FREEPLAY時はRE1がMODE選択中でも、RE2は `CHANNEL/REVERB` 操作として動作する。
- SEQUENCER系表示ではタイミング針を `y=0..7` の全高で表示する。
- SEQUENCER系オーバーレイ中は小節表示(下部バー帯)を常時描画する。
- 小節数が 9..16 の場合は、9 小節目以降を左端側へ折り返して黄色系で上書きする legacy 表示を用いる。
- SEQUENCER系のRE2パラメータ調整オーバーレイ中は左ペイン(8x8アイコン)を保持し、`x=8` フレームは非表示にする。
- SEQUENCER_REC/PLAY のオーバーレイ中は `x=8` フレームを保持し、右側楽譜エリアはオーバーレイ表示で覆う。
- SEQUENCER_PROGRAM の RE2回転/押下/離上操作では、区切り線(`x=8`)と右側楽譜エリアを通常表示のまま維持し、オーバーレイを出さない。
- SEQUENCERのチャンネル選択は `xbar(x=0, y=(ch%8), len=32)` を使用する。
- `1..8ch` と `9..16ch` の切替は、区切り線左(`x=7`)の識別ドット(`y=0` / `y=1`)で示す。

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
	- GOTODFU実行時に `CC119 (MODECONTROL), value=100` (`IO_MIDI_MODECONTROL_GOTODFU`) をSPI3へ送信してからローカルDFUへ遷移する。

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

