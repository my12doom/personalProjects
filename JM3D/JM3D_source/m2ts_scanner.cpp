#include <math.h>
#include "..\h264_parse\mpeg4ip.h"
#include "..\h264_parse\mpeg4ip_bitstream.h"
#include "..\h264_parse\mp4av_h264.h"
#include "m2ts_scanner.h"

CFileBuffer *scanner_buffer = NULL;
int width = 0;
int height = 0;

DWORD WINAPI h264_parse_thread(LPVOID lpvParam);
static uint32_t remove_03 (uint8_t *bptr, uint32_t len);
uint8_t h264_parse_nal (h264_decode_t *dec, CBitstream *bs);
void h264_parse_sequence_parameter_set (h264_decode_t *dec, CBitstream *bs);
uint32_t h264_find_next_start_code (uint8_t *pBuf, uint32_t bufLen);

// fast printf block...
void myprintf(char *format, ...)
{
	char para[512];
	va_list valist;

	va_start(valist,para);
	// null output
	va_end (valist);
}
h264_scan_result scan_m2ts(const char *name)
{
	ts::demuxer demuxer;
	h264_scan_result result;
	memset(&result, 0, sizeof(result));

	// init
	scanner_buffer = new CFileBuffer(1024000);
	demuxer.type_todemux = 0x1b;	// type h264
	demuxer.av_only = false;
	demuxer.silence = true;
	demuxer.parse_only = true;
	demuxer.demux_target = scanner_buffer;

	// create parse thread first
	HANDLE h_h264_parse_thread = CreateThread(NULL, NULL, h264_parse_thread, NULL, NULL, NULL);
	// start scan file(it feed parse thread data)
	if (demuxer.fast_scan_file(name))
		result.error = true;
	scanner_buffer->no_more_data = true;

	WaitForSingleObject(h_h264_parse_thread, INFINITE);

	for(std::map<u_int16_t,ts::stream>::const_iterator i=demuxer.streams.begin();i!=demuxer.streams.end();++i)
	{
		const ts::stream& s=i->second;

		// find the h264 pid
		if(s.type==0x1b)
		{
			printf("[JM3DSource info] possible left eye pid:%4x\n", i->first);
			result.frame_length = s.frame_length;
			u_int64_t end = s.last_pts + s.frame_length;
			result.length = end - s.first_pts;
			if (!result.possible_left_pid)
				result.possible_left_pid = i->first;

		}

		// MVC right eye stream
		if (s.type == 0x20)
		{
			printf("[JM3DSource info] possible right eye pid:%4x\n", i->first);
			if (!result.possible_right_pid)
				result.possible_right_pid = i->first;
		}
	}

	result.width = width;
	result.height = height;

	delete scanner_buffer;
	scanner_buffer = NULL;

	// post process
	result.fps_denominator = result.frame_length;
	result.fps_numerator = 90000;
	if (result.frame_length == 3753 || result.frame_length == 3754)
	{
		// 23.976
		result.frame_length = 3754;
		result.fps_numerator = 24000;
		result.fps_denominator = 1001;
	}

	if (result.frame_length == 1501 || result.frame_length == 1502)
	{
		// 59.94
		result.frame_length = 1501;
		result.fps_numerator = 60000;
		result.fps_denominator = 1001;
	}

	if (result.frame_length)
		result.frame_count = (uint32_t)((double)result.length / result.frame_length + 1.5);

	return result;
}

