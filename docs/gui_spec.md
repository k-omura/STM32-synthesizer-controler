# 8x32 LED Display UI仕様書

## 1. 目的
本仕様書は、コミット `fed630a` の実コードから、8x32 LED表示仕様を再構成したものです。
解釈差分を防ぐため、文字表示の内容・フォント・開始座標、グラフィックのピクセルマップを明示します。

## 3. ディスプレイ座標系と描画プリミティブ
- 論理解像度: 32x8 (`x=0..31`, `y=0..7`)
- 原点: 左上 (`x` 右方向, `y` 下方向)
- 1px描画: `neopixel_fb_pixel(x, y, color)`
- 横バー: `neopixel_fb_xbar(x, y, length, color)`
- 縦バー: `neopixel_fb_ybar(x, y, length, color)`
- 8x8アイコン: `neopixel_fb_icon(icon, color)` は左上 `(0,0)` 固定
- 文字列: `neopixel_fb_stringBitmap(x, y, text, size, color, font)`

備考:
- 配線タイプ `neopixel_type==1` の場合、奇数列は物理マッピングが反転するが、論理座標指定は不変。

## 4. フォント仕様
### 4.1 通常テキスト (実運用の主流)
- API呼び出し: `size=0`
- 実体: `FONT5x3` (3x5ドット)
- 1文字あたりの進み: `+4px` (3px glyph + 1px spacing)
- 文字の開始位置: `neopixel_fb_stringBitmap` に渡した `(x,y)`

### 4.2 拡張フォント
- API呼び出し: `size!=0`
- 実体: `FONT8x8[font]` の可変幅描画
- 本UI経路では基本的に未使用

## 5. 表示ステートとタイムアウト
- `state.code`: 現在表示ステート
- `state.base_code`: 基本表示ステート
- 一時表示の標準タイムアウト:
- `MODESEL`: 1000ms後に `base_code` 復帰
- `CHANNEL`: 1000ms後に `base_code` 復帰
- `MIDIIN`: 1000ms後に `base_code` 復帰
- `SETTING`: 1000ms後に `base_code` 復帰
- `HELLO`: 2500ms周期で表示遷移

## 5.1 色仕様

色は `fb_color24_t` を使用し、チャネル順は `{green, red, blue}`。

基本色定義 (実装値):
- `col_reset = {0, 0, 0}`
- `col_string = {8, 8, 8}`
- `col_needle = {10, 0, 2}`
- `col_ble = {0, 0, 10}`
- `col_bar_all = {6, 3, 3}`
- `col_cursorx = {1, 1, 1}`
- `col_cursory = {1, 2, 1}`
- `col_sel_bar = {1, 1, 10}`
- `col_sel_channel = {1, 10, 1}`
- `col_sel_play = {10, 1, 1}`
- `col_sel_stop = {8, 8, 8}`
- `col_sel_rec = {0, 10, 0}`

シーケンサ録音ノート色 (`col_sel_recnote[8]`):
- `{20, 4, 4}`
- `{4, 20, 4}`
- `{4, 4, 20}`
- `{20, 20, 4}`
- `{4, 20, 20}`
- `{20, 4, 20}`
- `{20, 12, 0}`
- `{0, 12, 20}`

動的色:
- NOTE の発音色: velocity から算出
- `green = vel & 0x03`, `red = (vel >> 2) & 0x03`, `blue = (vel >> 4) & 0x03`
- CC 値バー色: `green = 10 * val / 127`, `blue = 10 - green`, `red = 0`

表示要素ごとの色割り当て:
- 通常文字列: `col_string`
- 背景クリア: `col_reset`
- METRONOME 針: `col_needle`
- BLEインジケータ: `col_ble`
- シーケンサ縦フレーム: `col_cursorx`
- シーケンサ横カーソル/チャンネル選択帯: `col_cursory`
- シーケンサ全体バー: `col_bar_all`
- シーケンサ選択バー: `col_sel_bar`
- シーケンサ選択チャンネル点: `col_sel_channel`
- PLAYアイコン: `col_sel_play`
- STOPアイコン: `col_sel_stop`
- RECORDアイコン: `col_sel_rec`

