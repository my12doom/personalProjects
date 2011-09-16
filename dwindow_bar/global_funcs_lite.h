#pragma once
#include <Windows.h>
#include "..\libchecksum\libchecksum.h"

extern char g_passkey_big[128];
extern char *g_server_address;
#define g_server_gen_key "gen_key.php"
HRESULT download_url(char *url_to_download, char *out, int outlen = 64);
HRESULT check_passkey();


// settings
bool save_setting(const WCHAR *key, const void *data, int len, DWORD REG_TYPE=REG_BINARY);
int load_setting(const WCHAR *key, void *data, int len);
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