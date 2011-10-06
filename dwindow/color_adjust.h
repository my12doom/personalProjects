// no thread-safe and non-modal support, sorry
#pragma once
#include <Windows.h>

typedef enum enum_color_adjust_cb_parameter_type
{
	saturation = 0,
	luminance = 1,
	hue = 2,
	contrast = 3,

	saturation2 = 4,
	luminance2 = 5,
	hue2 = 6,
	contrast2 = 7,

	color_adjust_max = 8,
}color_adjust_cb_parameter_type;

class IColorAdjustCB
{
public:
	virtual HRESULT set_parameter(int parameter, double value) PURE;
	virtual HRESULT get_parameter(int parameter, double *value) PURE;
};

HRESULT show_color_adjust(HINSTANCE instance, HWND parent, IColorAdjustCB *cb);