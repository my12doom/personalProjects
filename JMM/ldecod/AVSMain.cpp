#include <stdio.h>
#include <windows.h>
#include <Shlwapi.h>
#include "AVSMain.h"
#include "readmpls.h"
#include "ts_stream.h"
#include "..\resource.h"

#include "..\..\my12doom_revision.h"

#pragma comment (lib, "Shlwapi.lib")

// ldecod decoding
extern "C"
{
#include "win32.h"
#include "h264decoder.h"
#include "configfile.h"
};

float buffer_load = 0;
JMAvs *avs = NULL;
DWORD WINAPI decoding_thread(LPVOID p);

void * create_avs()
{
	return avs;
}

int close_avs(void *pavs)		// just set no_more_data = 1
{
	JMAvs * avs = (JMAvs *) pavs;

	avs->m_decoding_done = true;

	return 0;
}

int view_count(void *pavs)
{
	JMAvs * avs = (JMAvs *) pavs;
	return avs->right_buffer == NULL ? 1 : 2;
}

int insert_offset_metadata(void *avs, BYTE *data, int count)
{
	if (!avs)
		return 0;

	JMAvs *p = (JMAvs*)avs;
	p->insert_offset(data, count);
	return 0;
}

int set_avs_resolution(void *pavs, int width, int height, int fps, int fpsdenumorator)
{
	JMAvs * avs = (JMAvs *) pavs;

	avs->ldecod_init(width, height, fps, fpsdenumorator);

	return 0;
}

int insert_frame(void *pavs, void **pY, void **pV, void **pU, int view_id, int poc)
{
	JMAvs * avs = (JMAvs *) pavs;

	if (avs->m_width == 0 || avs->m_height == 0)
		return 0;

	CFrameBuffer *buffer = view_id == 0 ? avs->left_buffer : avs->right_buffer;

	if (buffer == NULL)
	{
		printf("Warning...inserting to NULL buffer\n");
		return 0;
	}

	buffer->insert_no_number((const BYTE**)pY, (const BYTE**)pV, (const BYTE**)pU);

	int total = avs->left_buffer->m_unit_count + (avs->right_buffer ? avs->right_buffer->m_unit_count : 0);
	int current = avs->left_buffer->m_item_count + (avs->right_buffer ? avs->right_buffer->m_item_count : 0);

	buffer_load = (float)current/total;

	return 0;
}



CFrameBuffer::CFrameBuffer()
{
	CAutoLock lck(&cs);
	m_width = 0;
	m_height = 0;
	m_unit_count = 0;
	the_buffer = NULL;

	m_frame_count = 0;
	m_max_fn_recieved = -1;
	m_item_count = 0;
	m_discard_all = false;
}

CFrameBuffer::~CFrameBuffer()
{
	if (the_buffer)
	{
		for(int i=0; i<m_unit_count; i++)
			delete [] the_buffer[i].data;

		delete [] the_buffer;
	}
}

int CFrameBuffer::init(int width, int height, int buffer_unit_count, int frame_count)
{
	CAutoLock lck(&cs);

	m_width = width;
	m_height = height;
	m_unit_count = buffer_unit_count;
	m_frame_count = frame_count;

	the_buffer = new buffer_unit[m_unit_count];
	for(int i=0; i<m_unit_count; i++)
	{
		the_buffer[i].frame_number = -1;
		the_buffer[i].data = new BYTE[m_width * m_height * 3 / 2];
	}

	return 0;
}

int CFrameBuffer::insert(int n, const BYTE*buf)
{
	int id = -1;
	while(true)
	{
		if (m_discard_all)
			return -1;

		cs.Lock();
		for(int i=0; i<m_unit_count; i++)
		{
			if (the_buffer[i].frame_number == -1)
			{
				id = i;
				goto wait_ok;		// break outside, cs still locked
			}
		}
		cs.Unlock();
		Sleep(10);
	}

wait_ok:
	// insert
	memcpy(the_buffer[id].data, buf, m_width * m_height * 3 / 2);
	the_buffer[id].frame_number = n;
	if(n>m_max_fn_recieved) m_max_fn_recieved = n;
	m_item_count ++;
	cs.Unlock();
	return id;
}
int CFrameBuffer::insert(int n, const BYTE **pY, const BYTE **pV, const BYTE **pU)
{
	int id = -1;
	while(true)
	{
		if (m_discard_all)
			return -1;

		cs.Lock();
		for(int i=0; i<m_unit_count; i++)
		{
			if (the_buffer[i].frame_number == -1)
			{
				id = i;
				goto wait_ok;		// break outside, cs still locked
			}
		}
		cs.Unlock();
		Sleep(10);
	}

wait_ok:
	buffer_unit &b = the_buffer[id];
	b.frame_number = n;
	m_max_fn_recieved = max(m_max_fn_recieved, n);
	m_item_count ++;

	BYTE *dstY = b.data;
	BYTE *dstV = b.data + m_width * m_height;
	BYTE *dstU = b.data + m_width * m_height * 5 / 4;

	for(int y=0; y<m_height; y++)
	{
		memcpy(dstY, pY[y], m_width);
		dstY += m_width;
	}
	for(int y=0; y<m_height/2; y++)
	{
		memcpy(dstV, pV[y], m_width/2);
		dstV += m_width/2;
	}
	for(int y=0; y<m_height/2; y++)
	{
		memcpy(dstU, pU[y], m_width/2);
		dstU += m_width/2;
	}
	cs.Unlock();

	return id;
}

