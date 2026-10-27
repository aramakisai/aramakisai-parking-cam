#include "camera.h"

#include <Arduino.h>

#include "config.h"

namespace {

constexpr size_t MAX_FRAME_BYTES = 1048576;

void discardFrames(int count) {
  for (int i = 0; i < count; i++) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (fb) esp_camera_fb_return(fb);
    delay(100);
  }
}

}  // namespace

bool cameraInit(int exposure) {
  camera_config_t c = {};
  c.ledc_channel = LEDC_CHANNEL_0;
  c.ledc_timer = LEDC_TIMER_0;
  c.pin_d0 = 5;
  c.pin_d1 = 18;
  c.pin_d2 = 19;
  c.pin_d3 = 21;
  c.pin_d4 = 36;
  c.pin_d5 = 39;
  c.pin_d6 = 34;
  c.pin_d7 = 35;
  c.pin_xclk = 0;
  c.pin_pclk = 22;
  c.pin_vsync = 25;
  c.pin_href = 23;
  c.pin_sccb_sda = 26;
  c.pin_sccb_scl = 27;
  c.pin_pwdn = 32;
  c.pin_reset = -1;
  c.xclk_freq_hz = 20000000;
  c.pixel_format = PIXFORMAT_JPEG;
  c.frame_size = FRAMESIZE_UXGA;
  c.jpeg_quality = 10;
  c.fb_count = 1;
  c.fb_location = CAMERA_FB_IN_PSRAM;
  c.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  esp_err_t err = esp_camera_init(&c);
  if (err != ESP_OK) {
    Serial.printf("[camera] init failed err=0x%x\n", err);
    return false;
  }

  sensor_t* s = esp_camera_sensor_get();
  Serial.printf("[camera] init ok pid=0x%02x\n", s->id.PID);
  s->set_whitebal(s, 1);
  s->set_awb_gain(s, 1);
  s->set_wb_mode(s, 0);
  s->set_lenc(s, 1);
  s->set_ae_level(s, exposure);
  s->set_vflip(s, IMAGE_VFLIP);
  s->set_hmirror(s, IMAGE_HMIRROR);

  // センサー起動直後は露出・ホワイトバランスが安定しない
  delay(2000);
  discardFrames(5);
  return true;
}

camera_fb_t* cameraCapture() {
  // フレームバッファ1枚だと前回の古いフレームが残るため、読み捨てて最新にする
  discardFrames(3);
  for (int i = 0; i < 20; i++) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
      delay(500);
      continue;
    }
    if (fb->len > MAX_FRAME_BYTES) {
      esp_camera_fb_return(fb);
      return nullptr;
    }
    Serial.printf("[capture] bytes=%u\n", (unsigned)fb->len);
    return fb;
  }
  return nullptr;
}

void cameraSetExposure(int exposure) {
  sensor_t* s = esp_camera_sensor_get();
  if (s) s->set_ae_level(s, exposure);
}
