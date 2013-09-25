// active_dwindowDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "active_dwindow.h"
#include "active_dwindowDlg.h"

#include <intrin.h>
#include "..\libchecksum\libchecksum.h"
#include "..\AESFile\rijndael.h"
#include "..\renderer_prototype\ps_aes_key.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Cactive_dwindowDlg 对话框




Cactive_dwindowDlg::Cactive_dwindowDlg(CWnd* pParent /*=NULL*/)
	: CDialog(Cactive_dwindowDlg::IDD, pParent)
	, m_password(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Cactive_dwindowDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_password);
}

BEGIN_MESSAGE_MAP(Cactive_dwindowDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON1, &Cactive_dwindowDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &Cactive_dwindowDlg::OnBnClickedButton2)
END_MESSAGE_MAP()


// Cactive_dwindowDlg 消息处理程序

BOOL Cactive_dwindowDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void Cactive_dwindowDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}



//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR Cactive_dwindowDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

unsigned int dwindow_n[32] = {0x5cc5db57, 0x881651da, 0x7981477c, 0xa65785e0, 0x5fe54c72, 0x5247aeff, 0xd2b635f7, 0x24781e90, 0x3d4e298e, 0x92f1bb41, 0x90494ef2, 0x2f0f6eeb, 0xb151c299, 0x6641acf9, 0x8a0681de, 0x9dd53c21, 0x623631ca, 0x906c24ff, 0x14980fce, 0xbbd5419f, 0x1ef0366c, 0xb759416a, 0x1a214b0f, 0x070c6972, 0x382d1969, 0xb9a02637, 0xeadffc31, 0xcd13093a, 0xfd273caf, 0xd2140f85, 0x2800ba50, 0x877d04f0};
unsigned int dwindow_d[32] = {0x3ccd1f98, 0x69c8d2e1, 0xbbff8870, 0x9b6110f3, 0x1a76aa3b, 0xb581463b, 0x56ee1605, 0xb611eb8e, 0xef3239a5, 0xf87e0ed3, 0x3e210bb3, 0xd75b28a5, 0x3cca9966, 0x343027e4, 0xe0de438a, 0x7acf57d8, 0xdf0a7157, 0x188c1b58, 0x9e8317aa, 0xddbdfd5e, 0xb81892a8, 0x11854a75, 0x2c58aca7, 0x7fe7127f, 0xd6fdc4d8, 0xcecde17b, 0x9c24eebc, 0x23cafa43, 0x3dedb2a, 0x5a61cab5, 0xb947f13a, 0x7b01e918};
HRESULT RSA_dwindow_private(void *input, void *output)
{
	DWORD out[32];
	RSA(out, (DWORD*)input, (DWORD*)dwindow_d, (DWORD*)dwindow_n, 32);
	memcpy(output, out, 128);
	return S_OK;
}

HRESULT generate_passkey_big(const unsigned char * passkey, __time64_t time_start, __time64_t time_end, dwindow_passkey_big *out)
{
	memcpy(out->passkey, passkey, 32);
	memcpy(out->passkey2, passkey, 32);
	memcpy(out->ps_aes_key, ps_aes_key, 32);
	out->time_start = time_start;
	out->time_end = time_end;
	memset(out->reserved, 0, sizeof(out->reserved));
	out->zero = 0;
	out->usertype = 0;

	RSA_dwindow_private(out, out);

	return S_OK;
}

const WCHAR* soft_key= L"Software\\DWindow";
bool save_setting(const WCHAR *key, const void *data, int len, DWORD REG_TYPE = REG_BINARY )
{
	HKEY hkey = NULL;
	int ret = RegCreateKeyExW(HKEY_CURRENT_USER, soft_key, 0,0,REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WRITE |KEY_SET_VALUE, NULL , &hkey, NULL  );
	if (ret != ERROR_SUCCESS)
		return false;
	ret = RegSetValueExW(hkey, key, 0, REG_TYPE, (const byte*)data, len );
	if (ret != ERROR_SUCCESS)
		return false;

	RegCloseKey(hkey);
	return true;
}

