#include <stdio.h>
#include <Windows.h>
#include "srt_parser.h"

// helper functions
inline wchar_t srt_parser::swap_big_little(wchar_t x)
{
	wchar_t low = x & 0xff;
	wchar_t hi = x & 0xff00;
	return (low << 8) | (hi >> 8);
}

bool srt_parser::wisgidit(wchar_t *str)
{
	size_t len = wcslen(str);
	for (size_t i=0; i<len; i++)
		if (str[i] < L'0' || str[i] > L'9')
			return false;
	return true;
}
int srt_parser::wstrtrim(wchar_t *str, wchar_t char_ )
{
	int len = (int)wcslen(str);
	//LEADING:
	int lead = 0;
	for(int i=0; i<len; i++)
		if (str[i] != char_)
		{
			lead = i;
			break;
		}

	//ENDING:
	int end = 0;
	for (int i=len-1; i>=0; i--)
		if (str[i] != char_)
		{
			end = len - 1 - i;
			break;
		}
	//TRIMMING:
	memmove(str, str+lead, (len-lead-end)*sizeof(wchar_t));
	str[len-lead-end] = NULL;

	return len - lead - end;
}
int srt_parser::time_to_decimal(wchar_t *str)
{
	//11:22:33,444

	if (wcslen(str) != 12)
		return -1;

	int h,m,s,ms;
	if (str[8] == L',')
		swscanf(str, L"%d:%d:%d,%d", &h,&m,&s,&ms);
	else
		swscanf(str, L"%d:%d:%d.%d", &h,&m,&s,&ms);


	return h*3600000 + m*60000 +s*1000 +ms;
}


// end helper functions

// class functions
srt_parser::srt_parser()
{
	m_index = NULL;
	m_text_data = NULL;	
	m_last_type = 0;
	m_index_pos = 0;
	m_text_pos = 0;
}

srt_parser::~srt_parser()
{
	if (m_index)
		delete [] m_index;
	if (m_text_data)
		delete [] m_text_data;
}
//size in wchar_t
void srt_parser::init(int num, int text_size)
{
	m_index = new subtitle_index[num];
	m_text_data = new wchar_t[text_size];
	m_last_type = 0;
	m_index_pos = 0;
	m_text_pos = 0;
	m_text_data[0] = NULL;
}
int srt_parser::load(wchar_t *pathname)
{
	if (!m_index)
		return -1;

	FILE * f = _wfopen(pathname, L"rb");
	if (NULL == f) return -1;

	// read data
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);
	unsigned char *data = new unsigned char[size];
	fread(data, 1, size, f);
	fclose(f);


	if (data[0] == 0xFE && data[1] == 0xFF && data[3] != 0x00)				//UTF-16 Big Endian
		handle_data_16((unsigned short*)(data+2), true, (int)size-2);
	else if (data[0] == 0xFF && data[1] == 0xFE && data[2] != 0x00)			//UTF-16 Little Endian
		handle_data_16((unsigned short*)(data+2), false, (int)size-2);
	else if (data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF)			//UTF-8
		handle_data_8 (data+3, CP_UTF8, (int)size-3);
	else																	//Unknown BOM
		handle_data_8 (data, CP_ACP, (int)size);


	delete [] data;

	return 0;
}
int srt_parser::get_subtitle(int start, int end, wchar_t *out, bool multi)	//size in wchar_t
{
	if (!m_index)
		return -1;

	if (start > end)
		return 0;

	out[0] = NULL;

	if (multi)
	for(int i=m_index_pos-1; i>=0; i--)
	{
		int sub_start = m_index[i].time_start;
		int sub_end = m_index[i].time_end;
		if ( 
			 ( start <= sub_start && sub_start <= end ) ||
			 ( start <= sub_end && sub_end <= end) ||
			 ( sub_start <= start && start <= sub_end) ||
			 (sub_start <= end && end <= sub_end)
			 )
		{
			if (NULL == out[0] )
				wcscpy(out, m_text_data + m_index[i].pos);
			else
			{
				wcscat(out, L"\n");
				wcscat(out, m_text_data + m_index[i].pos);
			}
		}
	}

	else	// single
		for(int i=0; i<m_index_pos; i++)
		{
			int sub_start = m_index[i].time_start;
			int sub_end = m_index[i].time_end;
			if ( 
				( sub_start <= start && start <= sub_end) ||
				(sub_start <= end && end <= sub_end)
			 )
			{
				wcscpy(out, m_text_data + m_index[i].pos);
				return i;
			}
		}
	return 0;
}

