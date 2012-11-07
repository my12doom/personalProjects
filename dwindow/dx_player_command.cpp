#include "dx_player.h"

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


// helper classes
static const wchar_t myTRUE[] = L"True";
static const wchar_t myFALSE[] = L"False";
class myBool
{
public:
	myBool(const wchar_t *str)
	{
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
	myInt(const wchar_t *str)
	{
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
	myDouble(const wchar_t *str)
	{
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
HRESULT dx_player::execute_command_adv(wchar_t *command, wchar_t *out, const wchar_t **args, int args_count)
{
	const wchar_t *p;
	#define SWTICH(x) p=x;if(false);
	#define CASE(x) else if (wcscmp_nocase(p, x) == 0)
	#define DEFAULT else

	HRESULT hr = S_OK;

	SWTICH(command)
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
		wcscpy(out, myInt(now));
	}

	CASE(L"total")
	{
		int t;
		hr = total(&t);
		wcscpy(out, myInt(t));
	}

	CASE(L"set_volume")
		hr = set_volume(myDouble(args[0]));

	CASE(L"get_volume")
	{
		double v;
		hr = get_volume(&v);
		wcscpy(out, myDouble(v));
	}

	CASE(L"is_playing")
	{
		wcscpy(out, myBool(is_playing()));
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
		wcscpy(out, myBool(m_full1));
		hr = S_OK;
	}

	DEFAULT
		return E_NOTIMPL;

	return hr;
}
