# Requirements Document

## Project Description (Input)
荒牧祭の駐車場を、建物の4階から定期的に撮影して画像を送り続けるカメラ端末(ESP32-CAM)のファームウェアを作る。端末は撮影したJPEGを、デバイス用トークンを付けて、設定されたエンドポイントへHTTPSでPOSTする。

### 前提
- 端末はAI Thinker ESP32-CAM互換ボード(ESP32-D0WD-V3、flash 4MB、PSRAM約4MB、カメラOV2640)。検証機の実測で、UXGA(1600x1200)・`jpeg_quality=10`のJPEGは約221KB。起動直後の数フレームは使わずに捨てる。基板の向きによって画像が90°回転して写ることがある。
- 建物の4階から駐車場1箇所をカメラ1台で撮る。ナンバープレートや人の顔は判読できない。2.4GHzのWiFiとコンセント電源は現地で確保済み。WiFiの接続方式は設置場所によって、WPA2-Personal(SSIDとパスワード)とIEEE 802.1X(WPA2-Enterprise)のどちらかになる。
- 撮影と送信は5分間隔を既定とする。撮影は時計の5分の区切り(毎時0分、5分、…)に揃える。
- 駐車場を見るのは日本時間の9:00〜17:30を既定とし、その時間内だけ撮影・送信する。
- デバイス用トークンはカメラごとに発行されたものを端末ごとに書き込む。端末は、どの駐車場を撮っているかを知らず、駐車場を識別する値を別に持たない。

### 目標
- 既定5分間隔で撮影・送信を行い、間隔は現地で変えられること。
- 通信の失敗や端末のハングがあっても、人が操作しなくても動き続けること。
- 設置前に、シリアル出力で端末の動作を確かめられること。

## Introduction
駐車場カメラ端末(Parking Camera)のファームウェアの要件を定める。端末は駐車場を定期的に撮影し、設定されたエンドポイントへ画像を送る。

## Boundary Context
- **In scope**: 端末の時刻合わせ、監視時間内の定期撮影、画像の向きの補正、HTTPSでの画像送信とデバイス用トークンの付与、送信先の応答に含まれる写りの補正値の反映、書込み時の設定(WiFiの接続方式の選択を含む)、秘密値の扱い、通信失敗からの再試行、ハング時の自己回復、設置作業のための動作状況の出力
- **Out of scope**: 送信先での画像の扱い、写りの補正値の決め方、設置後の遠隔からの端末の設定変更(写りの補正値の反映を除く)やファームウェア更新、クライアント証明書を使うEAP-TLSによる802.1X認証、バッテリー駆動や省電力化、画像の顔やナンバーのぼかし処理、端末での混雑の判定や駐車台数の計数

## Requirements

### Requirement 1: 定期撮影
**Objective:** As a 実行委員, I want カメラ端末が決まった間隔で駐車場を撮り続けてほしい, so that 駐車場の最新の様子が人手を介さずに送信先へ届き続ける

#### Acceptance Criteria
1. When カメラ端末が起動し初期化と時刻合わせを終える, the Parking Camera shall 監視時間内であれば最初の撮影を行い、以降は監視時間内の、撮影間隔を刻みとする時計の区切り(撮影間隔が5分なら9:00、9:05、…、17:30)ごとに撮影する
2. Where 撮影間隔が設定で与えられていない, the Parking Camera shall 撮影間隔を5分とする
3. The Parking Camera shall 2回目以降の撮影の開始を、送信にかかった時間に関わらず、時計の区切りの時刻におおむね合わせる
4. The Parking Camera shall 1600x1200の解像度のJPEG画像として撮影する
5. When カメラの起動直後に撮影する, the Parking Camera shall 露出やホワイトバランスが落ち着く前の数フレームを捨て、その後のフレームを送信に使う
6. If 撮影に失敗する, the Parking Camera shall その周期の送信を行わず、次の周期で撮影をやり直す
7. While 監視時間外である, the Parking Camera shall 撮影と送信を行わない

### Requirement 2: 画像の向き
**Objective:** As a 実行委員, I want 基板の取り付け向きに関わらず、駐車場が正しい向きで写った画像を送りたい, so that 送信先で向きのずれのない画像を使える

#### Acceptance Criteria
1. Where 画像の上下反転・左右反転が設定で指定されている, the Parking Camera shall 指定どおりに反転した画像を撮影する
2. The Parking Camera shall 反転の指定を、ファームウェアのコードを変えずに書込み時の設定だけで変えられるようにする
3. If 基板の向きが原因で画像が90°回転して写る, the Parking Camera shall その補正を設置の向きの調整で行う前提とし、回転した画像をそのまま送る

