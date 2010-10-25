#include <stdio.h>
#include "image.h"
#include "filter.h"


void image_processor_stereo::init_out_size()
{
	if (m_mode == DWindowFilter_CUT_MODE_AUTO)
	{
		double aspect = (double)m_in_x / m_in_y;
		if (aspect > 2.425)
			m_mode = DWindowFilter_CUT_MODE_LEFT_RIGHT;
		else if (0 < aspect && aspect < 1.2125)
			m_mode = DWindowFilter_CUT_MODE_TOP_BOTTOM;
	}

	if (m_mode == DWindowFilter_CUT_MODE_LEFT_RIGHT)
	{
		m_image_x = m_out_x = m_in_x /2;
		m_image_y = m_out_y = m_in_y;
	}
	else if (m_mode == DWindowFilter_CUT_MODE_TOP_BOTTOM)
	{
		m_image_x = m_out_x = m_in_x;
		m_image_y = m_out_y = m_in_y /2;
	}
	else
		return;				// normal aspect... possible not side by side picture


	if (m_image_x % 2 )
		m_image_x ++;

	if (m_image_y % 2 )
		m_image_y ++;

	//计算目标高度
	int out_y_extended = m_image_y;
	double aspect_x = -1, aspect_y = -1;
	if (m_extend == DWindowFilter_EXTEND_NONE)
		aspect_x = aspect_y = -1;
	else if (m_extend == DWindowFilter_EXTEND_43)
		aspect_x = 4, aspect_y = 3;
	else if (m_extend == DWindowFilter_EXTEND_54)
		aspect_x = 5, aspect_y = 4;
	else if (m_extend == DWindowFilter_EXTEND_169)
		aspect_x = 16,aspect_y = 9;
	else if (m_extend == DWindowFilter_EXTEND_1610)
		aspect_x = 16,aspect_y =10;
	else if (m_extend > 5 && m_extend < 0xf0000) //DWindowFilter_EXTEND_CUSTOM
	{
		aspect_x = (m_extend -5) & 0xff;
		aspect_y = ((m_extend -5) & 0xff00) >> 8;
	}
	else if ((m_extend & 0xff0000) == 0xf0000)		 //DWindowFilter_EXTEND_TO
		out_y_extended = m_extend & 0xffff;
	else if ((m_extend & 0xf00000) == 0xf00000)
	{
		aspect_x = (double) (m_extend & 0xfffff) / 100000;
		aspect_y = 1;
	}

	if (aspect_x > 0 && aspect_y > 0)
		out_y_extended = (int)(m_image_x * aspect_y / aspect_x);

	//计算黑边高度
	if (m_out_y >= out_y_extended)
	{
		m_extend = DWindowFilter_EXTEND_NONE;
		m_letterbox_top = m_letterbox_bottom = m_letterbox_height = 0;
		return;
	}
	m_letterbox_height = out_y_extended - m_out_y;					//黑边总高度
	m_letterbox_top = m_letterbox_bottom = m_letterbox_height /2;	//默认为上下等高

	m_out_y = out_y_extended;

	if (m_out_x % 2 )
		m_out_x ++;

	if (m_out_y % 2 )
		m_out_y ++;
}



void image_processor_stereo::compute_zones()								// 计算各个区的范围
{
	if (m_nomask || m_mask_width <= 0 || m_mask_height <= 0)
	{
		m_free_zone_top = m_image_y;
		m_mix_zone = 0;
		m_free_zone_bottom = 0;
	}
	else
	{
		m_free_zone_top = min(max(m_mask_pos_top, 0),m_image_y);

		int line_mix_start = min(max(0, m_mask_pos_top), m_image_y);
		int line_mix_end = max(min(m_image_y, m_mask_pos_top + m_mask_height),0);
		m_mix_zone = line_mix_end - line_mix_start;

		m_free_zone_bottom = m_image_y - m_mix_zone - m_free_zone_top;
	}
}