## 6. モード名文字列
`mode_text[][7]`
- `MODE_STATE_FREEPLAY`: `FREEPLY`
- `MODE_STATE_METRONOME`: `METRNOM`
- `MODE_STATE_SEQUENCER_PARA`: `SQ-PARA`
- `MODE_STATE_SEQUENCER_PROGRAM`: `SQ-PROG`
- `MODE_STATE_SEQUENCER_REC`: `SQ-REC `
- `MODE_STATE_SEQUENCER_PLAY`: `SQ-PLAY`
- `MODE_STATE_SETTING`: `SETTING`

## 7. モード別 基本表示仕様

### 7.1 HELLO (`STM32SYNTH_DISPSTATE_HELLO`)
- 描画: `neopixel_fb_logo(state.cnt, col_string)`
- 画面: 32x8ロゴ全画面
- グラフィック: ロゴビットマップ (定数 `OTAKURO_LOGO`)

### 7.2 MODESEL (`STM32SYNTH_DISPSTATE_MODESEL`)
- 文字: `mode_text[state.message]`
- フォント: 3x5 (`size=0`)
- 開始座標: `(2,1)`

### 7.3 NOTE (`STM32SYNTH_DISPSTATE_NOTE`)
- ノート中心点:
- 座標: `(nn % 32, 2 + nn / 32)`
- 残光エフェクト:
- 座標: 中心から縦方向に `length=1+2*cnt` の `ybar`
- BLE表示: `(31,7)`

NOTE画面レイアウト (固定領域)

```
y=0  ................................
y=1  ................................
y=2  NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
y=3  NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
y=4  NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
y=5  NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
y=6  ................................
y=7  ...............................B
```

凡例:
- `N`: ノート描画領域 (動的)
- `B`: BLEピクセル

### 7.4 METRONOME (`STM32SYNTH_DISPSTATE_METRONOME`)
- 針:
- `needlePos = (31 * bpmDiff / bpmtick)` (往復)
- 描画: `ybar(needlePos, 0, 2)`
- 文字: `BPM%3d`
- フォント: 3x5
- 開始座標: `(4,3)`
- BLE表示: `(31,7)`

METRONOME画面レイアウト (固定領域)

```
y=0  .........|......................
y=1  .........|......................
y=2  ................................
y=3  ....TTTTTTTTTTTTTT..............
y=4  ....TTTTTTTTTTTTTT..............
y=5  ....TTTTTTTTTTTTTT..............
y=6  ................................
y=7  ...............................B
```

凡例:
- `|`: 針の縦バー位置 (動的)
- `T`: BPM文字描画領域
- `B`: BLEピクセル

### 7.5 SEQUENCER_PARA / SEQUENCER_PROGRAM / SEQUENCER_REC / SEQUENCER_PLAY
共通のフレームを使う。

固定描画:
- 縦フレーム: `x=8`, `y=0..7`
- 下部バー帯: `y=7`, `x=0..bar-1`
- アイコン: 左上8x8 (`x=0..7,y=0..7`)
- タイミング針: `x=9..31` を移動し、`y=0..7` の全高 `ybar`
- シーケンサ系の針は1小節の経過時間で、開始位置から終了位置まで1回進む
- `bar > 8` の場合、1..8 小節は通常色で表示し、9..16 小節は左端側へ折り返して黄色系で上書きする
- 現在小節位置は `bar_count` に基づいて表示する
- 現在小節位置の色は、1..8 小節では緑系、9..16 小節では黄色系とする
- 現在小節位置の更新タイミングは、クリック音大(小節頭)と同じ境界に合わせる

