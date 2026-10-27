#pragma once

#include "esp_camera.h"

bool cameraInit(int exposure);
camera_fb_t* cameraCapture();
void cameraSetExposure(int exposure);
