#pragma once

enum class ClockSource { None, Sntp, HttpDate };

void clockBegin();
bool clockEnsure();
void clockInvalidate();
ClockSource clockSource();
