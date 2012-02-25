// crypt.cpp : Ccrypt 的实现

#include "stdafx.h"
#include <math.h>
#include "crypt.h"
#include "..\AESFile\rijndael.h"
#include "..\renderer_prototype\ps_aes_key.h"

// Ccrypt

unsigned int dwindow_d[32] = {0x46bbf241, 0xd39c0d91, 0x5c9b9170, 0x43187399, 0x6568c96b, 0xe8a5445b, 0x99791d5d, 0x38e1f280, 0xb0e7bbee, 0x3c5a66a0, 0xe8d38c65, 0x5a16b7bc, 0x53b49e94, 0x11ef976d, 0xd212257e, 0xb374c4f2, 0xc67a478a, 0xe9905e86, 0x52198bc5, 0x1c2b4777, 0x8389d925, 0x33211e75, 0xc2cab10e, 0x4673bf76, 0xfdd2332e, 0x32b10a08, 0x4e64f572, 0x52586369, 0x7a3980e0, 0x7ce9ba99, 0x6eaf6bfe, 0x707b1206};
DWORD dwindow_network_d[32] = {0xf70b4b91, 0x4ea5db4f, 0xa3c3e23f, 0xb6e80229, 0xfed913d3, 0x19269824, 0x41b29509, 0xa652e1b9, 0x1e1f5d0e, 0x660b9683, 0xb0fe5d71, 0x36cc1f6b, 0x9f6ed8ec, 0xfbc2f93a, 0xadce8a8c, 0x75db6f8b, 0x7aa72c0f, 0x7b6dc001, 0x091aab56, 0xe2b07d17, 0x76fae5cf, 0x10b80462, 0xf401ea79, 0x575dd549, 0x3fbd6d65, 0x1a547ddd, 0x02f04ddd, 0x5eb09746, 0x636bc4c4, 0xbd282fb1, 0x1c452897, 0x2bd71de4};

int table_str2num[65536];
bool init = false;
void init_table()
{
if (!init)
{
	for(int i=0; i<65536; i++)
		table_str2num[i] = -1;

	for(int i=L'0'; i<=L'9'; i++)
		table_str2num[i] = i - L'0';

	for(int i=L'A'; i<=L'F'; i++)
		table_str2num[i] = i - L'A' + 0xA;
	init = true;
}
}

HRESULT RSA_dwindow_private(void *input, void *output)
{
	DWORD out[32];
	RSA(out, (DWORD*)input, (DWORD*)dwindow_d, (DWORD*)dwindow_n, 32);
	memcpy(output, out, 128);
	return S_OK;
}


HRESULT RSA_dwindow_network_private(void *input, void *output)
{
	DWORD out[32];
	RSA(out, (DWORD*)input, (DWORD*)dwindow_network_d, (DWORD*)dwindow_network_n, 32);
	memcpy(output, out, 128);
	return S_OK;
}

HRESULT bstr2binary(BSTR in, void * out, int outsize)
{
	init_table();

	int len = wcslen(in);
	if (len != outsize * 2)
		return E_FAIL;

	unsigned char * pout = (unsigned char*)out;

	for(int i=0; i<outsize; i++)
	{
		if (table_str2num[in[i*2]] < 0 || table_str2num[in[i*2+1]] < 0)
			return E_FAIL;

		pout[i] = (table_str2num[in[i*2]] << 4) + table_str2num[in[i*2+1]];
	}

	return S_OK;
}
HRESULT binary2bstr(void * input, int insize, BSTR *out)
{
	if (insize <= 0 )
		return E_INVALIDARG;

	unsigned char * pin = (unsigned char *)input;
	wchar_t *pout = new wchar_t[insize*2+1];
	pout[0] = NULL;
	wchar_t tmp[3];
	for(int i=0; i<insize; i++)
	{
		wsprintfW(tmp, L"%02X", pin[i]);
		wcscat(pout, tmp);
	}

	*out = SysAllocString(pout);
	delete [] pout;
	return S_OK;
}