void image_processor_stereo::generate_masks()											//生成左右子mask，一般由set_mask_prop调用
{
	// AYUV(00,ff,80,80) = (0,255,128,128) = white
	unsigned char yuv_color_y = (unsigned char)((m_mask_color & 0x00ff0000) >> 16);
	unsigned char yuv_color_u = (unsigned char)((m_mask_color & 0x0000ff00) >> 8 );
	unsigned char yuv_color_v = (unsigned char)((m_mask_color & 0x000000ff)      );

	int lb_left  = m_mask_pos_left;												//左边界，左眼
	int lb_right = min(m_mask_pos_left+m_mask_width, m_out_x);					//右边界

	int rb_left  = m_mask_pos_left + m_mask_offset;								//左边界，右眼
	int rb_right = min(m_mask_pos_left+m_mask_width+m_mask_offset, m_out_x);	//右边界

	memset(m_mask_left_y, 0, m_out_x * m_mask_height * sizeof(short));			//全尺寸，混合
	memset(m_mask_right_y, 0, m_out_x * m_mask_height * sizeof(short));

	memset(m_mask_left_u, 0, m_out_x/2 * m_mask_height/2 * sizeof(short));		//半尺寸，混合
	memset(m_mask_left_v, 0, m_out_x/2 * m_mask_height/2 * sizeof(short));
	memset(m_mask_right_u, 0, m_out_x/2 * m_mask_height/2 * sizeof(short));
	memset(m_mask_right_v, 0, m_out_x/2 * m_mask_height/2 * sizeof(short));

	memset(m_mask_left_n, 255, m_out_x * m_mask_height);						//全尺寸，反色
	memset(m_mask_right_n,255, m_out_x * m_mask_height);

	memset(m_mask_left_uv_n,255, m_out_x/2 * m_mask_height/2);					//半尺寸，反色
	memset(m_mask_right_uv_n,255, m_out_x/2 * m_mask_height/2);

	memset(m_mask_letterbox_left_y, 255, m_out_x * m_mask_height);
	memset(m_mask_letterbox_left_v, 128, m_out_x/2 * m_mask_height/2);
	memset(m_mask_letterbox_left_u, 128, m_out_x/2 * m_mask_height/2);
	memset(m_mask_letterbox_right_y, 255, m_out_x * m_mask_height);
	memset(m_mask_letterbox_right_v, 128, m_out_x/2 * m_mask_height/2);
	memset(m_mask_letterbox_right_u, 128, m_out_x/2 * m_mask_height/2);



	// 左眼，全尺寸
	for (int y=0; y<m_mask_height; y++)
	{
		for (int x = max(0,m_mask_pos_left); x < lb_right; x++)
		{
			m_mask_left_y[x+y*m_out_x] = m_mask[y*m_mask_width + x - lb_left];
			m_mask_left_n[x+y*m_out_x] = 255 - m_mask_left_y[x+y*m_out_x];
		}
	}

	// 右眼，全尺寸
	for (int y=0; y<m_mask_height; y++)
	{
		for (int x = max(0,m_mask_pos_left+ m_mask_offset); x < rb_right; x++)
		{
			m_mask_right_y[x+y*m_out_x] = m_mask[y*m_mask_width + x - rb_left];
			m_mask_right_n[x+y*m_out_x] = 255 - m_mask_right_y[x+y*m_out_x];
		}
	}

	// 半尺寸，以及全尺寸Y混合
	for(int y=0; y<m_mask_height/2; y++)
		for(int x=0; x<m_out_x/2; x++)
		{
			int x2 = x*2;
			int y2 = y*2;
			int half_pos = x+m_out_x/2*y;
			//左眼
			m_mask_left_u[half_pos] =(m_mask_left_y[x2   + y2   *m_out_x] +
									m_mask_left_y[x2+1 + y2   *m_out_x] +
									m_mask_left_y[x2   +(y2+1)*m_out_x] +
									m_mask_left_y[x2+1 +(y2+1)*m_out_x]) /4;

			m_mask_left_v[half_pos] = m_mask_left_u[half_pos] * yuv_color_v;	//左眼 V
			m_mask_left_u[half_pos] *= yuv_color_u;								//左眼 U

			//左眼全尺寸Y混合
			m_mask_left_y[x2   + y2   *m_out_x] *= yuv_color_y;
			m_mask_left_y[x2+1 + y2   *m_out_x] *= yuv_color_y;
			m_mask_left_y[x2   +(y2+1)*m_out_x] *= yuv_color_y;
			m_mask_left_y[x2+1 +(y2+1)*m_out_x] *= yuv_color_y;

			//黑边处左眼，Y
			m_mask_letterbox_left_y[x2   + y2   *m_out_x] = m_mask_left_y[x2   + y2   *m_out_x]/255;
			m_mask_letterbox_left_y[x2+1 + y2   *m_out_x] = m_mask_left_y[x2+1 + y2   *m_out_x]/255;
			m_mask_letterbox_left_y[x2   +(y2+1)*m_out_x] = m_mask_left_y[x2   +(y2+1)*m_out_x]/255;
			m_mask_letterbox_left_y[x2+1 +(y2+1)*m_out_x] = m_mask_left_y[x2+1 +(y2+1)*m_out_x]/255;

			//右眼
			m_mask_right_u[half_pos] =(m_mask_right_y[x2   + y2   *m_out_x] +
									m_mask_right_y[x2+1 + y2   *m_out_x] +
									m_mask_right_y[x2   +(y2+1)*m_out_x] +
									m_mask_right_y[x2+1 +(y2+1)*m_out_x]) /4;

			m_mask_right_v[half_pos] = m_mask_right_u[half_pos] * yuv_color_v;		//右眼 V
			m_mask_right_u[half_pos] *= yuv_color_u;								//右眼 U

			//右眼全尺寸Y混合
			m_mask_right_y[x2   + y2   *m_out_x] *= yuv_color_y;
			m_mask_right_y[x2+1 + y2   *m_out_x] *= yuv_color_y;
			m_mask_right_y[x2   +(y2+1)*m_out_x] *= yuv_color_y;
			m_mask_right_y[x2+1 +(y2+1)*m_out_x] *= yuv_color_y;

			//黑边处右眼，Y
			m_mask_letterbox_right_y[x2   + y2   *m_out_x] = m_mask_right_y[x2   + y2   *m_out_x]/255;
			m_mask_letterbox_right_y[x2+1 + y2   *m_out_x] = m_mask_right_y[x2+1 + y2   *m_out_x]/255;
			m_mask_letterbox_right_y[x2   +(y2+1)*m_out_x] = m_mask_right_y[x2   +(y2+1)*m_out_x]/255;
			m_mask_letterbox_right_y[x2+1 +(y2+1)*m_out_x] = m_mask_right_y[x2+1 +(y2+1)*m_out_x]/255;

			//左眼反色，半尺寸
			m_mask_left_uv_n[half_pos]=(m_mask_left_n[x2   + y2   *m_out_x] +
										m_mask_left_n[x2+1 + y2   *m_out_x] +
										m_mask_left_n[x2   +(y2+1)*m_out_x] +
										m_mask_left_n[x2+1 +(y2+1)*m_out_x]) /4;

			//右眼反色，半尺寸
			m_mask_right_uv_n[half_pos]=(m_mask_right_n[x2   + y2   *m_out_x] +
										m_mask_right_n[x2+1 + y2   *m_out_x] +
										m_mask_right_n[x2   +(y2+1)*m_out_x] +
										m_mask_right_n[x2+1 +(y2+1)*m_out_x]) /4;

			m_mask_letterbox_left_v[half_pos] = (m_mask_left_v[half_pos]+ 128* m_mask_left_uv_n[half_pos]) / 255 ;					//黑边处V
			m_mask_letterbox_left_u[half_pos] = (m_mask_left_u[half_pos]+ 128* m_mask_left_uv_n[half_pos]) / 255 ;					//黑边处U
			m_mask_letterbox_right_v[half_pos] = (m_mask_right_v[half_pos]+ 128*m_mask_right_uv_n[half_pos]) / 255 ;				//黑边处V
			m_mask_letterbox_right_u[half_pos] = (m_mask_right_u[half_pos]+ 128*m_mask_right_uv_n[half_pos]) / 255 ;				//黑边处U
		}
}