// copy from h264_pase main.cpp
DWORD WINAPI h264_parse_thread(LPVOID lpvParam)
{
	const MAX_BUFFER = 65536*8;
	uint8_t *buffer = new uint8_t[MAX_BUFFER];
	uint32_t buffer_on, buffer_size;
	uint64_t bytes = 0;
	h264_decode_t dec;
	bool have_prevdec = false;
	memset(&dec, 0, sizeof(dec));

	buffer_on = buffer_size = 0;

	while (!(scanner_buffer->no_more_data && scanner_buffer->m_data_size == 0)) 
	{
		bytes += buffer_on;
		if (buffer_on != 0) 
		{
			buffer_on = buffer_size - buffer_on;
			memmove(buffer, &buffer[buffer_size - buffer_on], buffer_on);
		}
		buffer_size = scanner_buffer->block_remove(MAX_BUFFER - buffer_on, buffer + buffer_on);
		if (buffer_size == 0)
			break;		// eof
		buffer_size += buffer_on;
		buffer_on = 0;

		bool done = false;
		CBitstream ourbs;
		do {
			uint32_t ret;
			ret = h264_find_next_start_code(buffer + buffer_on, 
				buffer_size - buffer_on);
			if (ret == 0) {
				done = true;
			} else {

				// have a complete NAL from buffer_on to end
				if (ret > 3) {
					uint32_t nal_len;

					nal_len = remove_03(buffer + buffer_on, ret);

					ourbs.init(buffer + buffer_on, nal_len * 8);
					uint8_t type;
					type = h264_parse_nal(&dec, &ourbs);

					// my12doom : deleted a lot of codes
					// direct return on SPS
					if (type == H264_NAL_TYPE_SEQ_PARAM)
						goto width_height_got;
				}

				buffer_on += ret; // buffer_on points to next code
			}
		} while (done == false);
	}

width_height_got:
	
	delete buffer;

	scanner_buffer->no_more_remove = true;
	return 0;
}


static uint32_t remove_03 (uint8_t *bptr, uint32_t len)
{
	uint32_t nal_len = 0;

	while (nal_len + 2 < len) {
		if (bptr[0] == 0 && bptr[1] == 0 && bptr[2] == 3) {
			bptr += 2;
			nal_len += 2;
			len--;
			memmove(bptr, bptr + 1, len - nal_len);
		} else {
			bptr++;
			nal_len++;
		}
	}
	return len;
}

void h264_check_0s (CBitstream *bs, int count)
{
	uint32_t val;
	val = bs->GetBits(count);
	if (val != 0) {
		myprintf("field error - %d bits should be 0 is %x\n", 
			count, val);
	}
}
uint8_t h264_parse_nal (h264_decode_t *dec, CBitstream *bs)
{
	uint8_t type = 0;

	if (bs->GetBits(24) == 0) bs->GetBits(8);
	//h264_check_0s(bs, 1);
	bs->GetBits(1);
	dec->nal_ref_idc = bs->GetBits(2);
	dec->nal_unit_type = type = bs->GetBits(5);
	if (type == H264_NAL_TYPE_SEQ_PARAM) 
	{
		h264_parse_sequence_parameter_set(dec, bs);
	}
	else
	{}

	return type;
}


static uint8_t exp_golomb_bits[256] = {
	8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 3, 
		3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
		1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 
};

uint32_t h264_find_next_start_code (uint8_t *pBuf, uint32_t bufLen)
{
	uint32_t val;
	uint32_t offset;

	offset = 0;
	if (pBuf[0] == 0 && pBuf[1] == 0 && pBuf[2] == 0 && pBuf[3] == 1) {
		pBuf += 4;
		offset = 4;
	} else if (pBuf[0] == 0 && pBuf[1] == 0 && pBuf[2] == 1) {
		pBuf += 3;
		offset = 3;
	}
	val = 0xffffffff;
	while (offset < bufLen - 3) {
		val <<= 8;
		val |= *pBuf++;
		offset++;
		if (val == H264_START_CODE) {
			return offset - 4;
		}
		if ((val & 0x00ffffff) == H264_START_CODE) {
			return offset - 3;
		}
	}
	return 0;
}

uint32_t h264_ue (CBitstream *bs)
{
	uint32_t bits, read;
	int bits_left;
	uint8_t coded;
	bool done = false;
	bits = 0;
	// we want to read 8 bits at a time - if we don't have 8 bits, 
	// read what's left, and shift.  The exp_golomb_bits calc remains the
	// same.
	while (done == false) {
		bits_left = bs->bits_remain();
		if (bits_left < 8) {
			read = bs->PeekBits(bits_left) << (8 - bits_left);
			done = true;
		} else {
			read = bs->PeekBits(8);
			if (read == 0) {
				bs->GetBits(8);
				bits += 8;
			} else {
				done = true;
			}
		}
	}
	coded = exp_golomb_bits[read];
	bs->GetBits(coded);
	bits += coded;

	//  myprintf("ue - bits %d\n", bits);
	return bs->GetBits(bits + 1) - 1;
}