文字オーバーレイ (1秒):
- 開始座標: `(8,1)`
- 文字内容:
- `CNTSEQ_BPM`: `BPM%3d`
- `CNTSEQ_BEAT`: `BEAT%2d`
- `CNTSEQ_BAR`: `BAR %2d`
- `CNTSEQ_SELBAR`:
- rec_mode=0 -> `UNDO`
- rec_mode=1 -> `CH CLR`
- `CNTSEQ_CHANNEL`: 文字表示なし
- `CNTSEQ_CHANNEL` の選択位置は `xbar(x=0, y=(ch%8), len=32)` で表示する
- `CNTSEQ_CHANNEL` のバンク識別は区切り線左(`x=7`)にドット表示する (`1-8ch: y=0`, `9-16ch: y=1`)
- `CNTSEQ_METRONOME`:
- 0 -> `MTROFF`
- 1 -> `MTRON`

シーケンサメトロノーム:
- 小節頭で強いクリック音を出す
- その間に `beat` 分割された拍ごとに弱いクリック音を出す

オーバーレイ時のベース維持:
- `SEQUENCER_PARA/CLEAR/PLAY` 由来のオーバーレイ表示中は、左ペイン(8x8アイコン)を保持する
- 小節表示(下部バー帯 `y=7`)はオーバーレイ中も常時描画する
- 右側の楽譜エリア(`x=9..31`)はベース描画を行わず、オーバーレイ表示で覆う
- `SEQUENCER_PARA/CLEAR/PLAY` のパラメータ調整オーバーレイ中は、区切り線(`x=8`)を描画しない
- `SEQUENCER_PLAY` の `value=0` (PLAY操作) オーバーレイ時は、区切り線(`x=8`)と右側グラフィックを維持する
- `SEQUENCER_PROGRAM` の RE2回転/押下/離上操作ではオーバーレイを出さず、区切り線(`x=8`)と右側グラフィックを通常表示のまま維持する

SEQUENCER_PLAY 追加表示:
- 開始座標: `(8,1)`
- 文字内容:
- `value=0` -> 表示なし
- `value>0` -> 文字表示なし
- `value>0` のとき、選択中ミュートchを `xbar(x=0, y=((value-1)%8), len=32)` で表示する
- バンク識別は区切り線左(`x=7`)にドット表示する (`1-8ch: y=0`, `9-16ch: y=1`)

シーケンサ画面レイアウト (固定領域)

```
y=0  IIIIIIII|GGGGGGGGGGGGGGGGGGGGGGG
y=1  IIIIIIII|TTTTTTTTTTTTTTTTTTTTTTT
y=2  IIIIIIII|GGGGGGGGGGGGGGGGGGGGGGG
y=3  IIIIIIII|GGGGGGGGGGGGGGGGGGGGGGG
y=4  IIIIIIII|GGGGGGGGGGGGGGGGGGGGGGG
y=5  IIIIIIII|GGGGGGGGGGGGGGGGGGGGGGG
y=6  IIIIIIII|GGGGGGGGGGGGGGGGGGGGGGG
y=7  BBBBBBBB|GGGGGGGGGGGGGGGGGGGGGGG
```

凡例:
- `I`: 8x8アイコン領域
- `|`: 縦フレーム
- `T`: 一時テキスト領域 (1秒)
- `G`: ノート・カーソル・チャンネル選択の描画領域
- `B`: バー本数/位置インジケータ領域

### 7.6 CHANNEL (`STM32SYNTH_DISPSTATE_CHANNEL`)
- 文字開始座標: `(2,1)`
- 表示内容:
- `message>15`: `CHNLALL`
- `message==9`: `CH DRUM`
- それ以外: `CHNL.%2d` (`message+1`)
- グラフィック:
- 選択ch位置を `y=7` に2pxバー (`x=2*ch, len=2`)

### 7.7 MIDIIN (`STM32SYNTH_DISPSTATE_MIDIIN`)
- ステート本体では描画しない
- 直前の `inputmidi` 内描画結果を 1000ms保持して表示