void image_processor_stereo::set_mask(unsigned char *mask, int width, int height)			
//仅仅将mask拷入image_processor_stereo并切去上下黑边
//如果这是要渲染的mask，请set_mask_prop()
{
	if(NULL == mask)
	{
		m_nomask = true;
		return;
	}
	else
	{
		m_nomask = false;
	}
	m_mask_width = width;
	m_mask_height = height;
	memcpy(m_mask, mask, min(width*height, m_in_x*m_in_y));
}

void image_processor_stereo::set_mask_prop(int left, int top, int offset, DWORD color)
{
    m_mask_pos_top = top;

	bool need_gen = false;
	if (m_mask_pos_left != left ||
		m_mask_color != color ||
		m_mask_offset != offset)
		need_gen = true;

	m_mask_pos_left = left;
	m_mask_color = color;
	m_mask_offset = offset;

	compute_zones();
	if (need_gen)
		generate_masks();
}

#define safe_delete(p) if(p){delete []p;p=NULL;}
image_processor_stereo::image_processor_stereo()
{
	no_more_malloc = false;

	m_mask_left_y=NULL;					//全尺寸，混合
	m_mask_right_y=NULL;

	m_mask_left_u=NULL;					//半尺寸，混合
	m_mask_left_v=NULL;
	m_mask_right_u=NULL;
	m_mask_right_v=NULL;

	m_mask_left_n=NULL;					//全尺寸，反色
	m_mask_right_n=NULL;

	m_mask_left_uv_n=NULL;				//半尺寸，反色
	m_mask_right_uv_n=NULL;

	m_mask_letterbox_left_y = NULL;
	m_mask_letterbox_left_v = NULL;
	m_mask_letterbox_left_u = NULL;
	m_mask_letterbox_right_y = NULL;
	m_mask_letterbox_right_v = NULL;
	m_mask_letterbox_right_u = NULL;

	m_mask_color = 0x00ff8080;			// AYUV(00,ff,80,80) = (0,255,128,128) = white
	m_mask_offset = 0;
}
void image_processor_stereo::free_masks()
{
	safe_delete(m_mask);

	safe_delete(m_mask_left_y);
	safe_delete(m_mask_right_y);

	safe_delete(m_mask_left_v);
	safe_delete(m_mask_right_v);
	safe_delete(m_mask_left_u);
	safe_delete(m_mask_right_u);


	safe_delete(m_mask_left_n);
	safe_delete(m_mask_right_n);

	safe_delete(m_mask_left_uv_n);
	safe_delete(m_mask_right_uv_n);

	safe_delete(m_mask_letterbox_left_y);
	safe_delete(m_mask_letterbox_left_v);
	safe_delete(m_mask_letterbox_left_u);
	safe_delete(m_mask_letterbox_right_y);
	safe_delete(m_mask_letterbox_right_v);
	safe_delete(m_mask_letterbox_right_u);


}

