// GUIDtoolDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "GUIDtool.h"
#include "GUIDtoolDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CGUIDtoolDlg 对话框




CGUIDtoolDlg::CGUIDtoolDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGUIDtoolDlg::IDD, pParent)
	, m_GUID(_T(""))
	, m_GUID_define(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CGUIDtoolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_GUID);
	DDX_Text(pDX, IDC_EDIT2, m_GUID_define);
}

BEGIN_MESSAGE_MAP(CGUIDtoolDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_EN_CHANGE(IDC_EDIT1, &CGUIDtoolDlg::OnEnChangeEdit1)
END_MESSAGE_MAP()


// CGUIDtoolDlg 消息处理程序

BOOL CGUIDtoolDlg::OnInitDialog()
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

void CGUIDtoolDlg::OnPaint()
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
HCURSOR CGUIDtoolDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CGUIDtoolDlg::OnEnChangeEdit1()
{
	UpdateData(TRUE);

	CLSID guid;
	CString guid_string = m_GUID;
	guid_string.Trim();

	if (SUCCEEDED(CLSIDFromString((LPOLESTR)(const TCHAR*)guid_string,&guid)))
	{
		m_GUID_define.Format(_T("DEFINE_GUID(CLSID_YOURNAME, 0x%08x, 0x%04x, 0x%04x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x)"),
			guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7], guid.Data4[8]);
	}
	else
	{
		m_GUID_define = _T("Invalid GUID string");
	}
	UpdateData(FALSE);
	
}
