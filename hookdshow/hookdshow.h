#pragma once

int enable_hookdshow();
int disable_hookdshow();
int stop_all_handles();
int clear_all_handles();
const wchar_t *URL2Token(const wchar_t *URL);
const wchar_t *Token2URL(const wchar_t *Token);
