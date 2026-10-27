# aramakisai-parking-cam

荒牧祭の駐車場を一定間隔で撮影し、JPEGを `aramakisai-web` の受信エンドポイント(`POST /api/parking-ingest`)へHTTPSで送るESP32-CAM(AI Thinker)向けファームウェア。受信・混雑判定・公開ページは `aramakisai-web` 側が担う。

## 動作

- 日本時間の監視時間内(既定 9:00〜17:30)に、区切りの時刻(既定は5分ごと)で1枚撮って送る。起動後に時刻が合った時点で監視時間内なら、区切りを待たず1枚目を撮る
- WiFiが切れてから繋がり直したとき(起動後の初回接続も含む)は、時刻が合い次第、監視時間外でも1回だけ撮って送り、疎通を確かめる。監視時間内の1枚目や区切りと重なれば、その1枚が兼ねる
- 監視時間外は撮影せず、WiFiと時刻を保って待つ
- 失敗した画像は再送せず、次の区切りで撮り直す。撮影か送信が6回連続で失敗すると再起動する。ウォッチドッグは90秒
- 受信側が返した露出補正値は次の撮影から使う。保存はしないので、再起動後は `EXPOSURE_DEFAULT` に戻る

## 設定

1. `include/device_config.example.h` を `include/device_config.h` に写して値を入れる。
2. `include/device_config.h` は `.gitignore` 済み。実値(トークン、WiFiの資格情報など)をコミットしない。

| 項目 | 内容 |
|---|---|
| `CMS_INGEST_URL` | `https://` で始まる受信エンドポイントのURL(必須) |
| `DEVICE_TOKEN` | 駐車場ごとに発行したデバイス用トークン(必須) |
| `WIFI_SSID` | SSID(必須) |
| `WIFI_AUTH` | `WIFI_AUTH_PSK` / `WIFI_AUTH_EAP_PEAP` / `WIFI_AUTH_EAP_TTLS` |
| `WIFI_PASSWORD` | PSKのとき必須 |
| `EAP_USERNAME` `EAP_PASSWORD` | 802.1Xのとき必須。`EAP_IDENTITY` を定義しなければ `EAP_USERNAME` を使う |
| `EAP_CA_PEM` | 認証サーバーのルートCA(PEM)。未定義だと認証サーバーを検証せず、起動時に警告を出す |
| `CAPTURE_INTERVAL_SEC` | 撮影間隔。60〜3600で3600を割り切る値(既定300) |
| `MONITOR_START_MIN` `MONITOR_END_MIN` | 監視時間。日本時間の0時からの分(既定 540 と 1050)。受信側の `parking_monitor` と合わせる |
| `IMAGE_VFLIP` `IMAGE_HMIRROR` | 上下反転・左右反転(0 / 1) |
| `EXPOSURE_DEFAULT` | 露出補正の既定値(-2〜2) |

必須値の欠け・不正値はビルド時に止まる。

### 別の設置先の802.1XのルートCA

雛形の `EAP_CA_PEM` はGunetEAP用(`include/eap_ca_gunet.h`)。別の設置先では、設置前に現地でLinuxのノートPCから接続し、wpa_supplicantのログ(`journalctl -u wpa_supplicant` など)の `CTRL-EVENT-EAP-PEER-CERT depth=… subject=…` で最上位の発行者を確かめ、そのCAの配布元からルート証明書のPEMを取得して `EAP_CA_PEM` に与える。

## ビルドと書込み

PlatformIOはPATHに入っていないため、フルパスで呼ぶ。

```bash
~/.platformio/penv/bin/pio test -e native      # ホスト単体テスト
~/.platformio/penv/bin/pio run -e esp32cam     # ビルド
~/.platformio/penv/bin/pio run -e esp32cam -t upload   # 書込み(CH340の自動リセット。GPIO0のジャンパー不要)
~/.platformio/penv/bin/pio device monitor      # シリアルモニタ(115200bps)
```

## 設置前の確認

シリアルは設置時の確認専用。トークン・パスワード・802.1Xの資格情報は出力しない。

- 起動から `[camera] init ok`、`[wifi] connected`、`[time] synced` まで人の操作なしに進む
- 本番の受信エンドポイントへ送り、`[upload] ok status=200` が出る。誤ったトークンでは `status=401` が出て、次の区切りで再試行する
- 設置場所の電波: `[wifi] connected` と `[upload]` のRSSIと送信時間(`ms`)を数周期見る。RSSI -70dBm以上で設置可。-70〜-80dBmは送信時間が10秒を超えないことを確かめる。-80dBm未満は位置を変える
- 監視時間外は撮影せず、開始の区切りで1枚目を送る(`MONITOR_START_MIN` を現在時刻の数分後にして確かめる)
- 窓越しに設置するときは、レンズをガラスに密着させ、周りを黒い布か筒で覆って室内の映り込みを防ぐ。17:30ごろに1枚撮り、映り込みが無いことを確かめる

## 紛失・開催後の対応

トークンとWiFiの資格情報(802.1Xでは実行委員個人の大学アカウント)は、フラッシュに平文で入る。

- 端末を紛失したら、CMSでデバイス用トークンを無効化し、大学アカウントのパスワードを変更する
- 開催後は、大学アカウントのパスワードを変更し、使わなくなったトークンを無効化する
