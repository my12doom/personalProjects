#include "bar.h"
#include "resource.h"

bar_drawer::bar_drawer()
{
	resource = new DWORD[206*14];
	result = new DWORD[4096*30];
	draw_x = draw_y = 0;
	total_width = 500;
}

bar_drawer::~bar_drawer()
{
	delete [] resource;
	delete [] result;
}

void bar_drawer::load_resource(HINSTANCE hExe)
{
	HBITMAP bm = (HBITMAP)LoadImage(hExe, MAKEINTRESOURCE(IDB_BAR), IMAGE_BITMAP, 206, 14, LR_DEFAULTCOLOR);
	GetBitmapBits(bm, 206*14*4, resource);
	DeleteObject(bm);
}



int bar_drawer::copy_to_result_bar(int sx, int sy, int cx, int cy, int dx, int dy)
{
	if (dx+cx>4096)
		cx = 4096 - dx;
	if (dy+cy>30)
		cy = 30 - dy;

	if (cx<0 || cy <0)
		return 0;

	int delta_y = sy - dy;
	for (int y=sy; y<sy+cy; y++)
	{
		memcpy( result + (y - delta_y) * 4096 + dx,
			resource + (y) * 206 + sx,
			sizeof(DWORD) * cx
			);

	}

	return 0;
}
int bar_drawer::clear()
{
	memset(result, 0, 4096*30*sizeof(DWORD));
	return 0;
}

int bar_drawer::color_fill(int cx, int cy, DWORD color)
{
	for (int y=draw_y; y<draw_y+cy; y++)
	{
		for (int x=draw_x; x<draw_x+cx; x++)
		{
			*(result + y*4096 + x) = color;
		}
	}
	return 0;
}

int bar_drawer::draw(int sx, int sy, int cx, int cy)
{
	return copy_to_result_bar(sx, sy, cx, cy, draw_x, draw_y);
}

int bar_drawer::move(int x, int y, bool abs)
{
	if (abs)
	{
		draw_x = x;
		draw_y = y;
	}
	else
	{
		draw_x += x;
		draw_y += y;
	}
	return 0;
}

int bar_drawer::draw_digit(int digit)
{
	return draw(6 + digit * 9, 0, 9, 14);
}

int bar_drawer::draw_colon()
{
	return draw(0, 0, 6, 14);
}

int bar_drawer::draw_button(int button)
{
	int sx = 0;
	if (button == 0)		// play
		sx = 96;
	else if (button == 1)	// pause
		sx = 110;
	else if (button == 2)	// fullscreen
		sx = 192;
	else
		return -1;

	draw(sx, 0, 14, 14);

	return 0;
}
int bar_drawer::draw_volume(double volume)
{
	if (volume>1)
		volume = 1;
	if (volume<0)
		volume = 0;

	int sx = 124;

	draw(sx+34, 0, 34, 14);
	draw(sx, 0, (int)(34*volume), 14);

	return 0;
}

int bar_drawer::get_progress_total()
{
	const int other = 274;
	return total_width - other;
}

int bar_drawer::draw_progress(double progress)
{
	int progress_width = get_progress_total();

	DWORD color1 = resource[206*3-16];
	DWORD color2 = resource[206*2+7];

	move(0, 6);
	color_fill(progress_width, 2, color1);
	color_fill((int)(progress_width*progress), 2, color2);

	move(0,-6);

	return progress_width;
}
void bar_drawer::draw_time(int time)
{
	int hour = (time / 3600000) % 10;
	int minute = (time / 60000) % 60;
	int second = (time / 1000) % 60;

	int minute_1 = minute / 10;
	int minute_2 = minute % 10;

	int second_1 = second / 10;
	int second_2 = second % 10;

	int x = draw_x;
	int y = draw_y;


	draw_digit(hour);
	move(9,0);
	draw_colon();
	move(6,0);
	draw_digit(minute_1);
	move(9,0);
	draw_digit(minute_2);
	move(9,0);
	draw_colon();
	move(6,0);
	draw_digit(second_1);
	move(9,0);
	draw_digit(second_2);

	move(x,y,true);
}


int bar_drawer::draw_total(bool paused, int current_time, int total_time, double volume)
{
	const int inter = 14;

	clear();

	move(inter,8,true);			// inter 14

	if (paused)
		draw_button(0);
	else
		draw_button(1);			
	move(14,0);					// button 14

	move(inter,0);				// inter 14

	draw_time(current_time);
	move(57,0);					// time 57

	move(inter,0);				// inter 14

	if (total_time == 0)
		total_time = current_time = 1;
	int progress_width = draw_progress((double)current_time / total_time);
	move(progress_width,0);		// unknown

	move(inter,0);				// inter 14

	draw_time(total_time);
	move(57,0);					// time 57

	move(inter,0);				// inter 14

	draw_volume(volume);
	move(34,0);					// volume 34

	move(inter,0);				// inter 14

	draw_button(2);
	move(14,0);					// button14

	move(inter,0);				// inter 14;

	return 0;
}

int bar_drawer::hit_test(int x, int y, double *out_value)			// return value: hit button
																	// 0 = nothing, 1= pause/play, 2 = full
																	// 3 = volume,  4 = progress, out_value = value
																	// -1= 彻底出界
																	// out_value: volume or progressbar balue
{
	if (out_value)
		*out_value = 0;

	if (y<0 || y>=30 || x<0 || x>total_width)			// 按钮只要大致
		return -1;

	if (7 <= x && x < 35)
		return 1;										// play/pause button

	if (total_width - 32 <= x && x < total_width - 7)	// full button
		return 2;

	if (y<8 || y>=22)									// 滚动条和音量要更加精确
		return 0;

	if (total_width - 79 <= x && x < total_width - 39)	// volume  本应该是-76到-42,宽松3个像素来方便0%和100%
	{
		if (out_value)
		{
			*out_value = (double) (x-(total_width-76)) / 33.0;
			if(*out_value>1) *out_value = 1;
			if(*out_value<0) *out_value = 0;
		}
		return 3;
	}

	if (110 <= x && x < 113+get_progress_total())	// progress bar 本应该是113到113+width，左边宽松3像素方便0%
	{
		if (out_value)
		{
			*out_value = (double) (x-113) / get_progress_total();
			if(*out_value>1) *out_value = 1;
			if(*out_value<0) *out_value = 0;
		}
		return 4;
	}

	return 0;
}

