// remux_publishDlg.h : 头文件
//

#pragma once


// Cremux_publishDlg 对话框
class Cremux_publishDlg : public CDialog
{
// 构造
public:
	Cremux_publishDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_REMUX_PUBLISH_DIALOG };

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


	// remux control:
	wchar_t pathname[MAX_PATH];
	int check_result;
	void publish_signature(const DWORD *checksum, DWORD *signature);
	void check_file();
public:
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnBnClickedButton2();
};
