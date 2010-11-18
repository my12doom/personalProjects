// remux_publishDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "remux_publish.h"
#include "remux_publishDlg.h"
#include ".\remux_publishdlg.h"

#include "..\libchecksum\libchecksum.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Cremux_publishDlg 对话框



Cremux_publishDlg::Cremux_publishDlg(CWnd* pParent /*=NULL*/)
	: CDialog(Cremux_publishDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Cremux_publishDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(Cremux_publishDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_WM_DROPFILES()
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedButton2)
END_MESSAGE_MAP()


// Cremux_publishDlg 消息处理程序

BOOL Cremux_publishDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	
	return TRUE;  // 除非设置了控件的焦点，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void Cremux_publishDlg::OnPaint() 
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
HCURSOR Cremux_publishDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void Cremux_publishDlg::OnDropFiles(HDROP hDropInfo)
{


	int cFiles = DragQueryFileW(hDropInfo, (UINT)-1, NULL, 0); 

	{
		DragQueryFileW(hDropInfo, 0, pathname, sizeof(pathname));
		check_file();
	}

	CDialog::OnDropFiles(hDropInfo);
}

void Cremux_publishDlg::check_file()
{
	check_result = ::verify_file(pathname);

	if (check_result == 0)
	{
		SetDlgItemText(IDC_STATIC, _T("State: new file"));
		::EnableWindow(GetDlgItem(IDC_BUTTON2)->m_hWnd, TRUE);
	}
	else if (check_result > 1)
	{
		SetDlgItemText(IDC_STATIC, _T("State: signature OK"));
		::EnableWindow(GetDlgItem(IDC_BUTTON2)->m_hWnd, FALSE);
	}
	else if (check_result == 1)
	{
		SetDlgItemText(IDC_STATIC, _T("State: corrupted file"));
		::EnableWindow(GetDlgItem(IDC_BUTTON2)->m_hWnd, TRUE);
	}
	else if (check_result <0)
	{
		SetDlgItemText(IDC_STATIC, _T("State: invalid file"));
		::EnableWindow(GetDlgItem(IDC_BUTTON2)->m_hWnd, FALSE);
	}

}


void Cremux_publishDlg::publish_signature(const DWORD *checksum, DWORD *signature)
{
	unsigned int dwindow_d[32] = {0x46bbf241, 0xd39c0d91, 0x5c9b9170, 0x43187399, 0x6568c96b, 0xe8a5445b, 0x99791d5d, 0x38e1f280, 0xb0e7bbee, 0x3c5a66a0, 0xe8d38c65, 0x5a16b7bc, 0x53b49e94, 0x11ef976d, 0xd212257e, 0xb374c4f2, 0xc67a478a, 0xe9905e86, 0x52198bc5, 0x1c2b4777, 0x8389d925, 0x33211e75, 0xc2cab10e, 0x4673bf76, 0xfdd2332e, 0x32b10a08, 0x4e64f572, 0x52586369, 0x7a3980e0, 0x7ce9ba99, 0x6eaf6bfe, 0x707b1206};
	DWORD m[32];

	memset(m, 0, 128);
	memcpy(m, checksum, 20);

	RSA(signature, m, (DWORD*)dwindow_d, (DWORD*)dwindow_n, 32);
}
void Cremux_publishDlg::OnBnClickedButton2()
{
	// calculate signature
	char sha1[20];
	DWORD signature[32];
	video_checksum(pathname, (DWORD*)sha1);
	publish_signature((DWORD*) sha1, signature);

	// check RSA error( should not happen)
	bool result = verify_signature((DWORD*)sha1, signature);
	if(!result)
	{
		printf("RSA error...\n");
		return; 
	}

	// write signature
	int pos = find_startcode(pathname);
	FILE *f = _wfopen(pathname, L"r+b");
	fseek(f, pos, SEEK_SET);
	fwrite(signature, 1, 128, f);
	fflush(f);
	fclose(f);

	check_file();
}