int CFrameBuffer::insert_no_number(const BYTE **pY, const BYTE **pV, const BYTE **pU)
{
	int id = -1;
	while(true)
	{
		if (m_discard_all)
			return -1;

		cs.Lock();
		for(int i=0; i<m_unit_count; i++)
		{
			if (the_buffer[i].frame_number == -1)
			{
				id = i;
				goto wait_ok;		// break outside, cs still locked
			}
		}
		cs.Unlock();
		Sleep(10);
	}

wait_ok:
	int n = m_max_fn_recieved + 1;
	buffer_unit &b = the_buffer[id];
	b.frame_number = n;
	m_max_fn_recieved = max(m_max_fn_recieved, n);
	m_item_count ++;

	BYTE *dstY = b.data;
	BYTE *dstV = b.data + m_width * m_height;
	BYTE *dstU = b.data + m_width * m_height * 5 / 4;

	for(int y=0; y<m_height; y++)
	{
		memcpy(dstY, pY[y], m_width);
		dstY += m_width;
	}
	for(int y=0; y<m_height/2; y++)
	{
		memcpy(dstV, pV[y], m_width/2);
		dstV += m_width/2;
	}
	for(int y=0; y<m_height/2; y++)
	{
		memcpy(dstU, pU[y], m_width/2);
		dstU += m_width/2;
	}
	cs.Unlock();

	return id;
}

int CFrameBuffer::remove(int n, BYTE *buf, int stride, int offset)
{
	// find direct match;
	cs.Lock();
	for(int i=0; i<m_unit_count; i++)
	{
		if (the_buffer[i].frame_number == n)
		{
			// copy Y
			BYTE *src = the_buffer[i].data;
			BYTE *dst = buf + offset;
			for(int j=0; j<m_height; j++)
			{
				memcpy(dst, src, m_width);
				src += m_width;
				dst += stride;
			}

			// copy UV
			dst = buf + stride * m_height + offset/2;
			for(int j=0; j<m_height; j++)
			{
				memcpy(dst, src, m_width/2);
				src += m_width/2;
				dst += stride/2;
			}

			//cs.Lock();
			the_buffer[i].frame_number = -1;
			m_item_count --;
			cs.Unlock();
			return n;
		}
	}

	// decide return -1 or -2
	int max_possible_n = min(m_max_fn_recieved + m_unit_count - m_item_count - 1, m_frame_count);
	int min_possible_n = m_max_fn_recieved - m_item_count + 1;
	cs.Unlock();

	if(n>max_possible_n || n<min_possible_n)
	{
		// no wait
		return -1;
	}
	else
	{
		// should wait and retry
		return -2;
	}
}

const char* get_filename(const char*pathname)
{
	const char *p = pathname + strlen(pathname) - 1;
	while (*p != '\\' && p>pathname)
		p--;
	return p==pathname?p:p+1;
}

