#include <stdio.h>
#include <Windows.h>
#include "srt_parser.h"

bool wcs_replace(wchar_t *to_replace, const wchar_t *searchfor, const wchar_t *replacer);

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

wchar_t *srt_parser::decimal_to_time(int decimal)		// warning: no MT support for this function, it returns a point to its internal static wchar_t[];
{
	//11:22:33,444
	static wchar_t out[15];
	int h = decimal / 3600000;
	int m = (decimal % 3600000) / 60000;
	int s = (decimal % 60000) / 1000;
	int ms = decimal % 1000;
	swprintf(out, L"%02d:%02d:%02d,%03d", h, m, s, ms);

	return out;
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
	memset(m_index, 0, sizeof(subtitle_index) * num);
	m_text_data = new wchar_t[text_size];
	tmp [0] = NULL;
	m_last_type = 0;
	m_index_pos = 0;
	m_text_pos = 0;
	m_text_data[0] = NULL;
}

wchar_t * wcsstr_nocase(const wchar_t *search_in, const wchar_t *search_for)
{
	wchar_t *tmp = new wchar_t[wcslen(search_in)+1];
	wchar_t *tmp2 = new wchar_t[wcslen(search_for)+1];
	wcscpy(tmp, search_in);
	wcscpy(tmp2, search_for);
	_wcslwr(tmp);
	_wcslwr(tmp2);

	wchar_t *out = wcsstr(tmp, tmp2);

	delete [] tmp;
	delete [] tmp2;
	return out ? out - tmp + (wchar_t*)search_in : NULL;
}

int srt_parser::load(wchar_t *pathname)
{
	if (!m_index)
		return -1;

	m_ass = wcsstr_nocase(pathname, L"ssa") || wcsstr_nocase(pathname, L"ass");
	m_ass_events_start = false;

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

int srt_parser::load_offset_metadata(const wchar_t *pathname, int fps /* = 24 */)		// sorry, no Unicode support
{
	FILE * f = _wfopen(pathname, L"rb");
	if (NULL == f) return -1;

	int frame_count = 0;
	int *offset_data = (int*) malloc(2048000);

	while (fscanf(f, "%d", offset_data+frame_count) != EOF)
		frame_count++;

	for(int i=0; i<m_index_pos; i++)
	{
		int frame_start = (int)(m_index[i].time_start * fps / 1001 );
		int frame_end = (int)(m_index[i].time_end * fps / 1001 );
		if (frame_start<frame_count && frame_end < frame_count)
		{
			// list all offsets;
			int offset_list[256];
			memset(offset_list, 0, sizeof(offset_list));
			for(int j = frame_start; j<=frame_end; j++)
				offset_list[offset_data[j]+128] ++;

			// find most common offset
			int offset = 0;
			int max_found = 0;
			for(int j=0; j<256; j++)
				if (offset_list[j] > max_found)
				{
					max_found = offset_list[j];
					offset = j-128;
				}

			// you know what
			m_index[i].has_offset = true;
			m_index[i].offset = offset;
		}
	}

	free(offset_data);
	return 0;
}

int srt_parser::save(const wchar_t *pathname)
{
	FILE * f = _wfopen(pathname, L"wb");
	if (NULL == f) return -1;

	unsigned char tmp[1024];

	// write BOM, UTF-16 Little
	tmp[0] = 0xFF;
	tmp[1] = 0xFE;
	tmp[2] = 0x00;
	fwrite(tmp, 1, 2, f);

	for(int i=0; i<m_index_pos; i++)
	{
		int sub_start = m_index[i].time_start;
		int sub_end = m_index[i].time_end;
		wchar_t start[15];
		wchar_t end[15];
		wchar_t text[1024];

		wcscpy(start, decimal_to_time(sub_start));
		wcscpy(end, decimal_to_time(sub_end));
		wcscpy(text, m_text_data + m_index[i].pos);
		bool has_offset = m_index[i].has_offset;
		int offset = m_index[i].offset;
replace:
		for(unsigned int j=1; j<wcslen(text); j++)
		{
			if (text[j] == L'\n' && text[j-1] != L'\r')
			{
				memmove(text+j+1, text+j, sizeof(wchar_t) * (wcslen(text) - j+1));
				text[j] = L'\r';
				goto replace;
			}
		}

		fwprintf(f, L"%d\r\n", i+1);
		fwprintf(f, L"%s --> %s\r\n", start, end);
		if (has_offset) fwprintf(f, L"<offset=%d>", offset);
		fwprintf(f, L"%s\r\n\r\n", text);
	}

	fclose(f);

	return 0;
}

int srt_parser::get_subtitle(int start, int end, wchar_t *out, bool *has_offset /* = NULL */, int *offset /* = NULL */, bool multi /* = false */)	//size in wchar_t
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
				if (has_offset)
					*has_offset = m_index[i].has_offset;
				if (offset)
					*offset = m_index[i].offset;
				return i;
			}
		}
	return 0;
}

