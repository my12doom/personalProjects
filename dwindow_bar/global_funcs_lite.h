#pragma once
#include <Windows.h>
#include "..\libchecksum\libchecksum.h"

extern char g_passkey_big[128];
extern char *g_server_address;
#define g_server_gen_key "gen_key.php"
HRESULT download_url(char *url_to_download, char *out, int outlen = 64);
HRESULT check_passkey();