AVSValue __cdecl Create_JM3DSource(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	if (avs != NULL)
	{
		env->ThrowError("Currently only 1 instance per process allowed.");
		return NULL;
	}

	avs = new JMAvs();

	// get paras
	char tmp[40960];
	char tmp1[40960];
	char tmp2[40960];
	const char *file1 = args[0].AsString("");
	const char *file2 = args[1].AsString("");
	int frame_count = args[2].AsInt(-1);
	int buffer_count = args[3].AsInt(10);
	const char *ldecod_args = args[4].AsString("");
	const bool swap_eyes = args[5].AsBool(false);
	char playlist[40960] = {0};

	// check para and input files
	if (file1[0] == NULL)
		env->ThrowError("must have a input file.");

	// check for folder
	if(PathIsDirectoryA(file1))
	{
		HRESULT hr;
		if (FAILED(hr = find_main_movie(file1, playlist)))
			env->ThrowError("Couldn't find main movie for '%s'", file1);
		else
		{
			printf("Found main movie: %s\n", playlist);
			file1 = playlist;
		}
	}


	// check for mpls
	int main_playlist_count = 0;
	int sub_playlist_count = 0;
	char main_playlist[40960];
	char sub_playlist[40960];
	int lengths[40960] = {0};
	if (file2[0] == NULL && 0 == scan_mpls(file1, &main_playlist_count, main_playlist, lengths, sub_playlist, &sub_playlist_count))
	{
		if (main_playlist_count != sub_playlist_count)
		{
			if (sub_playlist_count != 0)
				env->ThrowError("Invalid playlist file, main count(%d) != sub count(%d).", main_playlist_count, sub_playlist_count);
			else
				printf("2D MPLS file detected.\n");
		}
		else
		{
			printf("3D MPLS file detected.\n");
		}

		// find path
		int path_pos = 0;
		for(int i=strlen(file1); i>0; i--)
		{
			if (file1[i] == '\\')
			{
				if (path_pos)
				{
					path_pos = i+1;
					break;
				}
				else
					path_pos = i+1;
			}
		}
		memcpy(tmp1, file1, path_pos);
		tmp1[path_pos] = '\0';
		strcat(tmp1, "STREAM\\");

		tmp2[0] = NULL;
		if (sub_playlist_count>0)
			strcpy(tmp2, tmp1);

		for(int i=0; i<main_playlist_count; i++)
		{
			strcat(tmp1, main_playlist+6*i);
			strcat(tmp1, i==main_playlist_count-1?".m2ts":".m2ts:");
			if (sub_playlist_count>0)
			{
				strcat(tmp2, sub_playlist+6*i);
				strcat(tmp2, i==main_playlist_count-1?".m2ts":".m2ts:");
			}
		}

		file1 = tmp1;
		file2 = tmp2;

		printf("left  eye:%s.\n", file1);
		if (sub_playlist_count>0)
			printf("right eye:%s.\n", file2);
	}

	parse_combined_filename(file1, 0, tmp);
	h264_scan_result scan_result = scan_m2ts(tmp);
	if (scan_result.frame_count == 0 && frame_count == -1)
		env->ThrowError("can't detect frame count of input file (%s).", tmp);

	// recalculate frame count if it is a valid mpls
	if (main_playlist_count > 0 && lengths[0] > 0)
	{
		int total_length = 0;
		for(int i=0; i<main_playlist_count; i++)
			total_length += lengths[i];

		scan_result.frame_count = (LONGLONG)total_length * scan_result.fps_numerator / scan_result.fps_denominator / 1000;

	}

	// avs init
	avs->avs_init(file1, env, file2, playlist[0] ? playlist : file1, frame_count == -1 ? scan_result.frame_count : frame_count,
		buffer_count, scan_result.fps_numerator, scan_result.fps_denominator, swap_eyes);

	// start decoding thread
	avs->m_decoding_thread = CreateThread(NULL, NULL, decoding_thread, avs, NULL, NULL);

	// wait for ldecod init
	while (avs->left_buffer == NULL || avs->right_buffer == NULL)
		Sleep(10);

	// wait for buffer fill
	while(avs->left_buffer->m_item_count < buffer_count && avs->right_buffer->m_item_count == 0)
		Sleep(10);

	// delete right buffer if it is empty (2D)
	// and modify width if is 3D
	if (avs->right_buffer->m_item_count == 0)
	{
		delete avs->right_buffer;
		avs->right_buffer = NULL;
	}
	else
	{
		avs->vi.width *= 2;
	}

	// check for invalid FPS
	if (avs->vi.fps_denominator == 0)
	{
		if (avs->vi.width == 1280 || avs->vi.width == 2560)
		{
			avs->vi.fps_numerator = 60000;
			avs->vi.fps_denominator = 1001;
		}
		else
		{
			avs->vi.fps_numerator = 24000;
			avs->vi.fps_denominator = 1001;
		}
	}

	return avs;
}

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env)
{
	printf("ldecod for avisynth rev %d.\n", my12doom_rev);
	env->AddFunction("ldecod","[file1]s[file2]s[frame_count]i[buffer_count]i[ldecod_args]s[swap]b",Create_JM3DSource,0);
	env->SetMemoryMax(32);
	return 0;
}


