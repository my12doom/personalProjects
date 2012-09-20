#include "dx_player.h"

#ifdef VSTAR

#define safe_decommit(x) if(x)x->decommit()
#define safe_commit(x) if(x)x->commit()

HRESULT dx_player::init_gpu(int width, int height, IDirect3DDevice9 *device)
{
	safe_commit(m_toolbar_background);
	safe_commit(m_UI_logo);
	for(int i=0; i<7; i++)
		safe_commit(m_buttons[i]);
	for(int i=0; i<6; i++)
		safe_commit(m_progress[i]);
	safe_commit(m_volume_base);
	safe_commit(m_volume_button);

	m_Device = device;
	m_width = width;
	m_height = height;


	return S_OK;
}

HRESULT dx_player::init_cpu(int width, int height, IDirect3DDevice9 *device)
{
	m_Device = device;
	m_width = width;
	m_height = height;

	if (m_toolbar_background == NULL) m_renderer1->loadBitmap(&m_toolbar_background, L"界面条2.png");
	if (m_UI_logo == NULL) m_renderer1->loadBitmap(&m_UI_logo, L"logo2.jpg");


	if (m_toolbar_background == NULL) m_renderer1->loadBitmap(&m_toolbar_background, L"toolbar_background.png");
	if (m_volume_base == NULL) m_renderer1->loadBitmap(&m_volume_base, L"volume_base.png");
	if (m_volume_button == NULL) m_renderer1->loadBitmap(&m_volume_button, L"volume_button.png");

	wchar_t buttons_pic[7][MAX_PATH] = 
	{
		L"fullscreen.png",
		L"喇叭.png",
		L"前进.png",
		L"播放.png",
		L"后退.png",
		L"停止.png",
		L"3d.png",
	};
	for(int i=0; i<7; i++)
		if (m_buttons[i] == NULL) m_renderer1->loadBitmap(&m_buttons[i], buttons_pic[i]);


	wchar_t progress_pics[6][MAX_PATH] = 
	{
		L"progress_left_base.png",
		L"progress_center_base.png",
		L"progress_right_base.png",
		L"progress_left_top.png",
		L"progress_center_top.png",
		L"progress_right_top.png",
	};
	for(int i=0; i<6; i++)
		if (m_progress[i] == NULL) m_renderer1->loadBitmap(&m_progress[i], progress_pics[i]);

	return S_OK;
}

HRESULT dx_player::invalidate_gpu()
{
	safe_decommit(m_toolbar_background);
	safe_decommit(m_UI_logo);
	for(int i=0; i<7; i++)
		safe_decommit(m_buttons[i]);
	for(int i=0; i<6; i++)
		safe_decommit(m_progress[i]);
	safe_decommit(m_volume_base);
	safe_decommit(m_volume_button);


	m_ps_UI = NULL;
	m_vertex = NULL;
	m_ui_logo_gpu = NULL;
	m_ui_tex_gpu = NULL;
	m_ui_background_gpu = NULL;
	return S_OK;
}


