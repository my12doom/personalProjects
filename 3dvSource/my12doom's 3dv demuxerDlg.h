// my12doom's 3dv demuxerDlg.h : 头文件
//

#pragma once
#include "afxcmn.h"


// Cmy12dooms3dvdemuxerDlg 对话框
class Cmy12dooms3dvdemuxerDlg : public CDialog
{
// 构造
public:
	Cmy12dooms3dvdemuxerDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_MY12DOOMS3DVDEMUXER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;
	
	void cancel_work();
	bool cancel;
	HANDLE m_thread;
	static DWORD WINAPI worker_thread(LPVOID param);

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	CProgressCtrl m_progress;
	afx_msg void OnBnClickedButton1();
};