int srt_parser::direct_add_subtitle(wchar_t *line, int start, int end)
{
	// find offset tag <offset=xx>
	bool has_offset = false;
	int offset = 0;
	wchar_t *str_offset = wcsstr(line, L"<offset=");
	if (str_offset)
	{
		has_offset = true;
		offset = _wtoi(str_offset + wcslen(L"<offset="));
	}

	// remove <XXX> in the line
	wchar_t *l = wcsstr(line, L"<");
	wchar_t *r = wcsstr(line, L">");
	while (l && r)
	{
		wcscpy(l, r+1);
		l = wcsstr(line, L"<");
		r = wcsstr(line, L">");
	}

	// find duplicate
	for(int i=0; i<m_index_pos; i++)
		if (abs(m_index[i].time_start - start) < 10 && abs(m_index[i].time_end - end) < 10)
			return -1;

	m_index[m_index_pos].time_start = start;
	m_index[m_index_pos].time_end = end;
	m_index[m_index_pos].has_offset = has_offset;
	m_index[m_index_pos].offset = offset;

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
	if (m_ass)
	{
		if (!m_ass_events_start)
		{
			if (wcscmp(line, L"[Events]"))
				m_ass_events_start = true;
			else
				return 0;
		}
		else
		{
			// dialogs

			// find start time and end time
			wchar_t str_start[1024];
			wchar_t str_end[1024];
			wchar_t *tmp = wcsstr(line, L",");
			if (!tmp)
				return 0;

			wcscpy(str_start, tmp+1);
			tmp = wcsstr(str_start, L",");
			if (!tmp)
				return 0;
			tmp[0] = NULL;

			wcscpy(str_end, tmp+1);
			tmp = wcsstr(str_end, L",");
			if (!tmp)
				return 0;
			tmp[0] = NULL;

			wstrtrim(str_start);
			wstrtrim(str_end);

			// add leading and tailing 0
			if (str_start[1] == L':')
			{
				str_start[wcslen(str_start)+1] = NULL;
				memmove(str_start+1, str_start, wcslen(str_start)*2);
				str_start[0] = L'0';
			}
			if (str_end[1] == L':')
			{
				str_end[wcslen(str_end)+1] = NULL;
				memmove(str_end+1, str_end, wcslen(str_end)*2);
				str_end[0] = L'0';
			}
			wcscat(str_start, L"0");
			wcscat(str_end, L"0");

			int start = time_to_decimal(str_start);
			int end = time_to_decimal(str_end);



			// remove 9 commas to get text line
			int commas_left = 9;
			while (wchar_t *comma = wcsstr(line, L","))
			{
				if (!commas_left)
					break;

				commas_left--;
				wcscpy(line, comma+1);
			}

			// remove {XXX} in the line
			wchar_t *l = wcsstr(line, L"{");
			wchar_t *r = wcsstr(line, L"}");
			while (l && r)
			{
				wcscpy(l, r+1);
				l = wcsstr(line, L"{");
				r = wcsstr(line, L"}");
			}

			wcs_replace(line, L"\\N", L"\n");

			direct_add_subtitle(line, start, end);

		}
	}

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

			/*
			m_index[m_index_pos].time_start = ms_start;
			m_index[m_index_pos].time_end = ms_end;

			if (m_text_data[0] != NULL)
				m_text_pos += (int)wcslen(m_text_data + m_text_pos) + 1;

			m_index[m_index_pos].pos = m_text_pos;

			m_text_data[m_text_pos] = NULL;

			m_index_pos ++;
			*/

			if (tmp[0])
				direct_add_subtitle(tmp, tmp_index.time_start, tmp_index.time_end);

			tmp_index.time_start = ms_start;
			tmp_index.time_end = ms_end;
			tmp[0] = NULL;

			return 0;
		}
	}

	//text:
	if (m_last_type >= 2 && m_index_pos >= 0)
	{
		m_last_type = 3;

		if ( tmp[0] == NULL)
			wcscpy(tmp, line);
		else
		{
			wcscat(tmp, L"\n");
			wcscat(tmp, line);
		}
	}	
	return 0;
}