MIDIIN 代表表示:
- Program Change:
- テキスト: `stm32synth_prog_text[program]` (7文字)
- 開始座標: `(2,1)`
- 色: `col_string`
- 単一ch対象時: `y=7` にchバー (`x=2*ch, len=2`, 色 `col_string`)
- `ALL` ミラー時: chバーは描画しない
- Control Change:
- テキスト: `CC3文字 + .%3d` (例: `VOL.100`, `RVB.064`)
- 開始座標: `(2,1)`
- 色: 文字 `col_string`
- 値バー: `y=7`, `len=32*val/127`, 色 `green=10*val/127`, `blue=10-green`, `red=0`
- chマーカ: 単一ch対象時は `y=6`, `x=2*ch`, `len=2`, 色 `col_string`。`ALL` ミラー時は描画しない
- 除外CC: `DMP` (`CC64`), `MODECONTROL` (`CC119`) はMIDIIN表示しない

主要パラメータ変更時の表示:
- VOLUME変更 (`CC7`): `VOL.%3d` を `(2,1)` に表示、値バーを `y=7` に表示
- REVERB変更 (`CC91`): `RVB.%3d` を `(2,1)` に表示、値バーを `y=7` に表示
- 楽器変更 (`Program Change`): `stm32synth_prog_text[program]` の7文字を `(2,1)` に表示

### 7.7.1 CC番号と表示ラベル対応 (主要項目)
`stm32synth_cc_text[cc]` により、MIDIINで3文字ラベルを表示する。

| 項目 | CC番号 | 表示ラベル | 表示形式 |
|---|---:|---|---|
| Volume | 7 | `VOL` | `VOL.%3d` |
| Reverb Level | 91 | `RVB` | `RVB.%3d` |
| Bank Select | 0 | `BNK` | `BNK.%3d` |
| Pan | 10 | `PAN` | `PAN.%3d` |
| Expression | 11 | `EXP` | `EXP.%3d` |

備考:
- 上記以外のCCも同一形式 `XXX.%3d` で表示する。
- CCラベルは `stm32synth_cc_text` の3文字コードをそのまま使用する。
- `CC64(DMP)` と `CC119(MODECONTROL)` は表示対象外。

### 7.7.2 パラメータ変更時の表示テンプレート
- CC系パラメータ変更:
- 文字: `(2,1)` に `XXX.%3d`
- 値バー: `y=7`, 長さ `32*val/127`
- chマーカ: 単一ch対象時は `y=6`, `x=2*ch`, `len=2`。`ALL` ミラー時は表示しない
- Program Change:
- 文字: `(2,1)` に7文字楽器名 (`stm32synth_prog_text[program]`)
- chバー: 単一ch対象時は `y=7`, `x=2*ch`, `len=2`。`ALL` ミラー時は表示しない

### 7.8 SETTING (`STM32SYNTH_DISPSTATE_SETTING`)
- target=0 (現実装): `GOTODFU`
- フォント: 3x5
- 開始座標: `(2,1)`

## 8. ロータリーエンコーダ操作 -> 表示仕様

## 8.1 RE1 ボタン
- 役割選択を巡回: `MODE -> VOLUME -> PROGRAM -> MODE`
- 実装: `cnt_midi_cont_change(&re_rot[0].control, 0)`
- 表示: 役割選択オーバーレイ(`MODE`/`VOL`/`PRG`)は表示しない

## 8.2 RE1 回転
- `control=MODE`: `mode.mode` を更新し `MODESEL` 表示
- `control=VOLUME`: `0..127`, step=4, ch=ALL時は全chミラー
- `control=PROGRAM`: `0..127`, step=1, ch=ALL時は全chミラー
- VOLUMEは単一ch対象なら `MIDIIN` 相当の `VOL.%3d` を表示し、`y=6` にchマーカを出す
- PROGRAMは単一ch対象なら `MIDIIN` 相当の楽器名7文字を表示し、`y=7` にchマーカを出す
- ch=ALL時は内部送信扱いで全chへミラー送信し、chバーは表示しない

