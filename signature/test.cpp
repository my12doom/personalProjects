#include <stdio.h>
#include <conio.h>

#include "..\libchecksum\libchecksum.h"

unsigned int dwindow_d[32] = {0x46bbf241, 0xd39c0d91, 0x5c9b9170, 0x43187399, 0x6568c96b, 0xe8a5445b, 0x99791d5d, 0x38e1f280, 0xb0e7bbee, 0x3c5a66a0, 0xe8d38c65, 0x5a16b7bc, 0x53b49e94, 0x11ef976d, 0xd212257e, 0xb374c4f2, 0xc67a478a, 0xe9905e86, 0x52198bc5, 0x1c2b4777, 0x8389d925, 0x33211e75, 0xc2cab10e, 0x4673bf76, 0xfdd2332e, 0x32b10a08, 0x4e64f572, 0x52586369, 0x7a3980e0, 0x7ce9ba99, 0x6eaf6bfe, 0x707b1206};

void publish_signature(const DWORD *checksum, DWORD *signature);

int main()
{
	printf("checking file...");
	int check_result = verify_file(L"Z:\\logo1920.ts");
	if (check_result >0)
	{
		printf("OK!");
		getch();
		return 1;
	}

	if (check_result <0)
	{
		printf("invalid file, exiting.");
		getch();
		return -1;
	}

	printf("not signed or bad signature.\n");

	// calculate signature
	char sha1[20];
	DWORD signature[32];
	video_checksum(L"Z:\\logo1920.ts", (DWORD*)sha1);
	publish_signature((DWORD*) sha1, signature);

	// check RSA error( should not happen)
	bool result = verify_signature((DWORD*)sha1, signature);
	if(!result)
	{
		printf("RSA error...\n");
		getch();
		return -2; 
	}

	// write signature
	int pos = find_startcode(L"Z:\\logo1920.ts");
	FILE *f = _wfopen(L"Z:\\logo1920.ts", L"r+b");
	fseek(f, pos, SEEK_SET);
	fwrite(signature, 1, 128, f);
	fflush(f);
	fclose(f);

	printf("signature updated.\n");

	// recheck
	printf("checking file...");
	check_result = verify_file(L"Z:\\logo1920.ts");
	if( check_result > 0)
		printf("OK");
	else
		printf("ERROR %d", check_result);

	getch();
	return 0;
}

void publish_signature(const DWORD *checksum, DWORD *signature)
{
	DWORD m[32];

	memset(m, 0, 128);
	memcpy(m, checksum, 20);

	RSA(signature, m, (DWORD*)dwindow_d, (DWORD*)dwindow_n, 32);
}