int srt_parser::direct_add_subtitle(wchar_t *line, int start, int end)
{
	// find duplicate
	for(int i=0; i<m_index_pos; i++)
		if (abs(m_index[i].time_start - start) < 10 && abs(m_index[i].time_end - end) < 10)
			return -1;

	m_index[m_index_pos].time_start = start;
	m_index[m_index_pos].time_end = end;

	if (m_text_data[0] != NULL)
		m_text_pos += (int)wcslen(m_text_data + m_text_pos) + 1;

	m_index[m_index_pos].pos = m_text_pos;

	wcscpy(m_text_data+m_text_pos, line);

	m_index_pos ++;

	return 0;
}

int srt_parser::handle_data_16(unsigned short *data, bool big, int size)
{
	wchar_t line_w[1024];
	int p = 0;

	if(big)
		for (int i=0; i<size; i++)
		{
			wchar_t c = swap_big_little(data[i]);
			if (c != 0xA && c != 0xD && p<1024)
				line_w[p++] = c;
			else
			{
				if(c == 0xD)
				{
					line_w[p] = NULL;
					wstrtrim(line_w);

					if (NULL != line_w[0])
						handle_line(line_w);
					else
						m_last_type = 0;
				}
				else
					p = 0;
			}
		}
	else//little
		for (int i=0; i<size/2; i++)
		{
			wchar_t c = data[i];
			if (c != 0xA && c != 0xD && p<1024)
				line_w[p++] = c;
			else
			{
				if(c == 0xD)
				{
					line_w[p] = NULL;
					wstrtrim(line_w);

					if (NULL != line_w[0])
						handle_line(line_w);
					else
						m_last_type = 0;
				}
				else
					p = 0;
			}
		}

	return 0;
}

int srt_parser::handle_data_8(unsigned char *data, int code_page, int size)
{
	char line[1024];
	wchar_t line_w[1024];
	int p = 0;
	for (int i=0; i<size; i++)
	{
		if (data[i] != 0xA && data[i] != 0xD && p<1024)
			line[p++] = data[i];
		else
		{
			if(data[i] == 0xD)
			{
				line[p] = NULL;
				MultiByteToWideChar(code_page, 0, line, 1024, line_w, 1024);
				wstrtrim(line_w);

				if (NULL != line_w[0])
					handle_line(line_w);
				else
					m_last_type = 0;
			}
			else
				p = 0;
		}
	}

	return 0;
}
int srt_parser::handle_line(wchar_t *line)
{
	// number
	if (wisgidit(line) && m_last_type == 0)
	{
		m_last_type = 1;
		return 0;
	}

	//timecode:
	wchar_t *e_str = wcsstr(line, L"-->");
	if (e_str)
	{
		e_str[0] = NULL;
		wstrtrim(line);
		wstrtrim(e_str+3);
		int ms_start = time_to_decimal(line);
		int ms_end = time_to_decimal(e_str + 3);
		if (ms_start >= 0 && ms_end > 0 && m_last_type == 1)
		{
			m_last_type = 2;

			m_index[m_index_pos].time_start = ms_start;
			m_index[m_index_pos].time_end = ms_end;

			if (m_text_data[0] != NULL)
				m_text_pos += (int)wcslen(m_text_data + m_text_pos) + 1;

			m_index[m_index_pos].pos = m_text_pos;

			m_text_data[m_text_pos] = NULL;

			m_index_pos ++;

			return 0;
		}
	}

	//text:
	if (m_last_type >= 2 && m_index_pos >= 0)
	{
		m_last_type = 3;

		wchar_t * p = m_text_data + m_text_pos;

		if ( p[0] == NULL)
			wcscpy(p, line);
		else
		{
			wcscat(p, L"\n");
			wcscat(p, line);
		}
	}	
	return 0;
}