HRESULT decode_client_request(BSTR input, dwindow_message_uncrypt *output)
{
	DWORD RSA_data[32];
	if (FAILED(bstr2binary(input, RSA_data, 128)))
		return E_FAIL;

	RSA_dwindow_network_private(RSA_data, output);

	return S_OK;
}

HRESULT generate_passkey_big(const unsigned char * passkey, __time64_t time_start, __time64_t time_end, int max_bar_user, dwindow_passkey_big *out, int theater_version = 0)
{
	memcpy(out->passkey, passkey, 32);
	memcpy(out->passkey2, passkey, 32);
	memcpy(out->ps_aes_key, ps_aes_key, 32);
	out->time_start = time_start;
	out->time_end = time_end;
	out->max_bar_user = max_bar_user;
	out->theater_version = theater_version;
	memset(out->reserved, 0, sizeof(out->reserved));
	out->zero = 0;

	RSA_dwindow_private(out, out);

	return S_OK;
}

// class 
STDMETHODIMP Ccrypt::test(BSTR* ret)
{
	return E_NOTIMPL;
}

STDMETHODIMP Ccrypt::test2(BSTR* rtn)
{
	return E_NOTIMPL;
}


STDMETHODIMP Ccrypt::get_passkey(BSTR input, BSTR* ret)
{
	return E_NOTIMPL;
}

STDMETHODIMP Ccrypt::get_hash(BSTR input, BSTR* rtn)
{
	return E_NOTIMPL;
}

STDMETHODIMP Ccrypt::get_key(BSTR input, BSTR* ret)
{
	return E_NOTIMPL;
}

STDMETHODIMP Ccrypt::decode_message(BSTR input, BSTR* ret)
{
	dwindow_message_uncrypt message;
	HRESULT hr = decode_client_request(input, &message);
	if (FAILED(hr))
	{
		*ret = 		SysAllocString(L"ERROR:INVALID ARGUMENT");
		return S_OK;
	}

	BSTR passkey, requested_hash, aes_key, password;
	//message.password_uncrypted[19] = 0;
	binary2bstr(message.passkey, 32, &passkey);
	binary2bstr(message.requested_hash, 20, &requested_hash);
	binary2bstr(message.random_AES_key, 32, &aes_key);
	binary2bstr(message.password_uncrypted, 20, &password);

	wchar_t out[64+40+64+40+20+3+1] = L"";
	wsprintfW(out, L"%s,%s,%s,%s,%d", passkey, requested_hash, aes_key, password, (unsigned int)message.client_time);

	SysFreeString(passkey);
	SysFreeString(requested_hash);
	SysFreeString(aes_key);
	SysFreeString(password);

	*ret = SysAllocString(out);
	return S_OK;
}

STDMETHODIMP Ccrypt::AES(BSTR data, BSTR key, BSTR* ret)
{
	int size =  wcslen(data)/2;
	int block_count = size/16 + ((size%16) ? 1 : 0);
	unsigned char *bin_data = new unsigned char[block_count*16];
	unsigned char bin_key[32];

	if (FAILED(bstr2binary(data, bin_data, size)))
		goto onfail;

	if (FAILED(bstr2binary(key, bin_key, 32)))
		goto onfail;

	AESCryptor aes;
	aes.set_key(bin_key, 256);
	for(int i=0; i<block_count; i++)
		aes.encrypt(bin_data+16*i, bin_data+16*i);

	HRESULT hr = binary2bstr(bin_data, block_count*16, ret);
	delete[] bin_data;
	return hr;

onfail:
	*ret = SysAllocString(L"");
	return S_OK;
}

STDMETHODIMP Ccrypt::gen_key(void)
{
	// TODO: 在此添加实现代码

	return S_OK;
}