int32_t h264_se (CBitstream *bs) 
{
	uint32_t ret;
	ret = h264_ue(bs);
	if ((ret & 0x1) == 0) {
		ret >>= 1;
		int32_t temp = 0 - ret;
		return temp;
	} 
	return (ret + 1) >> 1;
}

void h264_hrd_parameters (h264_decode_t *dec, CBitstream *bs)
{
	uint32_t cpb_cnt;
	dec->cpb_cnt_minus1 = cpb_cnt = h264_ue(bs);
	uint32_t temp;
	myprintf("     cpb_cnt_minus1: %u\n", cpb_cnt);
	myprintf("     bit_rate_scale: %u\n", bs->GetBits(4));
	myprintf("     cpb_size_scale: %u\n", bs->GetBits(4));
	for (uint32_t ix = 0; ix <= cpb_cnt; ix++) {
		myprintf("      bit_rate_value_minus1[%u]: %u\n", ix, h264_ue(bs));
		myprintf("      cpb_size_value_minus1[%u]: %u\n", ix, h264_ue(bs));
		myprintf("      cbr_flag[%u]: %u\n", ix, bs->GetBits(1));
	}
	temp = dec->initial_cpb_removal_delay_length_minus1 = bs->GetBits(5);
	myprintf("     initial_cpb_removal_delay_length_minus1: %u\n", temp);

	dec->cpb_removal_delay_length_minus1 = temp = bs->GetBits(5);
	myprintf("     cpb_removal_delay_length_minus1: %u\n", temp);
	dec->dpb_output_delay_length_minus1 = temp = bs->GetBits(5);
	myprintf("     dpb_output_delay_length_minus1: %u\n", temp);
	dec->time_offset_length = temp = bs->GetBits(5);  
	myprintf("     time_offset_length: %u\n", temp);
}

void h264_vui_parameters (h264_decode_t *dec, CBitstream *bs)
{
	uint32_t temp, temp2;
	temp = bs->GetBits(1);
	myprintf("    aspect_ratio_info_present_flag: %u\n", temp);
	if (temp) {
		temp = bs->GetBits(8);
		myprintf("     aspect_ratio_idc:%u\n", temp);
		if (temp == 0xff) { // extended_SAR
			myprintf("      sar_width: %u\n", bs->GetBits(16));
			myprintf("      sar_height: %u\n", bs->GetBits(16));
		}
	}
	temp = bs->GetBits(1);
	myprintf("    overscan_info_present_flag: %u\n", temp);
	if (temp) {
		myprintf("     overscan_appropriate_flag: %u\n", bs->GetBits(1));
	}
	temp = bs->GetBits(1);
	myprintf("    video_signal_info_present_flag: %u\n", temp);
	if (temp) {
		myprintf("     video_format: %u\n", bs->GetBits(3));
		myprintf("     video_full_range_flag: %u\n", bs->GetBits(1));
		temp = bs->GetBits(1);
		myprintf("     colour_description_present_flag: %u\n", temp);
		if (temp) {
			myprintf("      colour_primaries: %u\n", bs->GetBits(8));
			myprintf("      transfer_characteristics: %u\n", bs->GetBits(8));
			myprintf("      matrix_coefficients: %u\n", bs->GetBits(8));
		}
	}

	temp = bs->GetBits(1);
	myprintf("    chroma_loc_info_present_flag: %u\n", temp);
	if (temp) {
		myprintf("     chroma_sample_loc_type_top_field: %u\n", h264_ue(bs));
		myprintf("     chroma_sample_loc_type_bottom_field: %u\n", h264_ue(bs));
	}
	temp = bs->GetBits(1);
	myprintf("    timing_info_present_flag: %u\n", temp);
	if (temp) {
		myprintf("     num_units_in_tick: %u\n", bs->GetBits(32));
		myprintf("     time_scale: %u\n", bs->GetBits(32));
		myprintf("     fixed_frame_scale: %u\n", bs->GetBits(1));
	}
	temp = bs->GetBits(1);
	myprintf("    nal_hrd_parameters_present_flag: %u\n", temp);
	if (temp) {
		dec->NalHrdBpPresentFlag = 1;
		dec->CpbDpbDelaysPresentFlag = 1;
		h264_hrd_parameters(dec, bs);
	}
	temp2 = bs->GetBits(1);
	myprintf("    vcl_hrd_parameters_present_flag: %u\n", temp2);
	if (temp2) {
		dec->VclHrdBpPresentFlag = 1;
		dec->CpbDpbDelaysPresentFlag = 1;
		h264_hrd_parameters(dec, bs);
	}
	if (temp || temp2) {
		myprintf("    low_delay_hrd_flag: %u\n", bs->GetBits(1));
	}
	dec->pic_struct_present_flag = temp = bs->GetBits(1);
	myprintf("    pic_struct_present_flag: %u\n", temp);
	temp = bs->GetBits(1);
	if (temp) {
		myprintf("    motion_vectors_over_pic_boundaries_flag: %u\n", bs->GetBits(1));
		myprintf("    max_bytes_per_pic_denom: %u\n", h264_ue(bs));
		myprintf("    max_bits_per_mb_denom: %u\n", h264_ue(bs));
		myprintf("    log2_max_mv_length_horizontal: %u\n", h264_ue(bs));
		myprintf("    log2_max_mv_length_vertical: %u\n", h264_ue(bs));
		myprintf("    num_reorder_frames: %u\n", h264_ue(bs));
		myprintf("     max_dec_frame_buffering: %u\n", h264_ue(bs));
	}
}

