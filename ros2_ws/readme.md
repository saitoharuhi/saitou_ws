# 備忘録: 顔ーどの読み方・使い方

以下は本ワークスペース（ESP32カメラ + ROS2連携）の簡単な備忘メモです。開発中の手順や注意点を短くまとめています。

## 概要
- 本プロジェクトは Seeed XIAO ESP32S3 Sense を用いてカメラ映像を取得し、ROS2 側で受信・表示・配信するための実装群です。
- 「顔ーど」はこのプロジェクト内で扱う顔検出／顔情報を指す内部名称（備忘録）。読み方は「かおーど」。

## リポジトリ構成（重要ファイル）
- esp32_control/esp32_control.ino : ESP32 側の Arduino スケッチ（カメラ取得・TCP送信・モータ/サーボ制御）
- ros2_ws/src/esp_radio_controlled_pkg : ROS2 パッケージ（image_reception_node 等）
- その他テストスクリプト: kadai3.py, serial_python.py, test.py

## 使い方（ざっくり）
1. ESP32 側
   - esp32_control/esp32_control.ino を開き、Wi‑Fi SSID/パスワードと PC の IP（pc_ip）を設定する。
   - ボードを Seeed XIAO ESP32S3 Sense に設定して書き込み、シリアルでログを確認する。
2. ROS2 側
   - ワークスペースでビルド: `colcon build --packages-select esp_radio_controlled_pkg`。
   - セットアップ: `source install/setup.bash`。
   - ノード起動: `ros2 run esp_radio_controlled_pkg image_reception_node`。
   - 画像は `/camera/image_raw` トピックで配信される（rqt_image_view で確認推奨）。

## トラブルシュート（簡潔）
- ESP が接続できない: esp の pc_ip が PC の実IPになっているか確認（`hostname -I`）。
- ポートが LISTEN していない: image_reception_node を起動し、`ss -ltnp | grep 5000` を確認。
- GUI（cv2.imshow）問題: Wayland/Qt のエラーが出る場合は DISPLAY を無効にしてトピック側で確認する。

## 開発メモ
- control_node（キーボード→コマンド送信）はネットワーク方式に合わせて要修正。ESP 側は現在 TCP クライアントでフレームを送信、コマンドはシリアル経由の実装。
- 変更を加えたファイルは git にコミットしておくこと。

---
短いメモなので追記・変更は随時行ってください。不要な部分は消して使ってください。