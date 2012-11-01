#pragma once

#include <Windows.h>
#include "avisynth.h"

class sq2sbs : public GenericVideoFilter 
{
public:
	sq2sbs(PClip _child);
	~sq2sbs();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

	typedef struct struct_thread_parameter
	{
		int left;
		int right;
		DWORD *out;
		sq2sbs * _this;
	} thread_parameter;

	static DWORD WINAPI core_thread_entry(LPVOID parameter){return ((thread_parameter*)parameter)->_this->core_thread((thread_parameter*)parameter);}
	DWORD core_thread(thread_parameter *p);

	BYTE *YDataPadded;
};
