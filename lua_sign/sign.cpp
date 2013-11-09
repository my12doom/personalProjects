#include <stdio.h>
#include "..\libchecksum\basicRSA.h"
#include "..\libchecksum\libchecksum.h"
#include "..\libchecksum\SHA1.h"

DWORD dwindow_network_d[32] = {0xf70b4b91, 0x4ea5db4f, 0xa3c3e23f, 0xb6e80229, 0xfed913d3, 0x19269824, 0x41b29509, 0xa652e1b9, 0x1e1f5d0e, 0x660b9683, 0xb0fe5d71, 0x36cc1f6b, 0x9f6ed8ec, 0xfbc2f93a, 0xadce8a8c, 0x75db6f8b, 0x7aa72c0f, 0x7b6dc001, 0x091aab56, 0xe2b07d17, 0x76fae5cf, 0x10b80462, 0xf401ea79, 0x575dd549, 0x3fbd6d65, 0x1a547ddd, 0x02f04ddd, 0x5eb09746, 0x636bc4c4, 0xbd282fb1, 0x1c452897, 0x2bd71de4};

#pragma pack(push, 1)
typedef struct
{
	char leading[17];// = "local signature=\"";
	char signature[256];
	char ending[3];// = "\"";
} lua_signature;
#pragma pack(pop)


HRESULT RSA_dwindow_network_private(void *input, void *output)
{
	DWORD out[32];
	RSA(out, (DWORD*)input, (DWORD*)dwindow_network_d, (DWORD*)dwindow_network_n, 32);
	memcpy(output, out, 128);
	return S_OK;
}

int main()
{
	int argc = 1;
	LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);


	FILE *input = _wfopen(argv[1], L"rb");
	if (!input)
	{
		printf("open file failed\n");
		return -1;
	}

	// check for existing signature
	lua_signature sig_txt;
	int file_size;
	unsigned char * data = NULL;
	if (fread(&sig_txt, 1, sizeof(sig_txt), input) != sizeof(sig_txt) 
		|| memcmp(sig_txt.leading, "\xef\xbb\xbf-- signature=\"", sizeof(sig_txt.leading))
		|| memcmp(sig_txt.ending, "\"\r\n", sizeof(sig_txt.ending)))
	{
		// no signature
		fseek(input, 0, SEEK_END);
		file_size = ftell(input)-3;	// skip UTF8 BOM
		data = new unsigned char[file_size];
		fseek(input, 3, SEEK_SET);
		fread(data, 1, file_size, input);
	}
	else
	{
		// existing signature, remove it
		fseek(input, 0, SEEK_END);
		file_size = ftell(input) - sizeof(sig_txt);
		data = new unsigned char[file_size];
		fseek(input, sizeof(sig_txt), SEEK_SET);
		fread(data, 1, file_size, input);
	}


	fclose(input);

	unsigned char signature[128] = {0};
	SHA1Hash(signature, data, file_size);
	RSA_dwindow_network_private(signature, signature);

	FILE * f = _wfopen(argv[1], L"wb");
	lua_signature lua = {};
	memcpy(lua.leading, "\xef\xbb\xbf-- signature=\"", 17);
	for(int i=0; i<128; i++)
		sprintf(lua.signature+2*i, "%02X", signature[i]);
	memcpy(lua.ending, "\"\r\n", 3);
	fwrite(&lua, 1, sizeof(lua), f);

	fwrite(data, 1, file_size, f);
	fclose(f);
	delete data;

	return 0;
}
