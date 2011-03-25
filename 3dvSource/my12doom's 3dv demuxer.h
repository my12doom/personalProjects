// my12doom's 3dv demuxer.h : PROJECT_NAME 应用程序的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
#endif

#include "resource.h"		// 主符号


// Cmy12dooms3dvdemuxerApp:
// 有关此类的实现，请参阅 my12doom's 3dv demuxer.cpp
//

class Cmy12dooms3dvdemuxerApp : public CWinApp
{
public:
	Cmy12dooms3dvdemuxerApp();

// 重写
	public:
	virtual BOOL InitInstance();

// 实现

	DECLARE_MESSAGE_MAP()
};

extern Cmy12dooms3dvdemuxerApp theApp;