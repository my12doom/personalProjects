// lrtbDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "resource.h"
#include <math.h>
#include "lrtb.h"
#include "lrtbDlg.h"
#include "LayoutDetect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// ClrtbDlg 对话框




ClrtbDlg::ClrtbDlg(CWnd* pParent /*=NULL*/)
	: CDialog(ClrtbDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void ClrtbDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS1, m_progress);
}

BEGIN_MESSAGE_MAP(ClrtbDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_WM_DROPFILES()
END_MESSAGE_MAP()


// ClrtbDlg 消息处理程序

BOOL ClrtbDlg::OnInitDialog()
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

void ClrtbDlg::OnPaint()
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
HCURSOR ClrtbDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void ClrtbDlg::OnBnClickedButton1()
{
	//layout_detector detector(100, L"E:\\test_folder\\24.avi");
	//layout_detector detector(100, L"D:\\Users\\my12doom\\Documents\\非常小特务3_上下格式720-480.ts.mkv");
	//layout_detector detector(100, L"F:\\TDDOWNLOAD\\3840.ts");
	//layout_detector detector(100, L"F:\\TDDOWNLOAD\\00019hsbs.mkv");

}

void ClrtbDlg::OnDropFiles(HDROP hDropInfo)
{
	int cFiles = DragQueryFile(hDropInfo, (UINT)-1, NULL, 0); 

	if ( cFiles > 0)
	{
		SetWindowText(_T("检测中"));
		TCHAR FilePath[MAX_PATH];
		DragQueryFile(hDropInfo, 0, FilePath, sizeof(FilePath));
		CString file = FilePath;

		int nscan = 100;
		layout_detector detector(nscan, (LPCWSTR)file);
		while (detector.m_scaned < nscan)
		{
			m_progress.SetRange(1, nscan);
			m_progress.SetPos(detector.m_scaned);
			UpdateData(FALSE);
			MSG msg;

			// Process existing messages in the application's message queue.
			// When the queue is empty, do clean up and return.
			while (::PeekMessage(&msg,NULL,0,0,PM_NOREMOVE))
			{
				if (!AfxGetThread()->PumpMessage())
					break;
			}
			Sleep(1);
		}

		int sbs = 0;
		int tb = 0;
		int normal = 0;
		int logo = 0;
		for(int i=0; i<nscan; i++)
		{
			if (detector.sbs_result[i] - detector.tb_result[i] > 0.1 || detector.sbs_result[i]/detector.tb_result[i]>1.2)
				sbs++;

			else if (detector.tb_result[i] - detector.sbs_result[i] > 0.1 || detector.tb_result[i]/detector.sbs_result[i]>1.2)
				tb++;

			else if (detector.tb_result[i] > 0.9 && detector.sbs_result[i] > 0.9)
			{
				//logo image with no stereo
				logo ++;
			}

			else
				normal++;
		}

		CString out;
		if (normal > sbs+tb)
		{
			if (sbs+tb == 0)
				out.Format(_T("非立体影片(%d%%)"), normal*100/(normal+tb+sbs));
			else
				out.Format(_T("非立体影片(%d%%), 左右影片(%d%%), 上下影片(%d%%)"), normal*100/(normal+tb+sbs), sbs*100/(sbs+tb), tb*100/(sbs+tb));
		}
		else if (sbs > tb && sbs>0)
			out.Format(_T("左右格式(%d%%)"), sbs*100/(sbs+tb));
		else if (tb > sbs && tb>0)
			out.Format(_T("上下格式(%d%%)"), tb*100/(sbs+tb));
		else
			out = _T("检测失败");

		FILE *f = fopen("E:\\test_folder\\debug.log", "wb");
		if(f)
		{
		for(int i=0; i<nscan; i++)
			fprintf(f, "%.2f - %.2f\r\n", detector.sbs_result[i], detector.tb_result[i]);
		fclose(f);
		}
		//SetDlgItemText(IDC_STATIC1, (LPCTSTR)out);
		SetWindowText((LPCTSTR)out);

		UpdateData(FALSE);	
	}


	CDialog::OnDropFiles(hDropInfo);
}
