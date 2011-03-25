#include "3dvDemuxer.h"

DWORD reverse_byte_order(DWORD input)
{	
	BYTE tmp[4];
	memcpy(tmp, &input, 4);
	BYTE t = tmp[0];
	tmp[0] = tmp[3];
	tmp[3] = t;
	t = tmp[1];
	tmp[1] = tmp[2];
	tmp[2] = t;
	return *(DWORD*)tmp;
}

bool is_valid_fourcc(DWORD fourcc)
{
	BYTE *tmp = (BYTE*)&fourcc;
	for(int i=0; i<4; i++)
		if (tmp[i] < '1' || tmp[i] > 'z')
		{
			return false;
		}
	return true;
}

C3dvDemuxer::C3dvDemuxer(const wchar_t *file):
m_streams(NULL),
m_stream_count(0)
{
	m_file = _wfopen(file, L"rb");
	fseek(m_file, 0, SEEK_END);
	int file_size = ftell(m_file);
	fseek(m_file, 0, SEEK_SET);

	demux_riff(0, file_size);
}

C3dvDemuxer::~C3dvDemuxer()
{
	for(int i=0; i<m_stream_count; i++)
	{
		if (m_streams[i].packet_index)
			free(m_streams[i].packet_index);
		if (m_streams[i].packet_size)
			free(m_streams[i].packet_size);
	}
	if (m_streams)
		free(m_streams);
	if (m_file)
		fclose(m_file);
}

int C3dvDemuxer::demux_riff(int index, int size)
{
	if (size<8)
		return 0;
	while (size >= 8)
	{
		DWORD fourcc;
		DWORD data_size;
		{
			CAutoLock lck(&m_cs);
			fseek(m_file, index, SEEK_SET);
			fread(&data_size, 1, 4, m_file);
			fread(&fourcc, 1, 4, m_file);
		}
		fourcc = reverse_byte_order(fourcc);
		data_size = reverse_byte_order(data_size)-8;

		if (data_size > size-8 || !is_valid_fourcc(fourcc))
			return -1;

		// handle data first
		handle_data(fourcc, index+8, data_size);

		// then handle possible child riff
		if (size >= 16)
		{
			DWORD child_fourcc;
			DWORD child_data_size;
			{
				CAutoLock lck(&m_cs);
				fseek(m_file, index+8, SEEK_SET);
				fread(&child_data_size, 1, 4, m_file);
				fread(&child_fourcc, 1, 4, m_file);
			}
			child_fourcc = reverse_byte_order(child_fourcc);
			child_data_size = reverse_byte_order(child_data_size)-8;
			if (is_valid_fourcc(child_fourcc) && child_data_size <= size-16)
				demux_riff(index+8, data_size);
		}

		// handle next riff
		index += 8+data_size;
		size -= 8+data_size;
	}
	return 0;
}

int C3dvDemuxer::handle_data(DWORD fourcc, int index, int size)
{
	// always read 200 byte data
	BYTE data[200];

	// debuf char
#ifdef _DEBUG
	char cfourcc[4];
	memcpy(cfourcc, &fourcc, 4);
#endif
	{
		CAutoLock lck(&m_cs);
		fseek(m_file, index, SEEK_SET);
		fread(data, 1, min(size, 200), m_file);
	}

	// switch
	stream_3dv &current_track = m_streams[m_stream_count - 1];
	switch(fourcc)
	{
	case 'trak':
		m_stream_count++;
		m_streams = (stream_3dv*)realloc(m_streams, m_stream_count * sizeof(stream_3dv));
		memset(m_streams+m_stream_count-1, 0, sizeof(stream_3dv));
		m_streams[m_stream_count-1].f = m_file;
		m_streams[m_stream_count-1].file_lock = &m_cs;
		break;

	case 'tkhd':
		current_track.length = reverse_byte_order(*(DWORD*)(data+20));
		break;

	case 'mdhd':
		/*
		if (data[14] != 0)
		{
			current_track.type = 1;
			current_track.audio_sample_rate = reverse_byte_order(*(DWORD*)(data+12));
			current_track.audio_sample_count = reverse_byte_order(*(DWORD*)(data+16));
		}
		else
		{
			current_track.type = 0;
			current_track.video_fps = reverse_byte_order(*(DWORD*)(data+12));
			current_track.packet_count = reverse_byte_order(*(DWORD*)(data+16));	// possible not final frame count
		}
		*/
		current_track.time_units = reverse_byte_order(*(DWORD*)(data+12));
		current_track.time_in_units = reverse_byte_order(*(DWORD*)(data+16));
		break;

	case 'vmhd':
		current_track.type = 0;
		/*
		current_track.video_fps_denominator = 1;
		if (current_track.video_fps == 0)
		{
			BYTE tmp3[16];
			double tmp = 23.976;
			float tmp2 = 23.976;
			memcpy(tmp3, &tmp, 8);
			memcpy(tmp3+8, &tmp2, 4);
			current_track.video_fps = 24000;
			current_track.video_fps_denominator = 1001;
		}
		*/
		break;

	case 'smhd':
		current_track.type = 1;
		break;

	case 'stsd':
		if (current_track.type == 1)
		{
			if (data[11] == 0x55)
				current_track.type = 1;
			else
				current_track.type = 2;

			current_track.audio_channel = data[33];
			current_track.audio_bit_depth = data[35];
			current_track.audio_sample_rate = (data[40] << 8) + data[41];

		}
		else if (current_track.type == 0)
		{
			// nothing to do...
		}
		break;

	case 'stsz':
		current_track.CBR = reverse_byte_order(*(DWORD*)(data+4));
		current_track.packet_count = reverse_byte_order(*(DWORD*)(data+8));			// final packet count
		if (!current_track.CBR)
		{
			current_track.packet_size = (DWORD*)malloc(sizeof(DWORD) * current_track.packet_count);
			CAutoLock lck(&m_cs);
			fseek(m_file, index+12, SEEK_SET);
			fread(current_track.packet_size, 1, sizeof(DWORD) * current_track.packet_count, m_file);
			for(int i=0; i<current_track.packet_count; i++)
			{
				current_track.packet_size[i] = reverse_byte_order(current_track.packet_size[i]);
				if (current_track.packet_size[i] > current_track.max_packet_size)
					current_track.max_packet_size = current_track.packet_size[i];
			}
		}
		break;

	case 'stco':
		current_track.packet_index = (DWORD*)malloc(sizeof(DWORD) * current_track.packet_count);
		{
			CAutoLock lck(&m_cs);
			fseek(m_file, index+8, SEEK_SET);
			fread(current_track.packet_index, 1, sizeof(DWORD) * current_track.packet_count, m_file);
		}
		for(int i=0; i<current_track.packet_count; i++)
			current_track.packet_index[i] = reverse_byte_order(current_track.packet_index[i]);
		break;

	case 'stss':
		current_track.keyframe_count = reverse_byte_order(*(DWORD*)(data+4));
		current_track.keyframes = (DWORD*)malloc(sizeof(DWORD) * current_track.keyframe_count);
		{
			CAutoLock lck(&m_cs);
			fseek(m_file, index+8, SEEK_SET);
			fread(current_track.keyframes, 1, sizeof(DWORD) * current_track.keyframe_count, m_file);
		}
		for(int i=0; i<current_track.keyframe_count; i++)
		{
			current_track.keyframes[i] = reverse_byte_order(current_track.keyframes[i]) - 1;
			printf("keyframe %d: %d\n", i, current_track.keyframes[i]);
		}
		printf("keyframe count %d\n", current_track.keyframe_count);
		break;
	}


	return 0;
}