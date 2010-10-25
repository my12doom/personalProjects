#include <Windows.h>

#ifndef MYSPLITTER_IMAGE_H
#define MYSPLITTER_IMAGE_H

class image_processor
{
public:
	image_processor()
	{
		m_letterbox_top = m_letterbox_bottom = m_mix_zone = 0;

		m_mode = 0;		//YV12_CUT_MODE_AUTO
		m_extend = 0;

		m_mask=NULL;						//原图
		m_max_mask_buffer = 1920*1080;		//默认1920x1080
		m_nomask = true;
		m_mask_height = m_mask_width = m_mask_offset = 0;
	};


protected:
	//vars
	int m_in_x;
	int m_in_y;

	int m_out_x;
	int m_out_y;
	int m_image_x;
	int m_image_y;
	
	int m_letterbox_height;
	int m_letterbox_delta;

	//总体分划5个区
	int m_letterbox_top;			//上部黑边
	int m_free_zone_top;			//上部直接复制区
	int m_mix_zone;					//带字幕区
	int m_free_zone_bottom;			//下部直接复制区
	int m_letterbox_bottom;			//下部黑边


	int m_extend;					//0 = no , 1 = 4:3, 2 = 5:4
	int m_mode;						//0 = left-right, 1 = top-bottom
									//其实会与m_out俩值有些冗余
	
	unsigned char *m_mask;			// source
	int m_max_mask_buffer;
	bool m_nomask;
	int m_mask_offset;
	int m_mask_height;
	int m_mask_width;

	// funcs
	virtual void generate_masks()=0;												//生成left和right，生成uv
	virtual void init_out_size()=0;
	virtual void compute_zones()=0;													// 计算各个区的范围
	virtual void set_mask(unsigned char *mask, int width, int height)=0;
	virtual void set_mask_prop(int left, int top, int offset, DWORD color)=0;
	virtual void free_masks()=0;
	virtual void malloc_masks()=0;
};

class image_processor_stereo : public image_processor
{
public:
	image_processor_stereo();
protected:
	int m_mask_pos_left;
	int m_mask_pos_top;
	DWORD m_mask_color;

	unsigned short *m_mask_left_y;		// cliped, offseted, mixed with mask color
	unsigned short *m_mask_right_y;		// out = input * mask_n(i) + mask(i) * color
										// m_mask(left&right) = mask(i)*color.Y

	unsigned short *m_mask_left_v;		// half size, mixed with V color
	unsigned short *m_mask_right_v;		//
	unsigned short *m_mask_left_u;		// half size, mixed with U color
	unsigned short *m_mask_right_u;		//


	unsigned char *m_mask_left_n;		// negtive
	unsigned char *m_mask_right_n;		//

	unsigned char *m_mask_left_uv_n;	// half size ,negtive
	unsigned char *m_mask_right_uv_n;	//

	unsigned char *m_mask_letterbox_left_y;
	unsigned char *m_mask_letterbox_left_v;
	unsigned char *m_mask_letterbox_left_u;
	unsigned char *m_mask_letterbox_right_y;
	unsigned char *m_mask_letterbox_right_v;
	unsigned char *m_mask_letterbox_right_u;

	bool no_more_malloc;
	//funcs
	void generate_masks();												//生成left和right，生成uv
	void init_out_size();
	void compute_zones();												// 计算各个区的范围
	void set_mask(unsigned char *mask, int width, int height);
	void set_mask_prop(int left, int top, int offset, DWORD color);
	void free_masks();
	void malloc_masks();

};

class image_processor_mono : public image_processor
{
public:
	image_processor_mono();
protected:
	int m_mask_pos_left;
	int m_mask_pos_top;
	DWORD m_mask_color;

	unsigned short *m_mask_y;		// cliped, offseted, mixed with mask color
									// out = input * mask_n(i) + mask(i) * color
									// m_mask(left&right) = mask(i)*color.Y

	unsigned short *m_mask_v;		// half size, mixed with V color
	unsigned short *m_mask_u;		// half size, mixed with U color


	unsigned char *m_mask_n;		// negtive
	unsigned char *m_mask_uv_n;		// half size ,negtive

	unsigned char *m_mask_letterbox_y;
	unsigned char *m_mask_letterbox_v;
	unsigned char *m_mask_letterbox_u;

	bool no_more_malloc;
	//funcs
	void generate_masks();												//生成left和right，生成uv
	void init_out_size();
	void compute_zones();												// 计算各个区的范围
	void set_mask(unsigned char *mask, int width, int height);
	void set_mask_prop(int left, int top, int offset, DWORD color);
	void free_masks();
	void malloc_masks();

};

#endif