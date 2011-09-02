#pragma  once
#include <Windows.h>
#include <atlbase.h>
#include <d3d9.h>
#include <dshow.h>
#include "..\libchecksum\libchecksum.h"


// setting class
class AutoSettingString;

// register window proc
INT_PTR CALLBACK register_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );


// public variables
extern AutoSettingString g_bar_server;
extern char g_passkey_big[128];
extern char g_passkey[32];
extern DWORD g_last_bar_time;
extern char *g_server_address;
#define HEARTBEAT_TIMEOUT 8000
#define g_server_E3D "w32.php"
#define g_server_gen_key "gen_key.php"
#define g_server_reg_check "reg_check.php"
extern int g_monitor_count;
extern HMONITOR g_monitors[16];
extern D3DADAPTER_IDENTIFIER9 g_ids[16];

//definitions
#define AmHresultFromWin32(x) (MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, x))

// GUIDs
DEFINE_GUID(CLSID_HaaliSimple, 0x8F43B7D9, 0x9D6B, 0x4F48, 0xBE, 0x18, 0x4D, 0x78, 0x7C, 0x79, 0x5E, 0xEA);
DEFINE_GUID(CLSID_E3DSource_TS, 0x0A68C3B5, 0x9164, 0x4A54, 0xAF, 0xAF, 0x99, 0x5B, 0x2F, 0xF0, 0xE0, 0xD4);
DEFINE_GUID(CLSID_E3DSource , 0x7de55957, 0xd7d6, 0x4386, 0x90, 0x78, 0x0f, 0xd2, 0xba, 0x96, 0xf4, 0x59);
DEFINE_GUID(CLSID_PD10_DEMUXER, 0xF07E981B, 0x0EC4, 0x4665, 0xA6, 0x71, 0xC2, 0x49, 0x55, 0xD1, 0x1A, 0x38);
DEFINE_GUID(CLSID_PD10_DECODER, 0xD00E73D7, 0x06f5, 0x44F9, 0x8B, 0xE4, 0xB7, 0xDB, 0x19, 0x1E, 0x9E, 0x7E);
DEFINE_GUID(CLSID_3dvSource, 0xfbcfd6af, 0xb25f, 0x4a6d, 0xae, 0x56, 0x5b, 0x53, 0x3, 0xf1, 0xa4, 0xe);
static const GUID CLSID_SSIFSource = { 0x916e4c8d, 0xe37f, 0x4fd4, { 0x95, 0xf6, 0xa4, 0x4e, 0x51, 0x46, 0x2e, 0xdf } };
static const GUID CLSID_my12doomSource = { 0x8FD7B1DE, 0x3B84, 0x4817, { 0xA9, 0x6F, 0x4C, 0x94, 0x72, 0x8B, 0x1A, 0xAE } };
DEFINE_GUID(CLSID_CoreAVC, 0x09571A4B, 0xF1FE, 0x4C60, 0x97, 0x60, 0xDE, 0x6D, 0x31, 0x0C, 0x7C, 0x31);
DEFINE_GUID(CLSID_LAVAudio, 0xe8e73b6b, 0x4cb3, 0x44a4, 0xbe, 0x99, 0x4f, 0x7b, 0xcb, 0x96, 0xe4, 0x91);
DEFINE_GUID(CLSID_XvidDecoder, 0x64697678, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
DEFINE_GUID(CLSID_DivxDecoder, 0x78766964, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
DEFINE_GUID(MEDIATYPE_Subtitle, 0xe487eb08, 0x6b26, 0x4be9, 0x9d, 0xd3, 0x99, 0x34, 0x34, 0xd3, 0x13, 0xfd);
DEFINE_GUID(MEDIASUBTYPE_UTF8, 0x87c0b230, 0x3a8, 0x4fdf, 0x80, 0x10, 0xb2, 0x7a, 0x58, 0x48, 0x20, 0xd);
DEFINE_GUID(MEDIASUBTYPE_PGS, 0x4eba53e, 0x9330, 0x436c, 0x91, 0x33, 0x55, 0x3e, 0xc8, 0x70, 0x31, 0xdc);

// funcs
extern wchar_t g_apppath[MAX_PATH];
HRESULT get_monitors_rect(RECT *screen1, RECT *screen2);
bool open_file_dlg(wchar_t *pathname, HWND hDlg, wchar_t *filter = NULL);
bool select_color(DWORD *color, HWND parent);
bool browse_folder(wchar_t *out, HWND owner = NULL);
HRESULT RemoveDownstream(CComPtr<IPin> &input_pin);
HRESULT set_lav_audio_bitstreaming(IBaseFilter *filter, bool active);
HRESULT find_main_movie(const wchar_t *folder, wchar_t *out);
HRESULT GetUnconnectedPin(IBaseFilter *pFilter,PIN_DIRECTION PinDir, IPin **ppPin);
HRESULT GetConnectedPin(IBaseFilter *pFilter,PIN_DIRECTION PinDir, IPin **ppPin);
HRESULT RemoveUselessFilters(IGraphBuilder *gb);
HRESULT DeterminPin(IPin *pin, wchar_t *name = NULL, CLSID majortype = CLSID_NULL, CLSID subtype = CLSID_NULL);
HRESULT GetPinByName(IBaseFilter *pFilter, PIN_DIRECTION PinDir, const wchar_t *name, IPin **ppPin);
HRESULT load_passkey();
HRESULT save_passkey();
HRESULT check_passkey();
HRESULT save_e3d_key(const unsigned char *file_hash, const unsigned char *file_key);
HRESULT load_e3d_key(const unsigned char *file_hash, unsigned char *file_key);
HRESULT download_e3d_key(const wchar_t *filename);
HRESULT make_xvid_support_mp4v();
HRESULT download_url(char *url_to_download, char *out, int outlen = 64, int timeout=INFINITE);
HRESULT bar_logout();
DWORD WINAPI killer_thread(LPVOID time);
DWORD WINAPI killer_thread2(LPVOID time);

// CoreMVC
HRESULT ActiveCoreMVC(IBaseFilter *decoder);
HRESULT beforeCreateCoreMVC();
HRESULT afterCreateCoreMVC();
class coremvc_hooker
{
public:
	coremvc_hooker();
	~coremvc_hooker();
};


// Settings loader & saver
bool save_setting(const WCHAR *key, const void *data, int len, DWORD REG_TYPE=REG_BINARY);
int load_setting(const WCHAR *key, void *data, int len);
bool del_setting(const WCHAR *key);
template<class ValueType>
class AutoSetting
{
public:
	AutoSetting(const wchar_t *key, const ValueType default_value, DWORD reg_type = REG_BINARY)
	{
		wcscpy(m_key, key);
		m_value = default_value;
		m_reg_type = reg_type;
		load_setting(key, &m_value, sizeof(ValueType));
	}
	~AutoSetting()
	{
		save_setting(m_key, &m_value, sizeof(ValueType), m_reg_type);
	}
	operator ValueType()
	{
		return m_value;
	}
	ValueType& operator= (ValueType in)
	{
		m_value = in;
		save_setting(m_key, &m_value, sizeof(ValueType), m_reg_type);
		return m_value;
	}

protected:
	DWORD m_reg_type;
	wchar_t m_key[256];
	ValueType m_value;
};


class AutoSettingString
{
public:
	void save()
	{
		save_setting(m_key, m_value, 1024, REG_SZ);
	}
	AutoSettingString(const wchar_t*key, const wchar_t *default_value)
	{
		wcscpy(m_key, key);
		m_value = new wchar_t[1024];
		wcscpy(m_value, default_value);
		load_setting(m_key, m_value, 1024);
	}
	~AutoSettingString()
	{
		save();
		delete m_value;
	}
	operator wchar_t*()
	{
		return m_value;
	}
	wchar_t* operator=(const wchar_t*in)
	{
		wcscpy(m_value, in);
		save_setting(m_key, m_value, 1024, REG_SZ);
		return m_value;
	}
protected:
	wchar_t m_key[256];
	wchar_t *m_value;
};


// localization
typedef enum{ENGLISH, CHINESE} localization_language;
extern AutoSetting<localization_language> g_active_language;
const wchar_t *C(const wchar_t *English);
HRESULT add_localization(const wchar_t *English, const wchar_t *Localized = NULL);
HRESULT set_localization_language(localization_language language);
HRESULT localize_menu(HMENU menu);


// CUDA
extern AutoSetting<bool> g_CUDA;