JMAvs::JMAvs()
{
	memset(&vi, 0, sizeof(vi));
	m_offset_file = NULL;
	m_frame_count = 0;
	m_width = 0;
	m_height = 0;
	m_buffer_unit_count = 0;
	m_m2ts_left[0] = '\0';
	m_m2ts_right[0] = '\0';

	memcpy(&m_header.file_header, "offs", 4);
	m_header.version = 1;
	m_header.point_count = 0;


	left_buffer = NULL;
	right_buffer = NULL;

	m_decoding_done = false;

	m_decoding_thread = INVALID_HANDLE_VALUE;
}

extern "C" void flush_output_queue();

JMAvs::~JMAvs()
{
	if (left_buffer)
		left_buffer->m_discard_all = true;

	if (right_buffer)
		right_buffer->m_discard_all = true;

	TerminateThread(m_decoding_thread, 0);
	flush_output_queue();

	if (left_buffer)
		delete left_buffer;

	if (right_buffer)
		delete right_buffer;
}

int JMAvs::avs_init(const char*m2ts_left, IScriptEnvironment* env, const char*m2ts_right /* = NULL */,
					const char*offset_out, const int frame_count /* = -1 */, int buffer_count /* = 10 */, int fps_numerator /* = 24000 */, int fps_denominator /* = 1001 */, bool swapeyes /* = false */)
{
	strcpy(m_m2ts_left, m2ts_left);
	strcpy(m_m2ts_right, m2ts_right);

	char offset_file[40960];
	strcpy(offset_file, get_filename(offset_out));
	for (int i=0; i<strlen(offset_file); i++)
		if (offset_file[i] == ':')
			offset_file[i] = NULL;
	strcat(offset_file, ".offset");
	printf("Opening file %s for offset metadata output...", offset_file);
	FILE * f = fopen(offset_file, "rb");
	if (f)
	{
		printf("Already exist\n");
		fclose(f);
	}
	else
	{
	m_offset_file = fopen(offset_file, "wb");
	printf("%s\n", m_offset_file ? "OK" : "FAILED");
	}


	m_frame_count = frame_count;
	m_buffer_unit_count = buffer_count;
	m_swap_eyes = swapeyes;
	vi.fps_numerator = fps_numerator;
	vi.fps_denominator = fps_denominator;
	vi.num_frames = frame_count;
	vi.pixel_type = VideoInfo::CS_YV12;

	return 0;
}

int JMAvs::ldecod_init(int width, int height, int fps, int fpsdenumorator)
{
	m_width = width;
	m_height = height;

	left_buffer = new CFrameBuffer();
	right_buffer = new CFrameBuffer();

	left_buffer->init(m_width, m_height, m_buffer_unit_count, m_frame_count);
	right_buffer->init(m_width, m_height, m_buffer_unit_count, m_frame_count);

	vi.width = width;
	vi.height = height;
	vi.fps_numerator = fps;
	vi.fps_denominator = fpsdenumorator;

	return 0;
}

int JMAvs::insert_offset(BYTE *data, int count)
{
	FILE *f = m_offset_file;
	if (!f)
		return 0;

	m_header.point_count += count;
	m_header.fps_numerator = vi.fps_numerator;
	m_header.fps_denumerator = vi.fps_denominator;
	fseek(f, 0, SEEK_SET);
	fwrite(&m_header, 1, sizeof(m_header), f);
	fseek(f, 0, SEEK_END);
	fwrite(data, 1, count, f);
	fflush(f);

	return 0;
}

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
PVideoFrame __stdcall JMAvs::GetFrame(int n, IScriptEnvironment* env)
{
	PVideoFrame dst = env->NewVideoFrame(vi);

	if (right_buffer)
	{
		// get left and copy to dst;
		get_frame(n, m_swap_eyes ? right_buffer : left_buffer,dst->GetWritePtr(PLANAR_Y), m_width * 2, 0);

		// get right and copy to dst;
		get_frame(n, m_swap_eyes ? left_buffer : right_buffer, dst->GetWritePtr(PLANAR_Y), m_width * 2, m_width);
	}
	else
	{
		// get left and copy to dst;
		get_frame(n, left_buffer,dst->GetWritePtr(PLANAR_Y), m_width, 0);
	}

	int total = avs->left_buffer->m_unit_count + (avs->right_buffer ? avs->right_buffer->m_unit_count : 0);
	int current = avs->left_buffer->m_item_count + (avs->right_buffer ? avs->right_buffer->m_item_count : 0);

	buffer_load = (float)current/total;

	// add mask here
	// assume colorspace = YV12
	if (n==0)
	{
		HMODULE hm = (HMODULE)&__ImageBase;
		HGLOBAL hDllData = LoadResource(hm, FindResource(hm, MAKEINTRESOURCE(IDR_RCDATA1), RT_RCDATA));
		void * dll_data = LockResource(hDllData);

		SIZE size = {200, 136};

		// get data and close it
		unsigned char *m_mask = (unsigned char*)malloc(27200);
		memset(m_mask, 0, 27200);
		if(dll_data) memcpy(m_mask, dll_data, 27200);

		// add mask
		int width = vi.width;
		int height = vi.height;
		BYTE *pdst = dst->GetWritePtr(PLANAR_Y);
		for(int y=0; y<size.cy; y++)
		{
			memcpy(pdst+(y+height/2-size.cy/2)*width +width/4-size.cx/2, 
				m_mask + size.cx*y, size.cx);
			memcpy(pdst+(y+height/2-size.cy/2)*width +width/4-size.cx/2 + width/2,
				m_mask + size.cx*y, size.cx);
		}

		free(m_mask);
	}

	return dst;
}

