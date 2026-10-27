# Research & Design Decisions

## Summary
- **Feature**: `parking-camera-firmware`
- **Discovery Scope**: New Feature(既存コードなし。試作ファームと実機の実測値あり)
- **Key Findings**:
  - Arduino core 2.0.17(ESP-IDF 4.4.7)のWPA2-EnterpriseはCA証明書の検証だけができ、認証サーバー名(ドメイン)の照合ができない。mbedTLSは`CONFIG_MBEDTLS_HAVE_TIME_DATE`が無効で、HTTPSと802.1Xのどちらでも証明書の有効期間を確かめない
  - Arduino coreの事前ビルドのブートローダーはフラッシュ暗号化を含まない。暗号化には`framework = arduino, espidf`のビルド、eFuseの焼き込み、鍵の保管が要り、負担が大きいため使わない。秘密値はフラッシュに平文で残り、運用で対処する
  - `cms.aramakisai.com`の証明書はCloudflareのUniversal SSLで、現在は`WE1`(Google Trust Services)→`GTS Root R4`。CloudflareはLet's Encrypt・Google Trust Services・SSL.comのいずれかから発行するため、ルートCAは3社分を持つ

## Research Log

### WPA2-Enterprise(802.1X)のAPI
- **Context**: 要件4.8〜4.11。PEAP/TTLS、MSCHAPv2、CA証明書の任意指定
- **Sources Consulted**: `~/.platformio/packages/framework-arduinoespressif32/libraries/WiFi/src/WiFiSTA.{h,cpp}`、`tools/sdk/esp32/include/wpa_supplicant/esp_supplicant/include/esp_wpa2.h`、`tools/sdk/esp32/lib/libwpa_supplicant.a`のシンボル
- **Findings**:
  - `WiFi.begin(ssid, WPA2_AUTH_PEAP|WPA2_AUTH_TTLS, identity, username, password, ca_pem)`。内部で`esp_wifi_sta_wpa2_ent_set_ca_cert(ca_pem, strlen(ca_pem))`・`set_identity`・`set_username`・`set_password`・`esp_wifi_sta_wpa2_ent_enable`を呼ぶ。`ca_pem`が`NULL`でも前回のCA証明書を消さない
  - 実機: `ca_pem`に渡すと、どのCAでも接続に失敗した(長さに終端のNULを含めないためと推定)。`WiFi.begin`の前に`esp_wifi_sta_wpa2_ent_disable`と`clear_ca_cert`・`clear_identity`・`clear_username`・`clear_password`を呼び、`set_ca_cert(pem, strlen(pem) + 1)`で設定し、`ca_pem`を`NULL`にすると接続できた
  - 実機: GunetEAPへの接続は1.3〜8.5秒(RSSI -69〜-82dBm)。電源投入直後の最初の接続が15秒で失敗することが2回あり、試し直しで成功した
  - `identity`に`NULL`を渡すと`strlen(NULL)`になる。匿名IDが空の場合はユーザー名を渡す
  - パスワードが64文字を超えるとログを出すだけで続行する
  - TTLSの内部認証は`esp_wifi_sta_wpa2_ent_set_ttls_phase2_method(ESP_EAP_TTLS_PHASE2_MSCHAPV2)`で指定する
  - ドメイン照合のAPI(`domain_match`・`altsubject_match`相当)は`libwpa_supplicant.a`に存在しない。サーバー名の照合はIDF 5.1以降(Arduino core 3.x)の`esp_eap_client_set_domain_name`が必要
  - CA証明書の束(`esp_crt_bundle`)を802.1Xで使うAPIもIDF 4.4には無い。CA証明書は明示的に1つ以上のPEMで渡す
  - TLSは`tls_mbedtls.c`(`CONFIG_WPA_MBEDTLS_CRYPTO=y`)
- **Implications**: CA証明書を与えた場合も、同じCAが発行した別名義の証明書を持つ偽のアクセスポイントを区別できない。制約としてdesignのSecurity Considerationsに書く

