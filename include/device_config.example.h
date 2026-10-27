// このファイルを device_config.h に写して値を入れる。device_config.h は Git の追跡対象外。
#pragma once

#include "cms_root_ca.h"
#include "eap_ca_gunet.h"

// ---- 必須 ----
#define CMS_INGEST_URL ""   // https:// で始まる受信エンドポイントのURL
#define DEVICE_TOKEN ""     // 駐車場ごとに発行したデバイス用トークン
#define WIFI_SSID ""

// WIFI_AUTH_PSK / WIFI_AUTH_EAP_PEAP / WIFI_AUTH_EAP_TTLS
#define WIFI_AUTH WIFI_AUTH_PSK

// PSK のとき必須
#define WIFI_PASSWORD ""

// 802.1X のとき必須。EAP_IDENTITY が空なら EAP_USERNAME を使う
#define EAP_USERNAME ""
#define EAP_PASSWORD ""
// #define EAP_IDENTITY ""

// 認証サーバーのルートCA。GunetEAP 用。別の設置先では PEM 文字列に差し替える。
// 定義しないと認証サーバーを検証しない
#define EAP_CA_PEM GUNET_EAP_CA_PEM

// ---- 任意(コメントを外すと既定値を上書き) ----
// #define CAPTURE_INTERVAL_SEC 300   // 60〜3600 で 3600 を割り切る値
// #define MONITOR_START_MIN 540      // 日本時間の0時からの分 (9:00)
// #define MONITOR_END_MIN 1050       // 同上 (17:30)。開始より後、1439以下
// #define IMAGE_VFLIP 0              // 0 / 1
// #define IMAGE_HMIRROR 0            // 0 / 1
// #define EXPOSURE_DEFAULT 0         // -2〜2
