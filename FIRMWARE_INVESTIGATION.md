# Poached Eggs firmware 調査レポート

## 要約

- このリポジトリは、**Poached Eggs** という分割キーボード向けの **ZMK** firmware をビルドする構成です。
- 左右の両方のデバイスは、**Seeed Studio XIAO BLE**（`seeeduino_xiao_ble`）を対象にしています。これは **nRF52840** ベースのマイコンです。
- 左半分は **split central**、右半分は **split peripheral** として構成されています。
- `.keymap` を変更して Actions で生成した firmware を書き込んでも**まったく反映されない**主因として最も可能性が高いのは、左半分で **ZMK Studio** が有効になっており、過去に保存された永続設定が新しい compiled keymap より優先されていることです。
- このリポジトリでは、もともと **`settings_reset`** 用の GitHub Actions artifact を生成していなかったため、Actions から取得した firmware だけでは永続設定を消去できませんでした。

## リポジトリ調査結果

### ハードウェア / MCU

- ビルド対象ボード: `seeeduino_xiao_ble`
  - 出典: `build.yaml`
- shield metadata 上では `seeed_xiao` を要求
  - 出典: `config/boards/shields/poached_eggs/poached_eggs.zmk.yml`

### split の役割

- `CONFIG_ZMK_SPLIT_ROLE_CENTRAL` は左側 shield でのみ有効
- `CONFIG_ZMK_SPLIT` は左右両方で有効
  - 出典: `config/boards/shields/poached_eggs/Kconfig.defconfig`

これはつまり、

- **左半分**: ホスト PC / 端末と通信し、最終的に keymap を評価する側
- **右半分**: キー入力イベントを左半分へ送る側

という意味です。  
そのため、ホストから見える挙動を確認する場合は、まず左半分の状態を重視して調べる必要があります。

### Studio / 永続設定

- 左半分では次の設定が有効です
  - `CONFIG_ZMK_STUDIO=y`
  - `CONFIG_ZMK_STUDIO_LOCKING=y`
  - 出典: `config/boards/shields/poached_eggs/poached_eggs_left.conf`
- GitHub Actions の左側 build には `studio-rpc-usb-uart` snippet も追加されています
  - 出典: `build.yaml`
- keymap には Bluetooth control layer 上に `&studio_unlock` binding があります
  - 出典: `config/poached_eggs.keymap`

これらの設定から、このリポジトリの左側 firmware は ZMK Studio と併用される前提で作られていると判断できます。

## 原因分析

今回の現象として最もあり得る流れは次のとおりです。

1. 以前に保存された runtime keymap またはその他の ZMK 永続設定がフラッシュ領域に残っている
2. ユーザーが `config/poached_eggs.keymap` を編集し、push して GitHub Actions から新しい firmware を取得する
3. 左右のデバイスに firmware を再書き込みする
4. 起動時に ZMK が永続設定を復元し、新しく compile されたデフォルト keymap ではなく、保存済みの runtime keymap / 状態を使い続ける
5. その結果、ユーザーからは「`.keymap` の変更が反映されていない」ように見える

左側 firmware で ZMK Studio と永続的な runtime 編集機能が有効になっているため、この説明はリポジトリの内容と整合します。

## 誤診しやすい理由

- 右半分は split central ではないため、右側だけを書き込んでもホスト側の keymap 挙動の説明にはなりません
- 左右の firmware が正常に書き込めたとしても、それは**永続設定が消去されたことを意味しません**
- 変更前のこのリポジトリでは、通常の left / right firmware しか生成されず、reset 用 image がありませんでした

## このリポジトリで行った変更

`build.yaml` に、以下の build を追加しました。

- `seeeduino_xiao_ble + settings_reset`

これにより、GitHub Actions から **ZMK の永続設定を消去するための artifact** を取得できるようになります。  
その後に通常の Poached Eggs firmware を再書き込みすることで、compiled keymap を反映しやすくなります。

## 推奨復旧手順

1. GitHub Actions を実行し、以下の artifact を取得する
   - `poached_eggs_left`
   - `poached_eggs_right`
   - `settings_reset`
2. **左半分** に `settings_reset` を書き込む
3. **右半分** にも `settings_reset` を書き込む
4. 続けて通常 firmware を再書き込みする
   - 左半分: `poached_eggs_left`
   - 右半分: `poached_eggs_right`
5. 以前の Bluetooth pairing が残っている場合は、ホスト側の登録を削除して再ペアリングする
6. 反映確認は、まず **左側 / central 側の挙動** を基準に行う

## 検証メモ

- リポジトリ内の build 設定、shield 設定、keymap 設定を直接確認しました
- この環境には `west` が入っていないため、ローカルでのフル ZMK build は実行できませんでした
- 今回の変更は意図的に最小限にしており、**不足していた reset artifact の build 追加** と **調査結果の記録** のみに絞っています