### Requirement 3: 画像の送信
**Objective:** As a 実行委員, I want 撮った画像が確実に送信先へ届くようにしたい, so that 送信先が常に最新の1枚を受け取れる

#### Acceptance Criteria
1. When 撮影に成功する, the Parking Camera shall 撮影したJPEG画像を、設定されたエンドポイント(送信先URL)へHTTPSでPOSTする
2. The Parking Camera shall 送信のたびに、設定されたデバイス用トークンを認証情報として付ける
3. The Parking Camera shall 送信先のサーバー証明書を検証し、検証に失敗した相手には画像を送らない
4. The Parking Camera shall 1回の撮影につき、その撮影で得た1枚の画像だけを送る(撮影と送信を同じ周期で行い、送信だけを別の間隔で繰り返さない)
5. When 送信先が成功以外の応答を返す, the Parking Camera shall その周期の送信を失敗として扱う
6. The Parking Camera shall 送信のたびに、その画像の撮影時刻と、撮影したときの露出補正値を付ける
7. The Parking Camera shall 送信のたびに、端末の状態(WiFiの電波強度、起動からの経過時間、直前の再起動の理由、その送信の直前まで続いた失敗の回数、ファームウェアの版)を付ける

### Requirement 4: 書込み時の設定
**Objective:** As a 実行委員, I want 設置場所ごとの値をファームウェアのコードを変えずに与えたい, so that 現地の状況に合わせて端末を用意できる

#### Acceptance Criteria
1. The Parking Camera shall 送信先URL、デバイス用トークン、WiFiの接続方式とその接続情報、撮影間隔、監視時間、画像の反転の指定、露出補正の既定値を、書込み時の設定として受け取る
2. The Parking Camera shall 撮影間隔の変更を、設定値の変更とファームウェアの再書込みだけで行えるようにする
3. If 必須の設定値(送信先URL、デバイス用トークン、選択したWiFiの接続方式に必要な接続情報)が欠けている, the Parking Camera shall ビルドまたは起動の時点でそのことを示し、欠けたまま送信を試みない
4. If 撮影間隔に正の値でない値、または1時間を割り切れない値が与えられる, the Parking Camera shall その値を受け付けず、ビルドまたは起動の時点でそのことを示す
5. Where 露出補正の既定値が設定で与えられていない, the Parking Camera shall 露出補正を0とする
6. The Parking Camera shall WiFiの接続方式として、WPA2-Personal(SSIDとパスワード)とIEEE 802.1X(WPA2-Enterprise)のどちらかを、書込み時の設定だけで選べるようにする
7. Where WiFiの接続方式にWPA2-Personalが選ばれている, the Parking Camera shall SSIDとパスワードを使ってWiFiに接続する
8. Where WiFiの接続方式にIEEE 802.1Xが選ばれている, the Parking Camera shall SSID、EAPの方式、ID(identity)、ユーザー名、パスワード、認証サーバーのCA証明書(任意)を書込み時の設定として受け取り、IDとパスワードによる認証でWiFiに接続する
9. The Parking Camera shall IEEE 802.1XのEAPの方式として、PEAPとEAP-TTLSのどちらも、内部認証をMSCHAPv2として選べるようにする
10. Where IEEE 802.1Xの認証サーバーのCA証明書が設定で与えられている, the Parking Camera shall そのCA証明書で認証サーバーのサーバー証明書を検証し、検証に失敗した相手には接続しない
11. Where IEEE 802.1Xの認証サーバーのCA証明書が設定で与えられていない, the Parking Camera shall サーバー証明書の検証を行わずに接続し、検証していないことを警告としてシリアルに出力する
12. Where 監視時間が設定で与えられていない, the Parking Camera shall 監視時間を日本時間の9:00〜17:30(両端の時刻を含む)とする
13. If 監視時間の開始が終了より後である、または時刻として正しくない, the Parking Camera shall その値を受け付けず、ビルドまたは起動の時点でそのことを示す

### Requirement 5: 端末の秘密値の扱い
**Objective:** As a 実行委員, I want トークンやWiFiのパスワード、802.1Xの認証情報が外に漏れないようにしたい, so that 第三者に送信先へ画像を送られたり、会場のWiFiを使われたりしない