void Cactive_dwindowDlg::OnBnClickedButton1()
{
	UpdateData();
	char pass[32];
	memset(pass, 0, 32);
	USES_CONVERSION;
	strcpy(pass, T2A(m_password));
	AESCryptor aes;
	aes.set_key((unsigned char*)pass, 256);

	for(int i=0; i<128; i+=16)
		aes.decrypt(((unsigned char*)dwindow_d)+i, ((unsigned char*)dwindow_d)+i);

	unsigned char passkey_big[128];
	generate_passkey_big(passkey_big, _time64(NULL), 0xffffffff, (dwindow_passkey_big*)passkey_big);

	for(int i=0; i<32; i++)
		TRACE("0x%x, ", dwindow_d[i]);

	for(int i=0; i<128; i+=16)
		aes.encrypt(((unsigned char*)dwindow_d)+i, ((unsigned char*)dwindow_d)+i);


	// old
	int CPUInfo[4];
	unsigned char CPUBrandString[48];
	memset(CPUBrandString, 0, 48);
	__cpuid(CPUInfo, 0x80000002);
	memcpy(CPUBrandString, CPUInfo, 16);
	__cpuid(CPUInfo, 0x80000003);
	memcpy(CPUBrandString+16, CPUInfo, 16);
	__cpuid(CPUInfo, 0x80000004);
	memcpy(CPUBrandString+32, CPUInfo, 16);
	for(int i=0; i<16; i++)
		CPUBrandString[i] ^= CPUBrandString[i+32];

	DWORD volume_c_sn;
	wchar_t volume_name[MAX_PATH];
	GetVolumeInformationW(L"D:\\", volume_name, MAX_PATH, &volume_c_sn, NULL, NULL, NULL, NULL);
	((DWORD*)CPUBrandString)[4] ^= volume_c_sn;

	aes.set_key(CPUBrandString, 256);
	for(int i=0; i<128; i+=16)
		aes.encrypt((unsigned char*)passkey_big+i, (unsigned char*)passkey_big+i);

	save_setting(L"passkey", passkey_big, 128);
}

void Cactive_dwindowDlg::OnBnClickedButton2()
{
	// new
	UpdateData();
	char pass[32];
	memset(pass, 0, 32);
	USES_CONVERSION;
	strcpy(pass, T2A(m_password));
	AESCryptor aes;
	aes.set_key((unsigned char*)pass, 256);

	for(int i=0; i<128; i+=16)
		aes.decrypt(((unsigned char*)dwindow_d)+i, ((unsigned char*)dwindow_d)+i);

	unsigned char passkey_big[256];
	generate_passkey_big(passkey_big, _time64(NULL), 0xffffffff, (dwindow_passkey_big*)passkey_big);

	for(int i=0; i<128; i+=16)
		aes.encrypt(((unsigned char*)dwindow_d)+i, ((unsigned char*)dwindow_d)+i);


	int CPUInfo[4];
	unsigned char CPUBrandString[48];
	memset(CPUBrandString, 0, 48);
	__cpuid(CPUInfo, 0x80000002);
	memcpy(CPUBrandString, CPUInfo, 16);
	__cpuid(CPUInfo, 0x80000003);
	memcpy(CPUBrandString+16, CPUInfo, 16);
	__cpuid(CPUInfo, 0x80000004);
	memcpy(CPUBrandString+32, CPUInfo, 16);
	memset(CPUBrandString + strlen((char*)CPUBrandString), 0, 48-strlen((char*)CPUBrandString));
	for(int i=0; i<16; i++)
		CPUBrandString[i] ^= CPUBrandString[i+32];


	DWORD volume_c_sn = 0;
	wchar_t volume_name[MAX_PATH];
	GetVolumeInformationW(L"C:\\", volume_name, MAX_PATH, &volume_c_sn, NULL, NULL, NULL, NULL);
	((DWORD*)CPUBrandString)[4] ^= volume_c_sn;

	aes.set_key(CPUBrandString, 256);
	for(int i=0; i<128; i+=16)
		aes.encrypt((unsigned char*)passkey_big+i, (unsigned char*)passkey_big+i);

	save_setting(L"passkey", passkey_big, 128);
}