void image_processor_stereo::malloc_masks()
{
	if(no_more_malloc)
		return;
	no_more_malloc = true;	// malloc only once

	free_masks();

	m_mask = new unsigned char[m_max_mask_buffer];

	m_mask_left_y = new unsigned short[m_max_mask_buffer];
	m_mask_right_y = new unsigned short[m_max_mask_buffer];

	m_mask_left_v = new unsigned short[m_max_mask_buffer/4];
	m_mask_right_v = new unsigned short[m_max_mask_buffer/4];
	m_mask_left_u = new unsigned short[m_max_mask_buffer/4];
	m_mask_right_u = new unsigned short[m_max_mask_buffer/4];


	m_mask_left_n = new unsigned char[m_max_mask_buffer];
	m_mask_right_n = new unsigned char[m_max_mask_buffer];

	m_mask_left_uv_n = new unsigned char[m_max_mask_buffer/4];
	m_mask_right_uv_n = new unsigned char[m_max_mask_buffer/4];

	m_mask_letterbox_left_y= new unsigned char[m_max_mask_buffer];
	m_mask_letterbox_left_v= new unsigned char[m_max_mask_buffer/4];
	m_mask_letterbox_left_u= new unsigned char[m_max_mask_buffer/4];
	m_mask_letterbox_right_y= new unsigned char[m_max_mask_buffer];
	m_mask_letterbox_right_v= new unsigned char[m_max_mask_buffer/4];
	m_mask_letterbox_right_u= new unsigned char[m_max_mask_buffer/4];
}