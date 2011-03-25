#ifndef BAR_DRAWER_H
#define BAR_DRAWER_H

#include <windows.h>

class bar_drawer
{
public:
	DWORD * result;
	int draw_x;
	int draw_y;
	int total_width;

	bar_drawer();
	~bar_drawer();
	void load_resource(HINSTANCE hExe);
	int copy_to_result_bar(int sx, int sy, int cx, int cy, int dx, int dy);
	int clear();
	int color_fill(int cx, int cy, DWORD color);
	int draw(int sx, int sy, int cx, int cy);
	int move(int x, int y, bool abs = false);
	int draw_digit(int digit);
	int draw_colon();
	int draw_button(int button);		// 0 = play, 1 = pause, 2 = full
	int get_progress_total();
	int draw_progress(double progress);
	int draw_volume(double volume);
	void draw_time(int time);

	int draw_total(bool paused, int current_time, int total_time, double volume);
	int hit_test(int x, int y, double *out_value);	// 0 = nothing, 1= pause/play, 2 = full
												// 3 = volume,  4 = progress, value = value
												// -1= ³¹µ×³ö½ç
protected:
	DWORD * resource;
};

#endif