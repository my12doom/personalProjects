// dwindow_launcherDlg.h : 头文件
//

#pragma once
#include "afxwin.h"


// Cdwindow_launcherDlg 对话框
class Cdwindow_launcherDlg : public CDialog
{
// 构造
public:
	Cdwindow_launcherDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_DWINDOW_LAUNCHER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	bool kill_ssp(bool kill);
	bool check_module();

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton4();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton2();
	CButton m_regsvr_button;
	afx_msg void OnBnClickedButton5();
};
