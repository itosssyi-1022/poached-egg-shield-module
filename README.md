# Poached Eggs ZMK Config

このリポジトリには、Poached Eggs 用の ZMK 設定と、BLE 接続先に応じて Windows / Mac の挙動を切り替える追加モジュールが含まれています。

## 現在の切り替え仕様

- BLE Profile 0: Windows
- BLE Profile 1: Mac
- BLE Profile 2 以降: 現状は Windows と同じ扱い

切り替え判定は [app/src/behavior_os_layer.c](app/src/behavior_os_layer.c) で行っています。左手側が split central なので、OS 切り替え機能は左手側ビルドで有効化しています。

## レイヤー構成

- Layer 0: ベース入力レイヤー。Windows の既定挙動。
- Layer 1: 記号 / ナビゲーションレイヤー。
- Layer 2: ショートカット / ファンクションレイヤー。
- Layer 3: 記号 / 言語レイヤー。
- Layer 4: Bluetooth 制御レイヤー。
- Layer 5: Mac 用ベース差分レイヤー。Profile 1 のときだけ自動で重なる。
- Layer 6: Mac 用ショートカット差分レイヤー。Mac の Layer 5 から遷移する。
- Layer 7: Mac 用記号 / 言語差分レイヤー。Mac の Layer 5 から遷移する。

## Windows 時の挙動

- 既定では Layer 0-4 のみを使います。
- ベース層の修飾キーは、左下寄りが Ctrl、親指側が GUI の割り当てです。
- ショートカット層ではコピーや貼り付けなどが Ctrl 系ショートカットです。
- 記号 / 言語層では Windows 向けに Lock、Explorer、Run、貼り付けが GUI 系ショートカットで割り当てられています。

## Mac 時の挙動

Profile 1 に切り替わると Layer 5 が有効になり、Layer 0 の一部だけを上書きします。

- 左手の Ctrl ポジションは Command に切り替わります。
- 左親指寄りの GUI ポジションは Control に切り替わります。
- 右手ホームポジションの hold-tap GUI は Control に切り替わります。
- ショートカットレイヤーへの遷移先が Layer 2 から Layer 6 に切り替わります。
- 記号 / 言語レイヤーへの遷移先が Layer 3 から Layer 7 に切り替わります。

Layer 6 と Layer 7 の差分は次のとおりです。

- コピー、貼り付け、切り取り、全選択、保存、Undo が Ctrl 系から Command 系に切り替わります。
- 記号 / 言語レイヤーの GUI 系ショートカットは、Mac 向けにロック、Spotlight、アプリ切り替え、絵文字ビューアへ置き換わります。

## 実装ファイル

- [app/src/behavior_os_layer.c](app/src/behavior_os_layer.c): BLE プロファイルの変更イベントを監視し、Mac 用 Layer 5 を ON/OFF する処理。
- [app/CMakeLists.txt](app/CMakeLists.txt): OS 切り替え機能が有効なときだけ追加 C ソースをビルド対象に含める設定。
- [app/Kconfig](app/Kconfig): `CONFIG_ZMK_BEHAVIOR_OS_LAYER` の定義。
- [config/boards/shields/poached_eggs/poached_eggs_left.conf](config/boards/shields/poached_eggs/poached_eggs_left.conf): 左手側ビルドで OS 切り替え機能を有効化する設定。
- [config/poached_eggs.keymap](config/poached_eggs.keymap): Windows 既定レイヤーと Mac 差分レイヤーの定義。

## ビルド時の前提

- Zephyr module としてこのリポジトリ自身を読み込むため、[build.yaml](build.yaml) で `ZMK_EXTRA_MODULES` を設定しています。
- `zephyr/module.yml` から `app/CMakeLists.txt` と `app/Kconfig` を読み込む構成です。

## 補足

- 現状、iOS 専用レイヤーは未実装です。
- Mac の切り替え先は BLE Profile 1 に固定しています。別の番号にしたい場合は [app/src/behavior_os_layer.c](app/src/behavior_os_layer.c) を変更してください。