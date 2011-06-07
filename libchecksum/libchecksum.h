#pragma  once

#include "basicRSA.h"
#include "SHA1.h"
#include <time.h>

typedef struct dwindow_license_struct
{
	DWORD id[32];			// license id,
							// first dword is number, second dword is reserved for later number, now is zero.
							// last(the 31th) dword is CRC32 checksum of other 31 bytes.
							// other bytes can be used to store special rights

	DWORD signature[32];	// the signature, RSA(signature) must == id
							// load & save only use this signature, id is ignored.
} dwindow_license;

typedef struct dwindow_license_detail_struct
{
	DWORD id;
	DWORD id_high;
	DWORD user_rights;
	DWORD ext[28];

	DWORD CRC32;

	DWORD signature[32];
} dwindow_license_detail;

typedef struct _dwindow_message_uncrypt
{
	unsigned char passkey[32];
	unsigned char requested_hash[20];
	unsigned char random_AES_key[32];
	unsigned char reserved[43];
	unsigned char zero;
}dwindow_message_uncrypt;

typedef struct _dwindow_passkey_big
{
	unsigned char passkey[32];
	unsigned char passkey2[32];		// should be same
	unsigned char ps_aes_key[32];
	__time64_t time_start;			// 8 byte
	__time64_t time_end;			// 8 byte
	unsigned char reserved[15];
	unsigned char zero;				// = 0
} dwindow_passkey_big;

extern unsigned int dwindow_n[32];
extern DWORD dwindow_network_n[32];
bool verify_signature(const DWORD *checksum, const DWORD *signature); // checksum is 20byte, signature is 128byte, true = match
int verify_file(wchar_t *file); //return value:
int video_checksum(wchar_t *file, DWORD *checksum);	// checksum is 20byte
int find_startcode(wchar_t *file);
HRESULT RSA_dwindow_public(const void *input, void *output);
HRESULT RSA_dwindow_network_public(const void *input, void *output);

// license funcs
bool is_valid_license(dwindow_license license);			// true = valid
bool load_license(wchar_t *file, dwindow_license *out);	// false = fail
bool save_license(wchar_t *file, dwindow_license in);	// false = fail
bool encrypt_license(dwindow_license license, DWORD *encrypt_out);	// out is 128byte, encrypt id for sending to server
bool decrypt_license(dwindow_license *license_out, DWORD *msg);		// msg is 128byte, decrypt license from message recieved from server
																	// warning: these two "en/de crypt" is not a pair
																	// encrypt <-> server_decrypt 
																	// server_encrypt <-> decrypt
																	// this is how it works.