#### Acceptance Criteria
1. The Parking Camera shall デバイス用トークン、WiFiのSSIDとパスワード、IEEE 802.1XのIDとユーザー名とパスワード、送信先URLの実際の値を、リポジトリにコミットされるファイルに含めない
2. The Parking Camera shall 設定値の雛形をリポジトリに置き、実際の値を入れるファイルはGitの追跡対象から外す
3. The Parking Camera shall シリアル出力に、デバイス用トークン、WiFiのパスワード、IEEE 802.1Xのユーザー名とパスワードを出さない
4. If 送信先URLがHTTPSでない, the Parking Camera shall 画像とトークンを送らず、そのことをシリアルに出力する

### Requirement 6: 端末の失敗からの回復
**Objective:** As a 実行委員, I want 通信が途切れたり端末が固まったりしても、人が操作せずに動き続けてほしい, so that 手の届きにくい場所に置いた端末を開催中に見に行かなくて済む

#### Acceptance Criteria
1. If WiFiに接続できない、または接続が切れている, the Parking Camera shall その周期の送信を失敗として扱い、次の周期の前にWiFiへの再接続を試みる
2. If 送信が失敗する(接続できない、タイムアウトする、成功以外の応答を受ける), the Parking Camera shall 次の周期で新しく撮影した画像を送り、失敗した画像を再送しない
3. The Parking Camera shall WiFiへの接続と送信のそれぞれに時間の上限を設け、上限を超えた処理を打ち切って次の周期へ進む
4. If 処理が一定時間以上進まない状態になる, the Parking Camera shall ウォッチドッグにより自動で再起動する
5. If カメラの初期化に失敗する, the Parking Camera shall 再起動して初期化をやり直す
6. When 再起動から復帰する, the Parking Camera shall 人の操作なしに設定値を読み込み、撮影と送信の周期を再開する
7. While 失敗が連続している, the Parking Camera shall 処理を止めずに周期ごとの撮影と送信の試行を続ける

### Requirement 7: 設置作業のための動作状況の出力
**Objective:** As a 実行委員, I want 書込み直後や設置前に、端末が正しく動いているかを確かめたい, so that 現地に置く前に設定の誤りや通信の問題に気づける

#### Acceptance Criteria
1. When カメラ端末が起動する, the Parking Camera shall カメラの初期化の成否、WiFiへの接続の成否、時刻合わせの成否をシリアルに出力する
2. When 撮影と送信を1回行う, the Parking Camera shall 撮影した画像のサイズと、送信の成否(成功、または失敗の種類と送信先の応答コード)と、送信時点のWiFiの電波の強さ(RSSI)をシリアルに出力する
3. When ウォッチドッグなどによる再起動から復帰する, the Parking Camera shall 直前の再起動の理由をシリアルに出力する

### Requirement 8: 写りの補正値の反映
**Objective:** As a 実行委員, I want 送信先から返される補正値で端末の写りを直したい, so that 明るさが時間帯や天気で変わっても、見る人にとって見やすい画像を送り続けられる

#### Acceptance Criteria
1. When 送信先が成功の応答に写りの補正値(露出補正)を含めて返す, the Parking Camera shall 次の撮影からその補正値で撮影する
2. The Parking Camera shall 露出補正として-2〜+2の値を受け付ける
3. If 応答に含まれる補正値が範囲外または不正な値である, the Parking Camera shall その値を無視し、今の設定のまま撮影を続ける
4. If 応答に補正値が含まれない、または送信が失敗する, the Parking Camera shall 今の設定のまま撮影を続ける
5. When 電源断や再起動から復帰する, the Parking Camera shall 書込み時の既定値の露出補正で撮影を再開し、受け取った補正値を保存しない
6. The Parking Camera shall ホワイトバランスを常に自動とし、レンズ補正を有効にして撮影する

### Requirement 9: 端末の時刻
**Objective:** As a 実行委員, I want 端末の時計を正しい時刻に合わせておきたい, so that 撮影を時計の区切りに揃え、送信先が撮影時刻で画像の新しさを確かめられる

#### Acceptance Criteria
1. When WiFiに接続する, the Parking Camera shall ネットワーク上の時刻源から端末の時刻を合わせる
2. While 端末の時刻が一度も合っていない, the Parking Camera shall 撮影と送信を行わず、時刻が合っていないことをシリアルに出力し、時刻合わせを試み続ける
3. The Parking Camera shall 時刻が合ったあとも、一定の周期で時刻を合わせ直す
4. If 送信先が撮影時刻の不正を理由に画像を拒否する, the Parking Camera shall 次の撮影の前に時刻を合わせ直す