STDMETHODIMP Ccrypt::gen_keys(BSTR passkey, DATE time_start, DATE time_end, BSTR* out)
{
	unsigned char pk[32];
	if (FAILED(bstr2binary(passkey, pk, 32)))
	{
		*out = SysAllocString(L"");
		return S_OK;
	}
	__time64_t start = (__time64_t)(time_start*3600*24);
	__time64_t end = (__time64_t)(time_end*3600*24);

	dwindow_passkey_big passkey_big;

	generate_passkey_big(pk, start, end, 1, &passkey_big);
	return binary2bstr(&passkey_big, 128, out);
}

STDMETHODIMP Ccrypt::gen_keys_int(BSTR passkey, ULONG time_start, ULONG time_end, BSTR* out)
{
	unsigned char pk[32];
	if (FAILED(bstr2binary(passkey, pk, 32)))
	{
		*out = SysAllocString(L"");
		return S_OK;
	}
	__time64_t start = time_start;
	__time64_t end = time_end;

	dwindow_passkey_big passkey_big;

	generate_passkey_big(pk, start, end, 1, &passkey_big);
	return binary2bstr(&passkey_big, 128, out);

	return S_OK;
}

STDMETHODIMP Ccrypt::genkeys(BSTR passkey, LONG time_start, LONG time_end, BSTR* out)
{
	unsigned char pk[32];
	if (FAILED(bstr2binary(passkey, pk, 32)))
	{
		*out = SysAllocString(L"");
		return S_OK;
	}
	__time64_t start = time_start;
	__time64_t end = time_end;

	dwindow_passkey_big passkey_big;

	generate_passkey_big(pk, start, end, 1, &passkey_big);
	return binary2bstr(&passkey_big, 128, out);

	return S_OK;
}

STDMETHODIMP Ccrypt::decode_binarystring(BSTR in, BSTR* out)
{
	int len = wcslen(in);
	if (len%2)
		return E_FAIL;

	char *tmp = new char[len/2+1];
	if (FAILED(bstr2binary(in, tmp, len/2)))
	{
		delete [] tmp;
		*out = SysAllocString(L"");
		return S_OK;
	}

	tmp[len/2] = NULL;
	USES_CONVERSION;
	*out = SysAllocString(A2W(tmp));
	delete [] tmp;

	return S_OK;
}

STDMETHODIMP Ccrypt::SHA1(BSTR in, BSTR* out)
{
	USES_CONVERSION;
	unsigned char sha1_result[20];
	SHA1Hash(sha1_result, (unsigned char*)W2A(in), wcslen(in));
	return binary2bstr(sha1_result, 20, out);
}

STDMETHODIMP Ccrypt::genkeys2(BSTR passkey, LONG time_start, LONG time_end, LONG max_bar_user, BSTR* out)
{
	unsigned char pk[32];
	if (FAILED(bstr2binary(passkey, pk, 32)))
	{
		*out = SysAllocString(L"");
		return S_OK;
	}
	__time64_t start = time_start;
	__time64_t end = time_end;

	dwindow_passkey_big passkey_big;

	generate_passkey_big(pk, start, end, max_bar_user, &passkey_big);
	return binary2bstr(&passkey_big, 128, out);

	return S_OK;
}

STDMETHODIMP Ccrypt::genkey3(BSTR passkey, LONG time_start, BSTR time_end, LONG max_bar_user, BSTR* out)
{
	return S_OK;
}

STDMETHODIMP Ccrypt::genkey4(BSTR passkey, LONG time_start, LONG time_end, LONG max_bar_user, LONG theater_version, BSTR* out)
{
	unsigned char pk[32];
	if (FAILED(bstr2binary(passkey, pk, 32)))
	{
		*out = SysAllocString(L"");
		return S_OK;
	}
	__time64_t start = time_start;
	__time64_t end = time_end;

	dwindow_passkey_big passkey_big;

	generate_passkey_big(pk, start, end, max_bar_user, &passkey_big, theater_version);
	return binary2bstr(&passkey_big, 128, out);

	return S_OK;
}