## 8.3 RE2 ボタン (RE1がMODE以外)
- `RE2 control` 巡回: `CHANNEL <-> REVERB`
- 実装: `cnt_midi_cont_change(&re_rot[1].control, 1)`
- 表示:
- CHANNEL: `stm32synth_disp_channel(cnt_channel)`
- REVERB: 直接専用画面はなし (CC表示はMIDIIN経由)

## 8.4 RE2 回転 (RE1がMODE以外)
- CHANNEL: `0..16` (`16=ALL`)
- REVERB: `0..127`, step=4
- 表示:
- CHANNELは専用 `CHANNEL` 画面
- REVERBはMIDI経路経由で `MIDIIN` に `RVB.%3d` を表示

## 8.5 RE2 操作 (RE1がMODE)
RE2は mode parameter に切替。

FREEPLAY:
- 回転: `RE2 control` に応じて `CHANNEL` または `REVERB` を操作
- ボタン: `RE2 control` を `CHANNEL <-> REVERB` で切替
- 表示: CHANNEL は `CHNL.%2d / CH DRUM / CHNLALL`、REVERB は `RVB.%3d` を表示

METRONOME:
- 回転: BPM (`30..360`, step=5)
- ボタン: タップテンポ (`cnt_metronme_tap2bpm`) -> BPM更新
- 表示: METRONOME画面 (`BPM%3d` at `(4,3)`)

SEQUENCER_PARA:
- ボタン: 選択項目巡回 (`CNTSEQ_BPM/BEAT/BAR/CHANNEL/METRONOME`)
- 回転: 選択項目の値を変更
- 表示: `SEQUENCER_PARA` の1秒テキスト (開始 `(8,1)`)

SEQUENCER_PROGRAM:
- 回転: 入力NoteNumを `0..127` で変更
- 回転時動作: NoteNum変更ごとに `NoteOn` し、100ms後に `NoteOff`
- ボタン押下: 現在のNoteNumを `NoteOn` し、同時にシーケンサへ録音開始
- ボタン離上: 同じNoteNumを `NoteOff` し、録音終了
- 表示: 区切り線および右側エリアは通常表示のまま、オーバーレイなし

SEQUENCER_REC:
- 回転: `cnt_seqpara_sel_bar 0..(bar*2-1)`
- rec_mode判定: `<bar` ならUNDO, `>=bar` ならCH CLEAR
- ボタン: `stm32synth_seq_progpush(0|1)` 相当の編集操作を実行
- 表示: 1秒テキスト `UNDO` or `CH CLR`

SEQUENCER_PLAY:
- 回転: `cnt_seqpara_playmode 0..16`
- ボタン:
- `0`: play toggle
- `1..16`: mute ch toggle (`ch = playmode-1`)
- 表示: `playmode>0` のとき全幅 xbar + バンク識別ドット + 下部バー帯 + `SEQ_PLAY` フレーム

SETTING:
- ボタン: `cnt_setting_execute()` -> `GOTODFU` 実行
- 表示: `GOTODFU` at `(2,1)`

## 8.6 RE操作時系列 (入力 -> 送信 -> 表示)

### 8.6.1 RE1 回転: VOLUME
1. RE1回転で `cnt_midi_val(RE1_CNT_VOLUME)` 更新 (step=4)
2. `cnt_midi_vol()` で `stm32synth_midi_volume()` または `stm32synth_midi_volumeAll()` を呼ぶ
3. CC7 (`VOL`) をMIDIバッファへ投入
4. `inputmidi` 側でCCイベントを描画し `state.code=MIDIIN` を1秒保持
5. 表示は `VOL.%3d` + 値バー (`y=7`)。外部入力時のみchマーカ表示

