#pragma once

#include <wchar.h>

int enable_hookdshow();
int disable_hookdshow();
const wchar_t *URL2Token(const wchar_t *URL);
const wchar_t *Token2URL(const wchar_t *Token);