static void scaling_list (uint ix, uint sizeOfScalingList, CBitstream *bs)
{
	uint lastScale = 8, nextScale = 8;
	uint jx;
	int deltaScale;

	for (jx = 0; jx < sizeOfScalingList; jx++) {
		if (nextScale != 0) {
			deltaScale = h264_se(bs);
			nextScale = (lastScale + deltaScale + 256) % 256;
			myprintf("     delta: %d\n", deltaScale);
		}
		if (nextScale == 0) {
			lastScale = lastScale;
		} else {
			lastScale = nextScale;
		}
		myprintf("     scaling list[%u][%u]: %u\n", ix, jx, lastScale);

	}
}
void h264_parse_sequence_parameter_set (h264_decode_t *dec, CBitstream *bs)
{
	uint32_t temp;
	dec->profile = bs->GetBits(8);
	myprintf("   profile: %u\n", dec->profile);
	myprintf("   constaint_set0_flag: %d\n", bs->GetBits(1));
	myprintf("   constaint_set1_flag: %d\n", bs->GetBits(1));
	myprintf("   constaint_set2_flag: %d\n", bs->GetBits(1));
	myprintf("   constaint_set3_flag: %d\n", bs->GetBits(1));
	h264_check_0s(bs, 4);
	myprintf("   level_idc: %u\n", bs->GetBits(8));
	myprintf("   seq parameter set id: %u\n", h264_ue(bs));
	if (dec->profile == 100 || dec->profile == 110 ||
		dec->profile == 122 || dec->profile == 144) {
			dec->chroma_format_idc = h264_ue(bs);
			myprintf("   chroma format idx: %u\n", dec->chroma_format_idc);

			if (dec->chroma_format_idc == 3) {
				dec->residual_colour_transform_flag = bs->GetBits(1);
				myprintf("    resigual colour transform flag: %u\n", dec->residual_colour_transform_flag);
			}
			dec->bit_depth_luma_minus8 = h264_ue(bs);
			myprintf("   bit depth luma minus8: %u\n", dec->bit_depth_luma_minus8);
			dec->bit_depth_chroma_minus8 = h264_ue(bs);
			myprintf("   bit depth chroma minus8: %u\n", dec->bit_depth_luma_minus8);
			dec->qpprime_y_zero_transform_bypass_flag = bs->GetBits(1);
			myprintf("   Qpprime Y Zero Transform Bypass flag: %u\n", 
				dec->qpprime_y_zero_transform_bypass_flag);
			dec->seq_scaling_matrix_present_flag = bs->GetBits(1);
			myprintf("   Seq Scaling Matrix Present Flag: %u\n", 
				dec->seq_scaling_matrix_present_flag);
			if (dec->seq_scaling_matrix_present_flag) {
				for (uint ix = 0; ix < 8; ix++) {
					temp = bs->GetBits(1);
					myprintf("   Seq Scaling List[%u] Present Flag: %u\n", ix, temp); 
					if (temp) {
						scaling_list(ix, ix < 6 ? 16 : 64, bs);
					}
				}
			}

		}

		dec->log2_max_frame_num_minus4 = h264_ue(bs);
		myprintf("   log2_max_frame_num_minus4: %u\n", dec->log2_max_frame_num_minus4);
		dec->pic_order_cnt_type = h264_ue(bs);
		myprintf("   pic_order_cnt_type: %u\n", dec->pic_order_cnt_type);
		if (dec->pic_order_cnt_type == 0) {
			dec->log2_max_pic_order_cnt_lsb_minus4 = h264_ue(bs);
			myprintf("    log2_max_pic_order_cnt_lsb_minus4: %u\n", 
				dec->log2_max_pic_order_cnt_lsb_minus4);
		} else if (dec->pic_order_cnt_type == 1) {
			dec->delta_pic_order_always_zero_flag = bs->GetBits(1);
			myprintf("    delta_pic_order_always_zero_flag: %u\n", 
				dec->delta_pic_order_always_zero_flag);
			myprintf("    offset_for_non_ref_pic: %d\n", h264_se(bs));
			myprintf("    offset_for_top_to_bottom_field: %d\n", h264_se(bs));
			temp = h264_ue(bs);
			for (uint32_t ix = 0; ix < temp; ix++) {
				myprintf("      offset_for_ref_frame[%u]: %d\n",
					ix, h264_se(bs));
			}
		}
		myprintf("   num_ref_frames: %u\n", h264_ue(bs));
		myprintf("   gaps_in_frame_num_value_allowed_flag: %u\n", bs->GetBits(1));
		uint32_t PicWidthInMbs = h264_ue(bs) + 1;

		myprintf("   pic_width_in_mbs_minus1: %u (%u)\n", PicWidthInMbs - 1, 
			PicWidthInMbs * 16);
		uint32_t PicHeightInMapUnits = h264_ue(bs) + 1;

		myprintf("   pic_height_in_map_minus1: %u\n", 
			PicHeightInMapUnits - 1);
		dec->frame_mbs_only_flag = bs->GetBits(1);
		myprintf("   frame_mbs_only_flag: %u\n", dec->frame_mbs_only_flag);
		myprintf("     derived height: %u\n", (2 - dec->frame_mbs_only_flag) * PicHeightInMapUnits * 16);

		// my12doom
		width = PicWidthInMbs * 16;
		height = (2 - dec->frame_mbs_only_flag) * PicHeightInMapUnits * 16;

		if (!dec->frame_mbs_only_flag) {
			myprintf("    mb_adaptive_frame_field_flag: %u\n", bs->GetBits(1));
		}
		myprintf("   direct_8x8_inference_flag: %u\n", bs->GetBits(1));
		temp = bs->GetBits(1);
		myprintf("   frame_cropping_flag: %u\n", temp);
		if (temp) {
			// my12doom
			uint32_t crop_left = h264_ue(bs);
			uint32_t crop_right = h264_ue(bs);
			uint32_t crop_top = h264_ue(bs);
			uint32_t crop_bottom = h264_ue(bs);
			width -= (crop_left + crop_right)*2;
			height -= (crop_top + crop_bottom)*2;

			myprintf("     frame_crop_left_offset: %u\n", crop_left);
			myprintf("     frame_crop_right_offset: %u\n", crop_right);
			myprintf("     frame_crop_top_offset: %u\n", crop_top);
			myprintf("     frame_crop_bottom_offset: %u\n", crop_bottom);
		}
		temp = bs->GetBits(1);
		myprintf("   vui_parameters_present_flag: %u\n", temp);
		if (temp) {
			h264_vui_parameters(dec, bs);
		}
}