### 8.6.2 RE1 回転: PROGRAM (楽器変更)
1. RE1回転で `cnt_midi_val(RE1_CNT_PROGRAM)` 更新 (step=1)
2. `cnt_midi_program()` で `stm32synth_midi_program()` または `stm32synth_midi_programAll()` を呼ぶ
3. Program Change をMIDIバッファへ投入
4. `inputmidi` 側でProgram Changeを描画し `state.code=MIDIIN` を1秒保持
5. 表示は楽器名7文字 (`stm32synth_prog_text[program]`) を `(2,1)`
6. 外部入力時のみ `y=7` に2pxのchバーを表示

### 8.6.3 RE2 回転: REVERB
1. RE2回転で `cnt_midi_val(RE2_CNT_REVERB)` 更新 (step=4)
2. `cnt_midi_reverb()` で `stm32synth_midi_reverb()` を呼ぶ
3. CC91 (`RVB`) をMIDIバッファへ投入
4. `inputmidi` 側でCCイベントを描画し `state.code=MIDIIN` を1秒保持
5. 表示は `RVB.%3d` + 値バー (`y=7`)。外部入力時のみchマーカ表示

### 8.6.4 RE2 回転: CHANNEL
1. RE2回転で `cnt_channel` 更新 (`0..16`, `16=ALL`)
2. `cnt_midi_channel()` が `stm32synth_disp_channel(cnt_channel)` を呼ぶ
3. `state.code=CHANNEL` で1秒表示
4. 表示は `CHNL.%2d` / `CH DRUM` / `CHNLALL` + `y=7` の2px chバー
5. sequencer 系のチャンネル選択は、`xbar(x=0, y=(ch%8), len=32)` と区切り線左のバンク識別ドットで表す

### 8.6.5 RE1 回転: MODE
1. RE1回転で `mode.mode` 更新
2. `cnt_midi_mode()` が `stm32synth_disp_mode(mode.mode)` を呼ぶ
3. `state.code=MODESEL` で1秒表示
4. 表示は `mode_text[]` を `(2,1)`
5. タイムアウト後は `state.base_code` へ復帰

## 8.7 操作別 送信MIDIコマンド仕様

表記:
- `Bn`: Control Change (`0xB0 | ch`)
- `Cn`: Program Change (`0xC0 | ch`)
- `ch`: MIDIチャンネル (`0..15`)
- `ALL`: 全ch送信 (`0..15` へ反復送信)

### 8.7.1 RE1 回転

| 条件 | 送信コマンド | データ内容 | 備考 |
|---|---|---|---|
| `control=MODE` | `B0` (CC) | `CC=MODECONTROL`, `value=mode_id` | モード変更通知 |
| `control=VOLUME`, `ch=0..15` | `Bn` (CC) | `CC7(VOL)`, `value=0..127` (step=4) | 単一ch更新 |
| `control=VOLUME`, `ch=ALL` | `B0..BF` (CC) | 各chへ `CC7(VOL)` 同値送信 | 全chミラー |
| `control=PROGRAM`, `ch=0..15` | `Cn` (PC) | `program=0..127` (step=1) | 単一ch更新 |
| `control=PROGRAM`, `ch=ALL` | `C0..CF` (PC) | 各chへ同一program送信 | 全chミラー |

### 8.7.2 RE2 回転 (RE1がMODE以外)

| 条件 | 送信コマンド | データ内容 | 備考 |
|---|---|---|---|
| `control=CHANNEL` | 送信なし | なし | 選択chのみ更新 |
| `control=REVERB` | `B0` (CC) | `CC91(RVB)`, `value=0..127` (step=4) | グローバル送信 |

### 8.7.3 RE2 操作 (RE1がMODE)

