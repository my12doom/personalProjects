{
	DWORD e[32];	

	DWORD m1[32]={0,1,2,3,4,5,6,7,8,9,10};
	BigNumberSetEqualdw(e, 65537, 32);
	RSA(m1, (DWORD*)g_passkey_big, e, (DWORD*)dwindow_n, 32);
	__time64_t *time_start = (__time64_t *)(m1+24);
	__time64_t *time_end = (__time64_t*)(m1+26);	
	for(int i=0; i<8; i++)
		if (m1[i] != m1[i+8] || *time_start > mytime() || mytime() > *time_end)
		{
			del_setting(L"passkey");
			TerminateProcess(GetCurrentProcess(), 0);
		}
	memcpy(g_passkey, m1, 32);
	memset(m1,0,128);
}