### 群馬大学GunetEAP
- **Context**: 設置先の例
- **Sources Consulted**: [無線LAN接続マニュアル(802.1X) 2024-10-18版](https://www.media.gunma-u.ac.jp/content/files/manual/wlan_guide_802.1x_20241018.pdf)
- **Findings**: PEAP、フェーズ2はMSCHAPV2、CA証明書は「システム証明書を使用」、ドメイン`gunma-u.ac.jp`、匿名IDは入力しない。サーバー証明書の表示名は`rad.gunma-u.ac.jp`で、マニュアルのフィンガープリントは版ごとに変わる(サーバー証明書は定期的に更新される)。発行元のルートCA名はマニュアルに無い。接続できない端末向けに`GunetWPA`がある(キャプティブポータル方式)
- **実機で確認したチェーン**: `rad.gunma-u.ac.jp`←NII Open Domain CA - G8 RSA←SECOM TLS RSA Root CA 2024。サーバーはSecurity Communication RootCA2によるクロス証明書版を送る。SECOM TLS RSA Root CA 2024(自己署名版、`http://repo2.secomtrust.net/root/tlsrsa/tlsrsarootca2024.cer`、2049年まで)とSecurity Communication RootCA2のどちらを設定しても接続でき、ISRG Root X1など他のCAでは接続できなかった。発行者はLinuxのwpa_supplicantのログの`CTRL-EVENT-EAP-PEER-CERT`で確かめられる
- **Implications**: サーバー証明書そのものを固定する方式は更新で壊れる。GunetEAPの既定はSECOM TLS RSA Root CA 2024にする

### 証明書の有効期間と端末の時刻
- **Context**: 端末の時刻が証明書検証に使われるか
- **Sources Consulted**: `tools/sdk/esp32/sdkconfig`、`tools/sdk/esp32/include/mbedtls/port/include/mbedtls/esp_config.h`
- **Findings**: `CONFIG_MBEDTLS_HAVE_TIME=y`、`# CONFIG_MBEDTLS_HAVE_TIME_DATE is not set`。`esp_config.h`は後者が無いと`MBEDTLS_HAVE_TIME_DATE`を`#undef`する。mbedTLSは`MBEDTLS_HAVE_TIME_DATE`が無いと証明書の期限切れ・未来日付を判定しない。802.1Xのmbedtlsも同じ設定
- **Implications**: 時刻が合う前でもTLSは成立する(時刻合わせの代替経路にHTTPSを使える)。期限切れの証明書は検出しない

### SNTP
- **Findings**: `CONFIG_LWIP_SNTP_MAX_SERVERS=3`、`CONFIG_LWIP_DHCP_GET_NTP_SRV=y`、既定の再同期`CONFIG_LWIP_SNTP_UPDATE_DELAY=10800000`(3時間)。`sntp_set_sync_interval`で変更できる
- **実機**: GunetEAPでNTPは4.2〜18.6秒で合った。`cms.aramakisai.com`の`/`への要求は404だが`Date`が付き、それで時刻を合わせられた
- **Implications**: 外向きUDP 123が遮られる学内網に備えて、HTTPS応答の`Date`ヘッダーで時刻を合わせる代替経路を持つ

### フラッシュ暗号化
- **Context**: 持ち去られた端末から秘密値を読み出されるおそれ
- **Sources Consulted**: `tools/sdk/esp32/sdkconfig`、`tools/sdk/esp32/bin/bootloader_dio_40m.elf`の文字列、`~/.platformio/platforms/espressif32/platform.py`・`builder/frameworks/espidf.py`・`builder/main.py`、ESP-IDF 4.4のFlash Encryptionの文書
- **Findings**:
  - 事前ビルドのsdkconfigは`# CONFIG_SECURE_FLASH_ENC_ENABLED is not set`。ブートローダーに初回起動時の暗号化処理が無い
  - `platform.py`は`framework = arduino, espidf`のとき`framework-espidf`を`~3.40407.0`(IDF 4.4.7)に下げる。`espidf.py`はIDF 4.x向けの分岐を残している
  - Arduino coreに同梱のIDFは`v4.4.7 38eeba213a`、esp32-cameraは`f0bb429`(`tools/sdk/versions.txt`)
  - 事前ビルドのsdkconfigは`CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH=y`。コアダンプはRAMの内容(トークン・パスワードを含む)をフラッシュに書く
  - ESP32の開発モードは、UARTのダウンロードモードから端末自身に暗号化させて任意のアプリを書き込めるため、持ち去った者が読み出し用のアプリを書いて秘密値を取り出せる。本番モードは`DISABLE_DL_ENCRYPT`・`DISABLE_DL_DECRYPT`・`DISABLE_DL_CACHE`とJTAG無効を焼き、鍵を知らない者は動くアプリを書けない
  - ホストで生成した鍵を事前に焼くと、本番モードでもホストで暗号化した画像をUARTで書き直せる
  - NVSはフラッシュ暗号化の対象外(NVS暗号化は別機能)。`WiFi.persistent(false)`でWiFiの接続情報をNVSに書かない
- **Implications**: フラッシュ暗号化は採らない(Design Decisions参照)。ビルド構成は試作と同じ`framework = arduino`のままにする

### HTTPSの送信先
- **Sources Consulted**: `openssl s_client -connect cms.aramakisai.com:443 -servername cms.aramakisai.com -showcerts`(2026-10-22)、[Cloudflare Certificate authorities](https://developers.cloudflare.com/ssl/reference/certificate-authorities/)
- **Findings**: 葉`CN=aramakisai.com`(SAN `aramakisai.com`、`*.aramakisai.com`、2027-01-10まで)、中間`WE1`、`GTS Root R4`(GlobalSign Root CAによる相互署名版も送られる)。TLS 1.3で接続。Universal SSLの発行元はLet's Encrypt・Google Trust Services・SSL.comで、ECDSAは3社とも発行する
- **実機**: 6ルートを連結した8,449バイトの`setCACert`で、`cms.aramakisai.com`に検証付きで接続できた(約2.8秒)。IPアドレスを直に指定すると失敗し、ホスト名の照合が効いている
- **Implications**: 信頼するルートは3社分(ISRG Root X1・X2、GTS Root R1・R4、SSL.com Root ECC・RSA)。ESP32のmbedTLS 2.28はTLS 1.2で接続する(Cloudflareは対応)。`WiFiClientSecure`は`mbedtls_ssl_set_hostname`でホスト名を照合する

### HTTPクライアント
- **Findings**: `HTTPClient`の既定タイムアウトは5000ms(`setConnectTimeout`・`setTimeout`で変更)。`WiFiClientSecure::setHandshakeTimeout`は秒指定。`collectHeaders`で`Date`を受け取れる
- **Implications**: 受信側の取り決め(`POST(fb->buf, fb->len)`、ヘッダー4つ)をそのまま実装できる

### 送信時間の実測
- **Sources Consulted**: 実機からspeed.cloudflare.comへのPOST(窓際、RSSI -70〜-78dBm)
- **Findings**: UXGA(約207〜223KB)は合計4.8〜5.7秒(接続は約3秒、本文は1.8〜2.7秒)。XGA(約96〜100KB)は合計3.8〜4.1秒(接続は約3秒、本文は0.7〜0.9秒)
- **Implications**: 接続の約3秒が大半を占め、XGAにしても1〜2秒しか縮まないのでUXGAのままにする。送信の上限(接続10秒+ハンドシェイク15秒、やり取り20秒)は実測の3倍以上ある

### 撮影の実測(試作ファーム)
- **Findings**: 起動直後は数フレーム捨てる。センサー設定の変更直後は`esp_camera_fb_get()`が失敗しやすく、500ms待ちで最大20回の再試行で全て成功した。AWBの切り替え直後は3フレーム捨てても色が赤く、落ち着くまで1〜2秒かかる。WBのモードはどれも色かぶりが強まる。`ae_level` -1/-2は自然に暗くなる
- **Implications**: AWB・レンズ補正は初期化時に一度だけ設定し切り替えない。露出補正は応答を受けた時点で変え、次の撮影(数分後)までに落ち着かせる

## Architecture Pattern Evaluation

| Option | Description | Strengths | Risks / Limitations | Notes |
|--------|-------------|-----------|---------------------|-------|
| 単一ループ+純粋ロジック分離 | `loop()`が1周期を順に実行し、判断は`lib/`の関数に任せる | 状態が少なく、失敗を戻り値で扱える | 1周期中は他の処理が止まる | steeringの構成方針どおり |
| FreeRTOSタスク分割 | 撮影・送信・WiFi監視を別タスクにする | 並行に動く | 共有状態とロックが増える | 5分に1枚では得るものが無い |

## Design Decisions

### Decision: フラッシュ暗号化を使わない
- **Context**: 802.1Xに実行委員個人の大学アカウントを書き込むため、持ち去られたときに秘密値を読み出されるおそれがある
- **Alternatives Considered**:
  1. 本番モードのフラッシュ暗号化 — `framework = arduino, espidf`への変更、取り消せないeFuseの焼き込み、端末ごとの鍵の保管と、暗号化したアプリでの書き直しが要る
  2. 使わない — 試作と同じ構成のまま、運用で対処する
- **Selected Approach**: 2
- **Trade-offs**: 秘密値はフラッシュに平文で残る。端末の固定、紛失時のパスワード変更とトークン無効化、開催後のパスワード変更で補う

### Decision: 802.1Xのサーバー名を照合できない制約を受け入れる
- **Context**: Arduino core 2.0.17では認証サーバー名を照合できない
- **Alternatives Considered**:
  1. Arduino core 3.x(IDF 5.1以降)へ移り`esp_eap_client_set_domain_name`を使う — platformの変更と実機確認のやり直しになるため採らない
  2. `GunetWPA`を使う — キャプティブポータル方式で、ブラウザの無い端末では使えないため採らない
  3. ルートCAを設定する運用で受け入れる
- **Selected Approach**: 3。開催後にパスワードを変える

### Decision: 時刻合わせの代替経路
- **Context**: 時刻が合うまで撮影しない。学内網でNTPが遮られると動かない
- **Selected Approach**: SNTPが30秒で合わなければ、送信先と同じホストへのHTTPS要求の`Date`ヘッダーで合わせる(TLSは時刻に依存しない)

## Risks & Mitigations
- 802.1Xのサーバー名照合ができない — CA証明書を必ず与える運用にし、開催後にアカウントのパスワードを変える
- 秘密値がフラッシュに平文で残る — 端末を固定し、紛失時はすぐにパスワード変更とトークン無効化、開催後にパスワード変更
- CloudflareのCA切り替え — 3社のルートを持つ

## References
- [Cloudflare Certificate authorities](https://developers.cloudflare.com/ssl/reference/certificate-authorities/)
- [群馬大学 無線LAN接続マニュアル(802.1X)](https://www.media.gunma-u.ac.jp/content/files/manual/wlan_guide_802.1x_20241018.pdf)
- 受信側の取り決め: `aramakisai-web`の`.kiro/specs/parking-camera-endpoint/design.md`
