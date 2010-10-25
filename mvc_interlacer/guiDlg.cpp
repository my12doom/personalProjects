// interlacer_guiDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "gui.h"
#include "guiDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Cinterlacer_guiDlg 对话框



Cinterlacer_guiDlg::Cinterlacer_guiDlg(CWnd* pParent /*=NULL*/)
	: CDialog(Cinterlacer_guiDlg::IDD, pParent)
	, m_input1(_T(""))
	, m_input2(_T(""))
	, m_output(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Cinterlacer_guiDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_input1);
	DDX_Text(pDX, IDC_EDIT2, m_input2);
	DDX_Text(pDX, IDC_EDIT3, m_output);
}

BEGIN_MESSAGE_MAP(Cinterlacer_guiDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
END_MESSAGE_MAP()


// Cinterlacer_guiDlg 消息处理程序

BOOL Cinterlacer_guiDlg::OnInitDialog()
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

void Cinterlacer_guiDlg::OnPaint() 
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
HCURSOR Cinterlacer_guiDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void Cinterlacer_guiDlg::OnBnClickedButton2()
{	
    TCHAR strFileName[MAX_PATH] = TEXT("");
    TCHAR strPath[MAX_PATH] = TEXT("");

    OPENFILENAME ofn = { sizeof(OPENFILENAME), this->GetSafeHwnd() , NULL,
                         TEXT(".ssif/.m2ts/.264/.h264\0*.ssif;*.m2ts;*.264;*.h264\0All Files\0*.*\0\0"), NULL,
                         0, 1, strFileName, MAX_PATH, NULL, 0, strPath,
                         TEXT("Open Input 1"),
                         OFN_FILEMUSTEXIST|OFN_HIDEREADONLY, 0, 0,
                         TEXT(".wav"), 0, NULL, NULL };

	if( TRUE == GetOpenFileName( &ofn ) )
    {
		m_input1 = strFileName;

		
		TCHAR out[MAX_PATH];
		_tcscpy(out, strFileName);
		
		for(int i=_tcslen(out); i>0; i--)
			if (out[i] == _T('.'))
				out[i] = NULL;

		_tcscat(out, _T(".mvc"));
		m_output = out;
    }
	UpdateData(FALSE);
}

void Cinterlacer_guiDlg::OnBnClickedButton3()
{
    TCHAR strFileName[MAX_PATH] = TEXT("");
    TCHAR strPath[MAX_PATH] = TEXT("");

    OPENFILENAME ofn = { sizeof(OPENFILENAME), this->GetSafeHwnd() , NULL,
                         TEXT(".ssif/.m2ts/.264/.h264\0*.ssif;*.m2ts;*.264;*.h264\0All Files\0*.*\0\0"), NULL,
                         0, 1, strFileName, MAX_PATH, NULL, 0, strPath,
                         TEXT("Open Input 1"),
                         OFN_FILEMUSTEXIST|OFN_HIDEREADONLY, 0, 0,
                         TEXT(".wav"), 0, NULL, NULL };

	if( TRUE == GetOpenFileName( &ofn ) )
    {
		m_input2 = strFileName;
    }
	UpdateData(FALSE);
}

void Cinterlacer_guiDlg::OnBnClickedButton4()
{
	UpdateData(TRUE);
    TCHAR strFileName[MAX_PATH];
	_tcscpy(strFileName, m_output);
    TCHAR strPath[MAX_PATH] = TEXT("");

    OPENFILENAME ofn = { sizeof(OPENFILENAME), this->GetSafeHwnd() , NULL,
                         TEXT(".mvc\0*.mvc\0\0"), NULL,
                         0, 1, strFileName, MAX_PATH, NULL, 0, strPath,
                         TEXT("Open Input 1"),
                         OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY, 0, 0,
                         TEXT(".wav"), 0, NULL, NULL };

	if( TRUE == GetSaveFileName( &ofn ) )
    {
		m_output = strFileName;
    }
	UpdateData(FALSE);
}

void Cinterlacer_guiDlg::OnBnClickedButton1()
{
	TCHAR interlacer_path[MAX_PATH+20] = _T("interlacer.exe");
	GetModuleFileName(NULL,interlacer_path,MAX_PATH);
	for(int i=_tcslen(interlacer_path); i>0; i--)
		if (interlacer_path[i] == _T('\\'))
		{
			interlacer_path[i] = NULL;
			break;
		}
	_tcscat(interlacer_path, _T("\\interlacer.exe"));


	FILE *f = _tfopen(interlacer_path, _T("rb"));
	if (f == NULL)
	{
		MessageBox(_T("interlacer.exe not found"), _T("Error"));
		return;
	}
	fclose(f);

	UpdateData(TRUE);

	CString parameter;
	parameter.Format(_T("\"%s\" \"%s\""), m_output, m_input1);
	if(m_input2 != _T(""))
		parameter += _T(" \"") + m_input2 + _T("\"");

	HINSTANCE inst = ShellExecute(NULL, NULL, interlacer_path, parameter, NULL, SW_SHOW);
	if ((int)inst < 32)
		MessageBox(_T("Error executing interlacer.exe ") + parameter, _T("Error"));
	else
		ExitProcess(0);
}
