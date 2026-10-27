#pragma once

#include <stdint.h>

enum class WifiResult { Connected, Timeout };

WifiResult wifiConnect(uint32_t timeoutMs);
bool wifiIsConnected();
