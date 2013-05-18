#include "resource.h"
#include <windows.h>
#include "full_cache.h"

disk_manager *g_manager;
__int64 size;

// drawing
RECT rect;
HBRUSH brush_disk = CreateSolidBrush(RGB(0,0,255));
HBRUSH brush_net = CreateSolidBrush(RGB(255,0,128));
HBRUSH brush_read = CreateSolidBrush(RGB(0,255,0));
HBRUSH brush_preread = CreateSolidBrush(RGB(255,0,0));

HBRUSH brush0 = CreateSolidBrush(RGB(255,255,255));
HDC hdc;
HDC memDC;
HBITMAP bitmap;
HGDIOBJ obj;
HWND g_hwnd;

#define blockSize 5
#define resolution 200

void paint_pos(int i, int j, int type)
{
	HBRUSH color_table[] = {brush_disk, brush_net, brush_read, brush_preread};
	SelectObject(memDC, color_table[type]);

	POINT p[4];


	p[0].x = i*blockSize +1;
	p[0].y = j*blockSize +1;

	p[1].x = (i+1)*blockSize -1;
	p[1].y = (j)*blockSize +1;

	p[2].x = (i+1)*blockSize -1;
	p[2].y = (j+1)*blockSize -1;

	p[3].x = i*blockSize +1;
	p[3].y = (j+1)*blockSize -1;

	Polygon(memDC, p,4);

}

void begin_paint()
{
	GetClientRect(g_hwnd, &rect);
	hdc = GetDC(g_hwnd);
	memDC = CreateCompatibleDC(hdc);
	bitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
	obj = SelectObject(memDC, bitmap);


	FillRect(memDC, &rect, brush0);
}

void end_paint()
{
	BitBlt(hdc, 0, 0, rect.right, rect.bottom, memDC, 0, 0, SRCCOPY);

	DeleteObject(obj);
	DeleteObject(bitmap);
	DeleteDC(memDC);
	ReleaseDC(g_hwnd, hdc);
	return;
}

static INT_PTR CALLBACK dialog_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) 
	{
	case WM_COMMAND:
		EndDialog(hDlg, 0);
		break;

	case WM_INITDIALOG:
		g_hwnd = hDlg;
		SetTimer(hDlg, 1, 100, NULL);
		break;
	
	case WM_TIMER:
		{
			std::list<debug_info> debug = g_manager->debug();

			begin_paint();
			for(std::list<debug_info>::iterator i = debug.begin(); i!= debug.end(); ++i)
			{
				debug_info info = *i;

				int block_start = info.frag.start * resolution * resolution / size;
				int block_end = info.frag.end  * resolution * resolution / size;

				for(int j=block_start; j<=block_end; j++)
				{
					paint_pos(j%resolution, j/resolution, info.type);
				}
			}

			end_paint();
		}
		break;

	case WM_CLOSE:
		EndDialog(hDlg, 0);
		break;

	default:
		return FALSE;
	}

	return TRUE; // Handled message
}

int debug_window(disk_manager *manager)
{
	g_manager = manager;
	size = manager->getsize();
	return DialogBoxA(NULL, MAKEINTRESOURCEA(IDD_DIALOG1), NULL, dialog_proc);
}