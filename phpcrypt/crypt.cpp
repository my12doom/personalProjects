// crypt.cpp : Ccrypt µÄÊµÏÖ

#include "stdafx.h"
#include "crypt.h"
#include "..\libchecksum\libchecksum.h"
#include "..\AESFile\rijndael.h"

// Ccrypt

unsigned int dwindow_d[32] = {0x46bbf241, 0xd39c0d91, 0x5c9b9170, 0x43187399, 0x6568c96b, 0xe8a5445b, 0x99791d5d, 0x38e1f280, 0xb0e7bbee, 0x3c5a66a0, 0xe8d38c65, 0x5a16b7bc, 0x53b49e94, 0x11ef976d, 0xd212257e, 0xb374c4f2, 0xc67a478a, 0xe9905e86, 0x52198bc5, 0x1c2b4777, 0x8389d925, 0x33211e75, 0xc2cab10e, 0x4673bf76, 0xfdd2332e, 0x32b10a08, 0x4e64f572, 0x52586369, 0x7a3980e0, 0x7ce9ba99, 0x6eaf6bfe, 0x707b1206};
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


HRESULT Decode_data(BSTR input, dwindow_message_uncrypt *output)
{
	init_table();

	int len = wcslen(input);
	if (len != 256)
	{
		return E_FAIL;
	}

	unsigned char RSA_data[128];
	for(int i=0; i<128; i++)
	{
		if (table_str2num[input[i*2]] < 0 || table_str2num[input[i*2+1]] < 0)
			return E_FAIL;

		RSA_data[i] = (table_str2num[input[i*2]] << 4) + table_str2num[input[i*2+1]];
	}

	RSA_dwindow_private(RSA_data, output);

	return S_OK;
}

STDMETHODIMP Ccrypt::test(BSTR* ret)
{
	BSTR temp = ::SysAllocString(L"PHP Hello Dll 071226");
	* ret=temp;

	return S_OK;
}

STDMETHODIMP Ccrypt::test2(BSTR* rtn)
{
	BSTR temp = ::SysAllocString(L"PHP Hello Dll 071226");
	* rtn = temp;

	return S_OK;
}


STDMETHODIMP Ccrypt::get_passkey(BSTR input, BSTR* ret)
{
	dwindow_message_uncrypt message;
	HRESULT hr = Decode_data(input, &message);
	if (FAILED(hr))
	{
		BSTR temp = SysAllocString(L"ERROR:INVALID ARGUMENT");
		*ret = temp;
		return S_OK;
	}

	wchar_t out[65] = L"";
	wchar_t tmp[3];
	for(int i=0; i<32; i++)
	{
		wsprintfW(tmp, L"%02X", message.passkey[i]);
		wcscat(out, tmp);
	}
	BSTR temp = SysAllocString(out);
	*ret = temp;
	return S_OK;
}

STDMETHODIMP Ccrypt::get_hash(BSTR input, BSTR* rtn)
{
	dwindow_message_uncrypt message;
	HRESULT hr = Decode_data(input, &message);
	if (FAILED(hr))
	{
		BSTR temp = SysAllocString(L"ERROR:INVALID ARGUMENT");
		*rtn = temp;
		return S_OK;
	}

	wchar_t out[41] = L"";
	wchar_t tmp[3];
	for(int i=0; i<20; i++)
	{
		wsprintfW(tmp, L"%02X", message.requested_hash[i]);
		wcscat(out, tmp);
	}
	BSTR temp = SysAllocString(out);
	*rtn = temp;
	return S_OK;
}

STDMETHODIMP Ccrypt::get_key(BSTR input, BSTR* ret)
{
	dwindow_message_uncrypt message;
	HRESULT hr = Decode_data(input, &message);
	if (FAILED(hr))
	{
		BSTR temp = SysAllocString(L"ERROR:INVALID ARGUMENT");
		*ret = temp;
		return S_OK;
	}

	wchar_t out[65] = L"";
	wchar_t tmp[3];
	for(int i=0; i<32; i++)
	{
		wsprintfW(tmp, L"%02X", message.random_AES_key[i]);
		wcscat(out, tmp);
	}
	BSTR temp = SysAllocString(out);
	*ret = temp;
	return S_OK;
}

STDMETHODIMP Ccrypt::decode_message(BSTR input, BSTR* ret)
{
	dwindow_message_uncrypt message;
	HRESULT hr = Decode_data(input, &message);
	if (FAILED(hr))
	{
		BSTR temp = SysAllocString(L"ERROR:INVALID ARGUMENT");
		*ret = temp;
		return S_OK;
	}

	wchar_t out[64+40+64+2+1] = L"";
	wchar_t tmp[3];
	for(int i=0; i<32; i++)
	{
		wsprintfW(tmp, L"%02X", message.passkey[i]);
		wcscat(out, tmp);
	}
	wcscat(out, L",");
	for(int i=0; i<20; i++)
	{
		wsprintfW(tmp, L"%02X", message.requested_hash[i]);
		wcscat(out, tmp);
	}
	wcscat(out, L",");
	for(int i=0; i<32; i++)
	{
		wsprintfW(tmp, L"%02X", message.random_AES_key[i]);
		wcscat(out, tmp);
	}
	BSTR temp = SysAllocString(out);
	*ret = temp;
	return S_OK;
}

STDMETHODIMP Ccrypt::AES(BSTR data, BSTR key, BSTR* ret)
{
	init_table();
	int data_len = wcslen(data);
	int key_len = wcslen(key);
	if (data_len != 64 || key_len != 64)
		goto onfail;

	unsigned char bin_data[32];
	unsigned char encrypt_data[32];
	unsigned char bin_key[32];

	for(int i=0; i<32; i++)
	{
		if (table_str2num[data[i*2]] < 0 || table_str2num[data[i*2+1]] < 0)
			goto onfail;
		if (table_str2num[key[i*2]] < 0 || table_str2num[key[i*2+1]] < 0)
			goto onfail;

		bin_data[i] = (table_str2num[data[i*2]] << 4) + table_str2num[data[i*2+1]];
		bin_key[i] = (table_str2num[key[i*2]] << 4) + table_str2num[key[i*2+1]];
	}

	AESCryptor aes;
	aes.set_key(bin_key, 256);
	aes.encrypt(bin_data, encrypt_data);
	aes.encrypt(bin_data+16, encrypt_data+16);

	wchar_t out[65] = L"";
	wchar_t tmp[3];
	for(int i=0; i<32; i++)
	{
		wsprintfW(tmp, L"%02X", encrypt_data[i]);
		wcscat(out, tmp);
	}

	BSTR t = SysAllocString(out);
	*ret = t;

	return S_OK;

onfail:
	BSTR temp = SysAllocString(L"");
	*ret = temp;
	return S_OK;
}
