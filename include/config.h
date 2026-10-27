#pragma once

#include "config_check.h"

#define WIFI_AUTH_PSK 1
#define WIFI_AUTH_EAP_PEAP 2
#define WIFI_AUTH_EAP_TTLS 3

// 実値のファイルが無いときはここでビルドが止まる。雛形 device_config.example.h を写して作る。
#include "device_config.h"

#ifndef CMS_INGEST_URL
#error "CMS_INGEST_URL が未設定"
#endif
#ifndef DEVICE_TOKEN
#error "DEVICE_TOKEN が未設定"
#endif
#ifndef WIFI_AUTH
#error "WIFI_AUTH が未設定"
#endif
#ifndef WIFI_SSID
#error "WIFI_SSID が未設定"
#endif

#ifndef EAP_IDENTITY
#define EAP_IDENTITY ""
#endif
#ifndef CAPTURE_INTERVAL_SEC
#define CAPTURE_INTERVAL_SEC 300
#endif
#ifndef MONITOR_START_MIN
#define MONITOR_START_MIN 540
#endif
#ifndef MONITOR_END_MIN
#define MONITOR_END_MIN 1050
#endif
#ifndef IMAGE_VFLIP
#define IMAGE_VFLIP 0
#endif
#ifndef IMAGE_HMIRROR
#define IMAGE_HMIRROR 0
#endif
#ifndef EXPOSURE_DEFAULT
#define EXPOSURE_DEFAULT 0
#endif

static_assert(isHttpsUrl(CMS_INGEST_URL), "CMS_INGEST_URL は https:// で始まりホスト名が要る");
static_assert(isNonEmpty(DEVICE_TOKEN), "DEVICE_TOKEN が空");
static_assert(isNonEmpty(WIFI_SSID), "WIFI_SSID が空");
static_assert(isValidInterval(CAPTURE_INTERVAL_SEC),
              "CAPTURE_INTERVAL_SEC は 60〜3600 で 3600 を割り切る値");
static_assert(isValidWindow(MONITOR_START_MIN, MONITOR_END_MIN),
              "監視時間は 0 <= 開始 < 終了 <= 1439 (0時からの分)");
static_assert(isValidExposure(EXPOSURE_DEFAULT), "EXPOSURE_DEFAULT は -2〜2");
static_assert(IMAGE_VFLIP == 0 || IMAGE_VFLIP == 1, "IMAGE_VFLIP は 0 か 1");
static_assert(IMAGE_HMIRROR == 0 || IMAGE_HMIRROR == 1, "IMAGE_HMIRROR は 0 か 1");

#if WIFI_AUTH == WIFI_AUTH_PSK
#ifndef WIFI_PASSWORD
#error "PSK では WIFI_PASSWORD が要る"
#endif
static_assert(isNonEmpty(WIFI_PASSWORD), "PSK では WIFI_PASSWORD が要る");
#elif WIFI_AUTH == WIFI_AUTH_EAP_PEAP || WIFI_AUTH == WIFI_AUTH_EAP_TTLS
#if !defined(EAP_USERNAME) || !defined(EAP_PASSWORD)
#error "802.1X では EAP_USERNAME と EAP_PASSWORD が要る"
#endif
static_assert(isNonEmpty(EAP_USERNAME), "802.1X では EAP_USERNAME が要る");
static_assert(isNonEmpty(EAP_PASSWORD), "802.1X では EAP_PASSWORD が要る");
#else
#error "WIFI_AUTH は WIFI_AUTH_PSK / WIFI_AUTH_EAP_PEAP / WIFI_AUTH_EAP_TTLS のいずれか"
#endif
