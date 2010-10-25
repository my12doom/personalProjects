// dwindow_launcherDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "shlwapi.h"
#pragma comment(lib, "shlwapi.lib")
#include <Tlhelp32.h>
#include "dwindow_launcher.h"
#include "dwindow_launcherDlg.h"
#include ".\dwindow_launcherdlg.h"

#include <atlbase.h>
#include <dshow.h>
#include <initguid.h>


// {F07E981B-0EC4-4665-A671-C24955D11A38}
DEFINE_GUID(CLSID_PD10_DEMUXER, 
                        0xF07E981B, 0x0EC4, 0x4665, 0xA6, 0x71, 0xC2, 0x49, 0x55, 0xD1, 0x1A, 0x38);

// {D00E73D7-06F5-44F9-8BE4-B7DB191E9E7E}
DEFINE_GUID(CLSID_PD10_DECODER, 
                        0xD00E73D7, 0x06f5, 0x44F9, 0x8B, 0xE4, 0xB7, 0xDB, 0x19, 0x1E, 0x9E, 0x7E);

// {419832C4-7813-4b90-A262-12496691E82E}
DEFINE_GUID(CLSID_DWindowSSP, 
						0x419832c4, 0x7813, 0x4b90, 0xa2, 0x62, 0x12, 0x49, 0x66, 0x91, 0xe8, 0x2e);

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Cdwindow_launcherDlg 对话框



Cdwindow_launcherDlg::Cdwindow_launcherDlg(CWnd* pParent /*=NULL*/)
	: CDialog(Cdwindow_launcherDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Cdwindow_launcherDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON2, m_regsvr_button);
}

BEGIN_MESSAGE_MAP(Cdwindow_launcherDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON4, OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON3, OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON5, OnBnClickedButton5)
END_MESSAGE_MAP()
// Cdwindow_launcherDlg 消息处理程序


bool Cdwindow_launcherDlg::check_module()
{
	// vista/win7 UAC shield icon
	const UINT BCM_SETSHIELD = 0x0000160C;
	
	// check module registration
	CoInitialize(NULL);
	CComPtr<IBaseFilter> ssp;
	CComPtr<IBaseFilter> demuxer;
	CComPtr<IBaseFilter> decoder;
	ssp.CoCreateInstance(CLSID_DWindowSSP);
	demuxer.CoCreateInstance(CLSID_PD10_DEMUXER);
	decoder.CoCreateInstance(CLSID_PD10_DECODER);

	if (ssp == NULL || demuxer == NULL || decoder == NULL)
	{
		// need registration
		m_regsvr_button.SendMessage(BCM_SETSHIELD, 0, TRUE);
		SetDlgItemText(IDC_BUTTON2, _T("注册组件"));
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_BUTTON2), TRUE);

		return false;
	}
	else
	{
		// no need to
		m_regsvr_button.SendMessage(BCM_SETSHIELD, 0, FALSE);
		SetDlgItemText(IDC_BUTTON2, _T("组件已注册"));
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_BUTTON2), FALSE);

		return true;
	}
}

BOOL Cdwindow_launcherDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	check_module();

	return TRUE;  // 除非设置了控件的焦点，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void Cdwindow_launcherDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作矩形中居中
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

//当用户拖动最小化窗口时系统调用此函数取得光标显示。
HCURSOR Cdwindow_launcherDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void Cdwindow_launcherDlg::OnBnClickedButton1()
{
	// exit
	OnOK();
}