HRESULT dx_player::invalidate_cpu()
{
	invalidate_gpu();
	m_ui_logo_cpu = NULL;
	m_ui_tex_cpu = NULL;

	safe_delete(m_toolbar_background);
	safe_delete(m_UI_logo);
	for(int i=0; i<7; i++)
		safe_delete(m_buttons[i]);
	for(int i=0; i<6; i++)
		safe_delete(m_progress[i]);
	safe_delete(m_volume_base);
	safe_delete(m_volume_button);

	return S_OK;
}
HRESULT dx_player::draw_ui(IDirect3DSurface9 * surface, bool running)
{

	m_Device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	m_Device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	// calculate alpha
	bool showui = m_show_ui && m_theater_owner == NULL;

	float delta_alpha = 1-(float)(timeGetTime()-m_ui_visible_last_change_time)/fade_in_out_time;
	delta_alpha = max(0, delta_alpha);
	delta_alpha = min(1, delta_alpha);
	float alpha = showui ? 1.0f : 0.0f;
	alpha -= showui ? delta_alpha : -delta_alpha;

	// UILayout
	int client_width = m_width;
	int client_height = m_height;
	int x = client_width - 40 - 32;
	int y = client_height - 40 - 8;

	// toolbar background
	RECTF t = {0, client_height - 65, client_width, client_height};
	m_renderer1->Draw(surface, m_toolbar_background, NULL, &t, alpha);


	// buttons
	for(int i=0; i<7; i++)
	{
		RECTF rect = {x, y, x+40, y+40};
		m_renderer1->Draw(surface,m_buttons[i], NULL, &rect, alpha);

		x -= 62;
	}

	// left size : 5
	// center size : 16
	// right size : 6
	int x_max = client_width - 460;
	int x_min = 37;
	int y_min = client_height - 37;
	int y_max = y_min + 21;

	int total_time = 0;
	total(&total_time);

	double value = (double) m_current_time / total_time;

	enum
	{
		left_base,
		center_base,
		right_base,
		left_top,
		center_top,
		right_top,
	};

	// draw progressbar base
	RECTF l = {x_min, y_min, x_min + 5, y_max};
	m_renderer1->Draw(surface,m_progress[left_base], NULL, &l, alpha);

	RECTF l2 = {x_min + 5, y_min, x_max-6, y_max};
	m_renderer1->Draw(surface,m_progress[center_base], NULL, &l2, alpha);

	RECTF l3 = {x_max-6, y_min, x_max, y_max};
	m_renderer1->Draw(surface,m_progress[right_base], NULL, &l3, alpha);

	// draw progressbar top
	int progress_width = x_max - x_min;
	float v = value * progress_width;
	if (v > 1.5)
	{
		RECTF t1 = {x_min, y_min, x_min + min(5,v), y_max};
		m_renderer1->Draw(surface,m_progress[left_top], NULL, &t1, alpha);
	}

	if (v > 5)
	{
		float r = min(x_max-6, x_min + v);
		RECTF t2 = {x_min + 5, y_min, r, y_max};
		m_renderer1->Draw(surface,m_progress[center_top], NULL, &t2, alpha);
	}

	if (v > progress_width - 6)
	{
		RECTF t3 = {x_max-6, y_min, x_max - (progress_width - v), y_max};
		m_renderer1->Draw(surface,m_progress[right_top], NULL, &t3, alpha);
	}

	// calculate volume alpha
	delta_alpha = 1-(float)(timeGetTime()-m_volume_visible_last_change_time)/fade_in_out_time;
	delta_alpha = max(0, delta_alpha);
	delta_alpha = min(1, delta_alpha);
	alpha = m_show_volume_bar ? 1.0f : 0.0f;
	alpha -= m_show_volume_bar ? delta_alpha : -delta_alpha;


	// draw volume bottom
	RECTF volume_rect = {client_width - 156, client_height - 376, 
		client_width - 156 + 84, client_height - 376 + 317};
	m_renderer1->Draw(surface,m_volume_base, NULL, &volume_rect, alpha);


	// draw volume button
	int volume_bar_height = 275;
	double ypos = 23 + volume_bar_height * (1-m_volume);
	RECTF button_rect = {volume_rect.left + 42 - 20,  volume_rect.top + ypos - 20};
	button_rect.bottom = button_rect.top + 40;
	button_rect.right = button_rect.left + 40;
	m_renderer1->Draw(surface,m_volume_button, NULL, &button_rect, alpha);

	return S_OK;
}

HRESULT dx_player::draw_nonmovie_bg(IDirect3DSurface9 *surface, bool left_eye)
{
	int client_width = m_width;
	int client_height = m_height;

	RECTF logo_rect = {client_width/2 - 960, client_height/2 - 540,
		client_width/2 + 960, client_height/2 + 540};
 	m_renderer1->Draw(surface, m_UI_logo, NULL, &logo_rect, 1);

	return S_OK;
}

#define rt(x) {*out=(x);return S_OK;}
HRESULT dx_player::hittest(int x, int y, int *out, double *out_value /* = NULL */)
{
	if (out_value)
		*out_value = 0;

	// hidden volume and brightness 
	if (((m_width - 72 <= x && x < m_width)  ||
		(0 <= x && x < 72))
		&& y < m_height - 64
		)
	{
		if (out_value)
		{
			*out_value = (double)(m_height-y) / (m_height-64);
			if(*out_value>1) *out_value = 1;
			if(*out_value<0) *out_value = 0;
		}
		rt(x < 100 ? hit_brightness : hit_volume2);
	}

	int button_x = m_width - 40 - 32;
	int button_outs[7] = {hit_full, hit_volume_button, hit_next, hit_play, hit_previous, hit_stop, hit_3d_swtich};
	for(int i=0; i<7; i++)
	{
		if (m_height - 52 < y && y< m_height - 4 && button_x < x && x < button_x + 40)
			rt(button_outs[i]);
		button_x -= 62;
	}

	// progress bar
	int progressbar_right = m_width - 460;
	int progressbar_left = 37;


	if (m_height - 52 < y && y< m_height - 4 && progressbar_left - 5 < x && x < progressbar_right + 5)
	{
		if (out_value)
		{
			*out_value = (double)(x-progressbar_left) / (progressbar_right - progressbar_left);
			if(*out_value>1) *out_value = 1;
			if(*out_value<0) *out_value = 0;
		}
		rt(hit_progress);
	}

	// volume bar
	RECTF volume_rect = {m_width - 156, m_height - 376, 
		m_width - 156 + 84, m_height - 376 + 317};
	if (volume_rect.left < x && x < volume_rect.right &&
		volume_rect.top < y && y < volume_rect.bottom)
	{
		if (out_value)
		{
			int volume_bar_height = 275;
			*out_value = 1 - (double)(y - 23 - volume_rect.top) / volume_bar_height;
			if(*out_value>1) *out_value = 1;
			if(*out_value<0) *out_value = 0;
		}
		if (m_show_volume_bar)
			rt(hit_volume);
	}


	*out = y < m_height - 64 ? hit_logo : hit_bg;
	return S_OK;
}

#endif