void JMAvs::get_frame(int n, CFrameBuffer *the_buffer, BYTE*data, int stride, int offset)
{
retry:
	// check if decoding down/done

	// try!
	int got = the_buffer->remove(n, data, stride, offset);

	if (got == -1  || (got == -2 && m_decoding_done))
	{
		// wrong range or feeder down/done
		// set it black and return
		for(int i=0; i<m_height; i++)
		{
			memset(data+offset, 0, m_width);
			data += stride;
		}
		for(int i=0; i<m_height; i++)
		{
			memset(data+offset/2, 128, m_width/2);
			data += stride/2;
		}

		return;
	}

	if (got == -2)
	{
		// decoding slow, sleep 10ms and retry later
		Sleep(10);
		goto retry;
	}
}

DWORD WINAPI decoding_thread(LPVOID p)
{
	JMAvs *avs = (JMAvs *)p;

	int iRet;
	DecodedPicList *pDecPicList;
	int iFramesOutput=0, iFramesDecoded=0;
	InputParameters InputParams;

	init_time();

	//get input parameters;
	InputParameters *p_Inp = &InputParams;
	memset(p_Inp, 0, sizeof(InputParameters));
	strcpy(p_Inp->infile2, avs->m_m2ts_right);
	strcpy(p_Inp->infile, avs->m_m2ts_left); //! set default bitstream name
	strcpy(p_Inp->outfile, ""); //! set default output file name
	strcpy(p_Inp->reffile, ""); //! set default reference file name

	memcpy (&cfgparams, p_Inp, sizeof (InputParameters));

	//Set default parameters.
	printf ("Setting Default Parameters...\n");
	InitParams(Map);

	char dummpy_content[40960] = "";
	ParseContent (p_Inp, Map, dummpy_content, (int) strlen(dummpy_content));


	TestParams(Map, NULL);
	if(p_Inp->export_views == 1)
		p_Inp->dpb_plus[1] = imax(1, p_Inp->dpb_plus[1]);

	cfgparams = *p_Inp;
	p_Inp->enable_32_pulldown = 0;
	InputParams.output_to_avs = 1;
	InputParams.DecodeAllLayers = 1;

	if (InputParams.thread_count)
		omp_set_num_threads(InputParams.thread_count);

	if (InputParams.cpu_mask)
		SetProcessAffinityMask(GetCurrentProcess(), InputParams.cpu_mask);

	//open decoder;
	iRet = OpenDecoder(&InputParams);
	if(iRet != DEC_OPEN_NOERR)
	{
		fprintf(stderr, "Open encoder failed: 0x%x!\n", iRet);
		return -1; //failed;
	}

	//decoding;
	do
	{
		iRet = DecodeOneFrame(&pDecPicList);
		if(iRet==DEC_EOS || iRet==DEC_SUCCEED)
		{
			//process the decoded picture, output or display;
			iFramesDecoded++;
		}
		else
		{
			//error handling;
			fprintf(stderr, "Error in decoding process: 0x%x\n", iRet);
		}
	}while((iRet == DEC_SUCCEED) && ((p_Dec->p_Inp->iDecFrmNum==0) || (iFramesDecoded<p_Dec->p_Inp->iDecFrmNum)));


	//quit;
	iRet = FinitDecoder(&pDecPicList);
	iRet = CloseDecoder();
	printf("%d frames are decoded.\n", iFramesDecoded);

	return 0;
}