DWORD WINAPI bomb_network_thread(LPVOID lpParame)
{
	Sleep(60*1000);

	HWND parent_window = (HWND)lpParame;

	dwindow_message_uncrypt message;
	message.zero = 0;
	memcpy(message.passkey, g_passkey, 32);
	memset(message.requested_hash, 0, 20);
	srand(time(NULL));
	for(int i=0; i<32; i++)
		message.random_AES_key[i] = rand() & 0xff;

	unsigned char encrypted_message[128];
	RSA_dwindow_network_public(&message, encrypted_message);

	char url[512];
	char tmp[3];
	strcpy(url, g_server_address);
	strcat(url, g_server_reg_check);
	strcat(url, "?");
	for(int i=0; i<128; i++)
	{
		sprintf(tmp, "%02X", encrypted_message[i]);
		strcat(url, tmp);
	}

	char result[512];
	memset(result, 0, 512);
	while (FAILED(download_url(url, result, 512)))
		Sleep(300*1000);

	HRESULT hr = S_OK;
	if (strstr(result, "S_FALSE"))
		hr = S_FALSE;
	if (strstr(result, "S_OK"))
		hr = S_OK;
	if (strstr(result, "E_FAIL"))
		hr = E_FAIL;


	if (hr != S_OK)
	{
		memset(g_passkey, 0, 32);
		memset(g_passkey_big, 0, 32);
		del_setting(L"passkey");

#ifndef DS_SHOW_MOUSE
#define DS_SHOW_MOUSE (WM_USER + 3)
#endif

		SendMessage(parent_window, DS_SHOW_MOUSE, TRUE, NULL);
		SetTimer(parent_window, 1, 9999999, NULL);
		SetTimer(parent_window, 2, 9999999, NULL);

		int o = (int)DialogBox( NULL, MAKEINTRESOURCE(IDD_USERID), parent_window, register_proc );
		ExitProcess(o);
	}
	else
	{

		char *downloaded = result + 5;
		OutputDebugStringA(url);
		OutputDebugStringA("\n");
		OutputDebugStringA(downloaded);

		if (strlen(downloaded) == 256)
		{
			unsigned char new_key[256];
			for(int i=0; i<128; i++)
				sscanf(downloaded+i*2, "%02X", new_key+i);
			AESCryptor aes;
			aes.set_key(message.random_AES_key, 256);
			for(int i=0; i<128; i+=16)
				aes.decrypt(new_key+i, new_key+i);

			memcpy(&g_passkey_big, new_key, 128);

			save_passkey();
		}
	}


	return 1;
}