| モード | 操作 | 送信コマンド | データ内容 |
|---|---|---|---|
| FREEPLAY | 回転/押下 | 送信なし | ローカル状態更新のみ |
| METRONOME | 回転/押下 | 送信なし | BPM内部値更新 |
| SEQUENCER_PARA | 回転/押下 | 送信なし | シーケンサ内部パラメータ更新 |
| SEQUENCER_PROGRAM | 回転/押下/離上 | NoteOn/NoteOff + 内部録音 | 回転でNoteNum試聴(100ms自動NoteOff)、押下/離上で録音ノート長を入力 |
| SEQUENCER_REC | 回転/押下 | 送信なし | 内部シーケンサ編集操作 |
| SEQUENCER_PLAY | 回転/押下 | 送信なし | 再生/ミュート内部状態更新 |
| SETTING | 押下 | 送信なし | `GOTODFU` 実行 |

### 8.7.4 外部MIDI入力の透過送信

受信MIDIは基本的にそのまま転送する。

| 受信イベント | 再送内容 |
|---|---|
| Control Change | 同一パケットを再送 |
| Program Change | 同一パケットを再送 |
| Note On/Off 等 | 同一パケットを再送 |

備考:
- 表示仕様 (MIDIIN) と送信仕様は独立であり、表示除外CC (`DMP`, `MODECONTROL`) でも再送自体は行う。

## 9. シーケンサアイコン 8x8 ピクセルマップ
描画領域: `(0,0)` 起点 8x8
記号: `#=点灯`, `.=消灯`

### 9.1 SEQ_ICON_PLAY
```
y0 ........
y1 .#......
y2 .##.....
y3 .###....
y4 .##.....
y5 .#......
y6 ........
y7 ........
```

### 9.2 SEQ_ICON_STOP
```
y0 ........
y1 ........
y2 .###....
y3 .###....
y4 .###....
y5 ........
y6 ........
y7 ........
```

### 9.3 SEQ_ICON_RECORD
```
y0 ........
y1 ........
y2 ..##....
y3 .####...
y4 .####...
y5 ..##....
y6 ........
y7 ........
```

## 10. 文字描画開始位置一覧 (実装値)
- MODESEL mode text: `(2,1)`
- METRONOME BPM: `(4,3)`
- SEQUENCER一時文字: `(8,1)`
- SEQUENCER_PLAY overlay text: `(8,1)`
- CHANNEL文字列: `(2,1)`
- SETTING `GOTODFU`: `(2,1)`
- MIDIIN Program名: `(4,1)`
- MIDIIN CC/イベント文字列: `(2,1)`

## 11. 実装上の注意
- 一部表示は「直接画面遷移」ではなく、MIDI入力処理経由で `MIDIIN` に反映される。
- よって、VOLUME/PROGRAM/REVERB などは「操作直後に専用UIを直接描く」設計ではない。
- `SEQ_*` 系は 1秒テキスト表示とグラフ表示の多層合成であり、`state.message` と `state.cnt` の扱いが表示決定に重要。

## 12. 実装適用ルール
- 文字列内容・座標は本書を正とする。
- 3x5フォント運用をデフォルトとし、幅詰め特性 (1文字4px進み) を前提にレイアウトする。
- グラフィックは本書の固定領域とピクセルマップを維持する。
- タイムアウト値 (1000ms/2500ms) を保持する。

## 13. 網羅チェック結果
- 表示ステート (`HELLO/MODESEL/NOTE/METRONOME/SEQUENCER_PARA/SEQUENCER_PROGRAM/SEQUENCER_REC/SEQUENCER_PLAY/CHANNEL/MIDIIN/SETTING`) の描画仕様を記載済み。
- RE1/RE2 のボタン・回転について、MODE/CHANNEL/SEQUENCER系に加えて VOLUME/PROGRAM/REVERB の表示仕様を記載済み。
- 楽器変更 (Program Change) は、文字列ソース・開始座標・内部/外部入力でのchバー差分まで記載済み。
- 主要CC (VOL/RVB/BNK/PAN/EXP) の番号・表示ラベル・表示形式を記載済み。
- RE操作時系列 (入力 -> MIDI送信 -> 画面更新 -> タイムアウト復帰) を記載済み。
