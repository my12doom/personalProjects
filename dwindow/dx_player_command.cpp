#include "dx_player.h"
#include "..\renderer_prototype\YV12_to_RGB32.h"
#include "../png2raw/include/il/il.h"
#pragma comment(lib, "../png2raw/lib/DevIL.lib")

// helper functions
int wcscmp_nocase(const wchar_t*in1, const wchar_t *in2)
{
	wchar_t *tmp = new wchar_t[wcslen(in1)+1];
	wchar_t *tmp2 = new wchar_t[wcslen(in2)+1];
	wcscpy(tmp, in1);
	wcscpy(tmp2, in2);
	_wcslwr(tmp);
	_wcslwr(tmp2);

	int out = wcscmp(tmp, tmp2);

	delete [] tmp;
	delete [] tmp2;
	return out;
}

wchar_t * wcscpy2(wchar_t *out, const wchar_t *in)
{
	if (!out || !in)
		return NULL;
	return wcscpy(out, in);
}
wchar_t * wcscat2(wchar_t *out, const wchar_t *in)
{
	if (!out || !in)
		return NULL;
	return wcscat(out, in);
}


// helper classes
static const wchar_t myTRUE[] = L"True";
static const wchar_t myFALSE[] = L"False";
class myBool
{
public:
	myBool(const wchar_t *str, bool _default=false)
	{
		if (str == NULL)
			m_value = _default;
		else
			m_value = wcscmp_nocase(str, myTRUE) == 0;
	}
	myBool(const bool b)
	{
		m_value = b;
	}

	operator bool()
	{
		return m_value;
	}

	operator const wchar_t*()
	{
		return m_value ? myTRUE : myFALSE;
	}
protected:
	bool m_value;
};


class myInt
{
public:
	myInt(const wchar_t *str, int _default = 0)
	{
		if (str == NULL)
			m_value = _default;
		else
			m_value = _wtoi(str);
	}
	myInt(const int b)
	{
		m_value = b;
	}

	operator int()
	{
		return m_value;
	}

	operator const wchar_t*()
	{
		swprintf(m_str, L"%d", m_value);
		return m_str;
	}
protected:
	int m_value;
	wchar_t m_str[200];
};

class myDouble
{
public:
	myDouble(const wchar_t *str, double _default=0)
	{
		if (str == NULL)
			m_value = _default;
		else
			m_value = _wtof(str);
	}
	myDouble(const double b)
	{
		m_value = b;
	}

	operator double()
	{
		return m_value;
	}

	operator const wchar_t*()
	{
		swprintf(m_str, L"%f", m_value);
		return m_str;
	}
protected:
	double m_value;
	wchar_t m_str[200];
};

