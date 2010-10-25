// dwindow_launcher.h : PROJECT_NAME 应用程序的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error 在包含用于 PCH 的此文件之前包含“stdafx.h”
#endif

#include "resource.h"		// 主符号


// Cdwindow_launcherApp:
// 有关此类的实现，请参阅 dwindow_launcher.cpp
//

class Cdwindow_launcherApp : public CWinApp
{
public:
	Cdwindow_launcherApp();

// 重写
	public:
	virtual BOOL InitInstance();

// 实现

	DECLARE_MESSAGE_MAP()
};

extern Cdwindow_launcherApp theApp;
