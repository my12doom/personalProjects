#include "basicRSA.h"
#include "SHA1.h"

extern unsigned int dwindow_n[32];
bool verify_signature(const DWORD *checksum, const DWORD *signature); // checksum is 20byte, signature is 128byte, true = match
int verify_file(wchar_t *file); //return value:
int video_checksum(wchar_t *file, DWORD *checksum);	// checksum is 20byte
int find_startcode(wchar_t *file);