// the code
bool auth = false;
HRESULT dx_player::execute_command_adv(wchar_t *command, wchar_t *out, const wchar_t **args, int args_count)
{
	const wchar_t *p;
	#define SWTICH(x) p=x;if(false);
	#define CASE(x) else if (wcscmp_nocase(p, x) == 0)
	#define DEFAULT else

	HRESULT hr = S_OK;

	if (!auth && wcscmp_nocase(command, L"auth")!=0)
		return E_FAIL;

	SWTICH(command)

	CASE(L"shot")
	{
		const char *tmpFile = "Z:\\tmp.jpg";

		{
			RGBQUAD *dst = new RGBQUAD[1920*1080];
			m_renderer1->screenshot((BYTE*)dst);

			// save it to jpg
			CAutoLock lck(&g_ILLock);
			ilInit();
			ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
			ilEnable(IL_ORIGIN_SET);
// 			ilEnable(IL_FILE_OVERWRITE);
			ILuint imageNo = 0;
			ilGenImages(1, &imageNo);
			ilBindImage(imageNo);
			ilSetInteger(IL_JPG_QUALITY, 60);
			ILboolean result =ilTexImage(1920, 1080, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, dst);
// 			for (int y=0; y<1080; ++y)
// 			{
// 				ilSetPixels(0, y, 0, 1920, 1, 1, 1080, IL_UNSIGNED_BYTE, (void*) dst);
// 				dst += 1920;
// 			}

			DeleteFileA(tmpFile);
			result = ilSaveImage((wchar_t*)L"Z:\\tmp.jpg");
			if (!result)
			{
				ILenum err = ilGetError() ;
				printf( "the error %x\n", err );
				printf( "string is %s\n", ilGetString( err ) );
			}

		}

		FILE *f = fopen(tmpFile, "rb");
		fseek(f, 0, SEEK_END);
		int size = ftell(f);
		fseek(f, 0, SEEK_SET);
		*((int*)out) = size;
		memset(out+2, 0, size);
		int r = fread(out+2, 1, size, f);
		fclose(f);

		return S_FALSE;
	}

	CASE(L"auth")
	{
		auth = wcscmp(args[0], L"TestCode") == 0;
		wcscpy2(out, myBool(auth));
	}
	CASE(L"reset")
		hr = reset();

	CASE(L"start_loading")
		hr = start_loading();

	CASE(L"reset_and_loadfile")
		hr = reset_and_loadfile(args[0], myBool(args[1]));

	CASE(L"load_subtitle")
		hr = load_subtitle(args[0], myBool(args[1]));

	CASE(L"load_file")
		hr = load_file(args[0], myBool(args[1]), myInt(args[2]), myInt(args[3]));

	CASE(L"end_loading")
		hr = end_loading();



	CASE(L"set_subtitle_pos")
		hr = set_subtitle_pos(myDouble(args[0]), myDouble(args[1]));

	CASE(L"set_subtitle_parallax")
		hr = set_subtitle_parallax(myInt(args[0]));



	CASE(L"set_swapeyes")
		hr = set_swap_eyes(myBool(args[0]));

	CASE(L"set_movie_pos")
		hr = set_movie_pos(myDouble(args[0]), myDouble(args[1]));



	CASE(L"play_next_file")
		hr = playlist_play_next();



	CASE(L"play")
		hr = play();

	CASE(L"pause")
		hr = pause();

	CASE(L"stop")
		hr = stop();

	CASE(L"seek")
		hr = seek(myInt(args[0]));

	CASE(L"tell")
	{
		int now;
		hr = tell(&now);
		wcscpy2(out, myInt(now));
	}

	CASE(L"total")
	{
		int t;
		hr = total(&t);
		wcscpy2(out, myInt(t));
	}

	CASE(L"set_volume")
		hr = set_volume(myDouble(args[0]));

	CASE(L"get_volume")
	{
		double v;
		hr = get_volume(&v);
		wcscpy2(out, myDouble(v));
	}

	CASE(L"is_playing")
	{
		wcscpy2(out, myBool(is_playing()));
		hr = S_OK;
	}

	CASE(L"show_mouse")
	{
		hr = show_mouse(myBool(args[0]));
	}

	CASE(L"toggle_fullscreen")
	{
		hr = toggle_fullscreen();
	}

	CASE(L"set_output_mode")
	{
		hr = set_output_mode(myInt(args[0]));
	}

	CASE(L"is_fullscreen")
	{
		wcscpy2(out, myBool(m_full1));
		hr = S_OK;
	}

	CASE(L"list_bd3d")
	{
		wcscpy2(out, L"");
		for(int i=L'Z'; i>L'B'; i--)
		{
			wchar_t tmp[MAX_PATH] = L"C:\\";
			wchar_t tmp2[MAX_PATH];
			tmp[0] = i;
			tmp[4] = NULL;
			if (GetDriveTypeW(tmp) == DRIVE_CDROM)
			{
				wcscat2(out, tmp);
				if (!GetVolumeInformationW(tmp, tmp2, MAX_PATH, NULL, NULL, NULL, NULL, 0))
				{
					// no disc
					wcscat2(out, L"/|");
				}
				else if (FAILED(find_main_movie(tmp, tmp2)))
				{
					// not bluray
					wcscat2(out, L":|");
				}
				else
				{
					GetVolumeInformationW(tmp, tmp2, MAX_PATH, NULL, NULL, NULL, NULL, 0);
					wcscat2(out, tmp2);
					wcscat2(out, L"|");
				}

			}
		}
		hr = S_OK;
	}

	CASE(L"list_audio_track")
	{
		wchar_t tmp[32][1024];
		wchar_t *tmp2[32];
		bool connected[32];
		for(int i=0; i<32; i++)
			tmp2[i] = tmp[i];
		int found = 0;
		list_audio_track(tmp2, connected, &found);

		out[0] = NULL;
		for(int i=0; i<found; i++)
		{
			wcscat2(out, tmp2[i]);
			wcscat2(out, L"|");
			wcscat2(out, myBool(connected[i]));
		}
	}

	CASE(L"list_subtitle_track")
	{
		wchar_t tmp[32][1024];
		wchar_t *tmp2[32];
		bool connected[32];
		for(int i=0; i<32; i++)
			tmp2[i] = tmp[i];
		int found = 0;
		list_subtitle_track(tmp2, connected, &found);

		out[0] = NULL;
		for(int i=0; i<found; i++)
		{
			wcscat2(out, tmp2[i]);
			wcscat2(out, L"|");
			wcscat2(out, myBool(connected[i]));
			wcscat2(out, L"|");
		}
	}

	CASE(L"enable_audio_track")
		hr = enable_audio_track(myInt(args[0], -999));		// -999 should cause no harm

	CASE(L"enable_subtitle_track")
		hr = enable_subtitle_track(myInt(args[0], -999));	// -999 should cause no harm

	CASE(L"list_drive")
	{
		wchar_t *tmp = new wchar_t[102400];
		tmp[0] = NULL;

		for(int i=0; i<26; i++)
		{
			wchar_t path[50];
			wchar_t tmp2[MAX_PATH];
			swprintf(path, L"%c:\\", L'A'+i);
			if (GetVolumeInformationW(path, tmp2, MAX_PATH, NULL, NULL, NULL, NULL, 0))
			{
				wcscat2(tmp, path);
				wcscat2(tmp, L"|");
			}
		}
		wcscpy2(out, tmp);
		delete [] tmp;
		return S_OK;
	}

	CASE(L"list_file")
	{
		wchar_t *tmp = new wchar_t[102400];
		wcscpy2(tmp, args[0]);
		wcscat2(tmp, L"*.*");

		WIN32_FIND_DATAW find_data;
		HANDLE find_handle = FindFirstFileW(tmp, &find_data);
		tmp[0] = NULL;

		if (find_handle != INVALID_HANDLE_VALUE)
		{
			do
			{
				if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)!=0
						&& wcscmp(L".",find_data.cFileName ) !=0
						&& wcscmp(L"..", find_data.cFileName) !=0
					)
				{
					wcscat2(tmp, find_data.cFileName);
					wcscat2(tmp, L"\\|");
				}
				else if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0)

				{
					wcscat2(tmp, find_data.cFileName);
					wcscat2(tmp, L"|");
				}

			}
			while( FindNextFile(find_handle, &find_data ) );
		}

		wcscpy2(out, tmp);
		delete [] tmp;
		return S_OK;
	}

	DEFAULT
		return E_NOTIMPL;

	return hr;
}