void Cdwindow_launcherDlg::OnBnClickedButton4()
{
	if (MessageBox(_T("这将自动关闭SSP，是否继续？"), NULL, MB_YESNO | MB_ICONWARNING) == IDNO)
		return;

	kill_ssp();
	// ssp set remux
	const TCHAR* filters_key= _T("Software\\3dtv.at\\Stereoscopic Player\\Preferred Filters");

	// reset first
	int ret = SHDeleteKey( HKEY_CURRENT_USER, filters_key);

	// write keys
	HKEY hkey = NULL;
	ret = RegCreateKeyEx(HKEY_CURRENT_USER, filters_key, 0,0,REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WRITE  |KEY_SET_VALUE, NULL , &hkey, NULL  );

	// demuxer
	HKEY hdemuxerkey = NULL;
	ret = RegCreateKeyEx(hkey, _T("MPEG-2 Stream Splitter"), 0,0,REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WRITE  |KEY_SET_VALUE, NULL , &hdemuxerkey, NULL  );
	ret = RegSetValueEx(hdemuxerkey, _T("0"), 0, REG_SZ, (const BYTE*)_T("{F07E981B-0EC4-4665-A671-C24955D11A38}"), sizeof(_T("{F07E981B-0EC4-4665-A671-C24955D11A38}")) );
	RegCloseKey(hdemuxerkey);

	// decoder
	HKEY hdecoderkey = NULL;
	ret = RegCreateKeyEx(hkey, _T("MPEG-4 AVC Video Decoder"), 0,0,REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WRITE  |KEY_SET_VALUE, NULL , &hdecoderkey, NULL  );
	ret = RegSetValueEx(hdecoderkey, _T("0"), 0, REG_SZ, (const BYTE*)_T("{D00E73D7-06F5-44F9-8BE4-B7DB191E9E7E}"), sizeof(_T("{D00E73D7-06F5-44F9-8BE4-B7DB191E9E7E}")) );
	RegCloseKey(hdecoderkey);

	// video processor
	HKEY hvpkey = NULL;
	ret = RegCreateKeyEx(hkey, _T("Video Processor"), 0,0,REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WRITE  |KEY_SET_VALUE, NULL , &hvpkey, NULL  );
	ret = RegSetValueEx(hvpkey, _T("0"), 0, REG_SZ, (const BYTE*)_T("{419832C4-7813-4B90-A262-12496691E82E}"), sizeof(_T("{419832C4-7813-4B90-A262-12496691E82E}")) );
	ret = RegSetValueEx(hvpkey, _T("1"), 0, REG_SZ, (const BYTE*)_T("{DBF9000E-F08C-4858-B769-C914A0FBB1D7}"), sizeof(_T("{DBF9000E-F08C-4858-B769-C914A0FBB1D7}")) );
	RegCloseKey(hvpkey);

	RegCloseKey(hkey);

	//
	MessageBox(_T("配置成功!"), NULL, MB_OK | MB_ICONINFORMATION);
	return;

}

void Cdwindow_launcherDlg::OnBnClickedButton3()
{
	if (MessageBox(_T("这将自动关闭SSP，是否继续？"), NULL, MB_YESNO | MB_ICONWARNING) == IDNO)
		return;
	kill_ssp();
	// ssp reset

	const TCHAR* filters_key= _T("Software\\3dtv.at\\Stereoscopic Player\\Preferred Filters");

	int ret = SHDeleteKey( HKEY_CURRENT_USER, filters_key);

	MessageBox(_T("恢复成功!"), NULL, MB_OK | MB_ICONINFORMATION);

	return;
}

void Cdwindow_launcherDlg::OnBnClickedButton2()
{
	// regsvr
	SHELLEXECUTEINFO ShExecInfo = {0};
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpFile = _T("regsvr.exe");
	ShExecInfo.lpParameters = _T("");	
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_SHOW;
	ShExecInfo.hInstApp = NULL;	
	ShExecInfo.lpVerb = _T("open");

	if ((DWORD)(LOBYTE(LOWORD(GetVersion()))) >= 6)
		ShExecInfo.lpVerb = _T("runas");

	ShellExecuteEx(&ShExecInfo);
	WaitForSingleObject(ShExecInfo.hProcess,INFINITE);
	//todo: wait for regsvr
	check_module();
}

void Cdwindow_launcherDlg::OnBnClickedButton5()
{
	int ret = (int)ShellExecute(m_hWnd, _T("open"), _T("dwindow.exe"), 0, 0, SW_NORMAL);
	if (ret>32)
		OnOK();
	else
		MessageBox(_T("DWindow 运行失败"), NULL, MB_OK | MB_ICONERROR);
}

void Cdwindow_launcherDlg::kill_ssp()
{
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pe32; 
	pe32.dwSize = sizeof(PROCESSENTRY32);
	if (Process32First(hSnap, &pe32))
	{
		do
		{
			wchar_t tmp[MAX_PATH];
			memset(tmp, 0, sizeof(tmp));
			for(int i=0; i<wcslen(pe32.szExeFile); i++)
				tmp[i] = towlower(pe32.szExeFile[i]);

			if (!wcscmp(tmp, L"stereoplayer.exe"))
			{
				HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
				TerminateProcess(process, -1);
				CloseHandle(process);
			}
		} while (Process32Next(hSnap, &pe32));
	}
}
