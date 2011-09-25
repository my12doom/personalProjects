#ifndef SRT_PARSER_H
#define SRT_PARSER_H
#include <wchar.h>

class srt_parser
{
public:
	srt_parser();
	~srt_parser();
	void init(int num, int text_size);			//init or reset
	int load(wchar_t *pathname);
	int get_subtitle(int start, int end, wchar_t *out, bool multi = false);//size in wchar_t
	int direct_add_subtitle(wchar_t *line, int start, int end);

protected:
	typedef struct subtitle_tag
	{
		int time_start;
		int time_end;
		int pos;
	} subtitle_index;

	subtitle_index *m_index;
	wchar_t *m_text_data;
	int m_last_type;
	int m_index_pos;
	int m_text_pos;
	bool m_ass;
	bool m_ass_events_start;

private:
	int handle_data_16(unsigned short *data, bool big, int size);
	int handle_data_8(unsigned char *data, int code_page, int size);
	int handle_line(wchar_t *line);

	//helper functions
	wchar_t swap_big_little(wchar_t x);
	bool wisgidit(wchar_t *str);
	int wstrtrim(wchar_t *str, wchar_t char_ = L' ');
	int time_to_decimal(wchar_t *str);
};
#endif