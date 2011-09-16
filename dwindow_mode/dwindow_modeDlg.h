// dwindow_modeDlg.h : 头文件
//

#pragma once
#include "afxwin.h"


// Cdwindow_modeDlg 对话框
class Cdwindow_modeDlg : public CDialog
{
// 构造
public:
	Cdwindow_modeDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_DWINDOW_MODE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedRadio1();
	bool m_check_normal;
	bool m_check_bar;
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
protected:
	CString m_server;
public:
	int m_port;
	CEdit m_server_edit;
	CEdit m_port_edit;
	afx_msg void OnBnClickedRadio2();
	afx_msg void OnBnClickedOk();
};
