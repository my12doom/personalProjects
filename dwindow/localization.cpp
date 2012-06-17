#include "global_funcs.h"


// localization
typedef struct _localization_element
{
	wchar_t *english;
	wchar_t *localized;
} localization_element;

int n_localization_element_count;
const int increase_step = 8;	// 2^8 = 256, increase when
localization_element *localization_table = NULL;
HRESULT hr_init_localization = set_localization_language(g_active_language);
void dump();

HRESULT add_localization(const wchar_t *English, const wchar_t *Localized)
{
	if (Localized == NULL)
	{
		Localized = English;
	}

	if ((n_localization_element_count >> increase_step) << increase_step == n_localization_element_count)
	{
		localization_table = (localization_element*)realloc(localization_table, sizeof(localization_element) * (n_localization_element_count + (1<<increase_step)));
	}

	for(int i=0; i<n_localization_element_count; i++)
	{
		if (wcscmp(English, localization_table[i].english) == 0)
		{
			return S_FALSE;
		}
	}

	localization_table[n_localization_element_count].english = (wchar_t*)malloc(sizeof(wchar_t) * (wcslen(English)+1));
	wcscpy(localization_table[n_localization_element_count].english, English);
	localization_table[n_localization_element_count].localized = (wchar_t*)malloc(sizeof(wchar_t) * (wcslen(Localized)+1));
	wcscpy(localization_table[n_localization_element_count].localized, Localized);

	n_localization_element_count++;

	return S_OK;
}

const wchar_t *C(const wchar_t *English)
{
	for(int i=0; i<n_localization_element_count; i++)
	{
		if (wcscmp(localization_table[i].english, English) == 0)
			return localization_table[i].localized;
	}

	return English;
};
HRESULT set_localization_language(localization_language language)
{
	g_active_language = language;

	for(int i=0; i<n_localization_element_count; i++)
	{
		if (localization_table[i].english)
			free(localization_table[i].english);
		if (localization_table[i].localized)
			free(localization_table[i].localized);
	}
	if (localization_table) free(localization_table);
	localization_table = NULL;
	n_localization_element_count = 0;

	switch (language)
	{
	case ENGLISH:
		{
			/*
			add_localization(L"Open File...");
			add_localization(L"Open BluRay3D");
			add_localization(L"Close");

			add_localization(L"Input Layout");
			add_localization(L"Auto");
			add_localization(L"Side By Side");
			add_localization(L"Top Bottom");
			add_localization(L"Monoscopic");

			add_localization(L"Aspect Ratio");
			add_localization(L"Default");

			add_localization(L"Output Mode");
			add_localization(L"Nvidia 3D Vision");
			add_localization(L"Monoscopic 2D");
			add_localization(L"Interlace Mode");
			add_localization(L"Row Interlace");
			add_localization(L"Line Interlace");
			add_localization(L"Checkboard Interlace");
			add_localization(L"Dual Projector");
			add_localization(L"Horizontal Span Mode");
			add_localization(L"Vertical Span Mode");
			add_localization(L"Independent Mode");
			add_localization(L"3DTV");
			add_localization(L"Half Side By Side");
			add_localization(L"Half Top Bottom");
			add_localization(L"Line Interlace");
			add_localization(L"Naked eye 3D");
			add_localization(L"3D Ready DLP");
			add_localization(L"Anaglyph");
			add_localization(L"Gerneral 120Hz Glasses");

			add_localization(L"Audio");

			add_localization(L"Subtitle");

			add_localization(L"Play/Pause");
			add_localization(L"Play");
			add_localization(L"Pause");
			add_localization(L"Fullscreen");
			add_localization(L"Swap Left/Right");
			add_localization(L"Always Show Right Eye");
			add_localization(L"Exit");

			add_localization(L"No BD Drive Found");
			add_localization(L"Folder...");
			add_localization(L"None");
			add_localization(L"Load Subtitle File...");
			add_localization(L"Load Audio Track...");
			add_localization(L"Font...");
			add_localization(L"Color...");
			add_localization(L" (No Disc)");
			add_localization(L" (No Movie Disc)");
			add_localization(L"Select Folder..");
			add_localization(L"Open File");
			add_localization(L"Language");
			add_localization(L"Feature under development");
			add_localization(L"Open Failed");
			add_localization(L"Display Subtitle");
			add_localization(L"Red - Cyan (Red - Blue, Red- Green)");
			add_localization(L"Custom Color");
			add_localization(L"Anaglyph Left Eye Color...");
			add_localization(L"Anaglyph Right Eye Color...");

			add_localization(L"Enter User ID");
			add_localization(L"This program will exit now, Restart it to use new user id.");
			add_localization(L"Exiting");
			add_localization(L"Activation Failed, Server Response:\n.");
			add_localization(L"Warning");
			add_localization(L"You selected the same monitor!");
			add_localization(L"CUDA Accelaration");
			add_localization(L"CUDA setting will apply on next file play.");
			*/
		}
		break;
	case CHINESE:
		{

			// intel S3D
			add_localization(L"Plase switch to one of device supported 3D reslutions first:\n\n",
							 L"请先切换到设备支持的3D分辨率：\n\n");
			add_localization(L"No supported device found.",			L"未找到支持的设备");

			add_localization(L"Monitor",				L"显示器");
			add_localization(L"Open File...",			L"打开文件...");
			add_localization(L"Open BluRay3D",			L"打开蓝光3D原盘");
			add_localization(L"Close",					L"关闭");

			add_localization(L"Input Layout",			L"输入格式");
			add_localization(L"Auto",					L"自动判断");
			add_localization(L"Side By Side",			L"左右格式");
			add_localization(L"Top Bottom",				L"上下格式");
			add_localization(L"Monoscopic",				L"非立体影片");

			add_localization(L"Aspect Ratio",			L"画面比例");
			add_localization(L"Output Aspect Ratio",	L"显示长宽比");
			add_localization(L"Default",				L"默认");
			add_localization(L"Letterbox Cropping",		L"切除黑边");
			add_localization(L"Aspect Ratio Mode",		L"长宽比模式");
			add_localization(L"Aspect Ratio",			L"画面比例");
			add_localization(L"Stretch",				L"拉伸");
			add_localization(L"Default(Letterbox)",		L"默认(黑边)");
			add_localization(L"Horizontal Fill",		L"水平填充");
			add_localization(L"Vertical Fill",			L"垂直填充");


			add_localization(L"Fullscreen Output",						L"全屏输出");
			add_localization(L"Fullscreen Output 1",					L"全屏输出1");
			add_localization(L"Fullscreen Output 2",					L"全屏输出2");

			add_localization(L"Output 1",					L"输出1");
			add_localization(L"Output 2",					L"输出2");
			add_localization(L"Output Mode",					L"输出模式");
			add_localization(L"Nvidia 3D Vision",				L"Nvidia 3D Vision");
			add_localization(L"AMD HD3D",						L"AMD HD3D");
			add_localization(L"Intel Stereoscopic",			L"Intel 立体显示");
			add_localization(L"Monoscopic 2D",					L"平面(2D)");
			add_localization(L"Interlace Mode",					L"交错模式");
			add_localization(L"Row Interlace",					L"列交错");
			//add_localization(L"Line Interlace",					L"行交错");
			add_localization(L"Checkboard Interlace",			L"棋盘交错");
			add_localization(L"Dual Projector",					L"双投影");
			add_localization(L"Horizontal Span Mode",			L"水平跨越模式");
			add_localization(L"Vertical Span Mode",				L"垂直跨越模式");
			add_localization(L"Independent Mode",				L"独立配置模式");
			add_localization(L"3DTV",							L"3D电视");
			add_localization(L"Half Side By Side",				L"左右半宽");
			add_localization(L"Half Top Bottom",				L"上下半高");
			add_localization(L"Line Interlace",					L"被动偏振");
			add_localization(L"Row Interlace",					L"裸眼3D");
			add_localization(L"Anaglyph",						L"有色眼镜");
			add_localization(L"IZ3D Displayer",					L"IZ3D 显示器");
			add_localization(L"Gerneral 120Hz Glasses",			L"普通120Hz眼镜");
			add_localization(L"Checkboard Interlace", L"棋盘交错");

			add_localization(L"Audio",					L"音频");
			add_localization(L"Use LAV Audio Decoder",		L"使用内置音频解码器");
			add_localization(L"Audio Decoder setting may not apply until next file play or audio swtiching.",
				L"音频解码器已设置，可能需要重新播放或音轨切换时生效");
			add_localization(L"Use Bitstreaming",			L"使用源码输出");
			add_localization(L"Bitstreaming setting may not apply until next file play or audio swtiching.",
				L"源码输出已设置，可能需要重新播放或音轨切换时生效");

			add_localization(L"Subtitle",				L"字幕");
			add_localization(L"This is first time to load ass/ssa subtilte, font scanning may take one minute or two, the player may looks like hanged, please wait...",
							 L"这是第一次载入ass/ssa字幕，扫描字体可能需要几分钟，期间播放器可能无法响应操作，请稍候...");
			add_localization(L"Please Wait",			L"请稍候");

			add_localization(L"Video",					L"视频");
			add_localization(L"Adjust Color...",					L"调整色彩...");
			add_localization(L"Forced Deinterlace",		L"强制去除隔行扫描");
			add_localization(L"Deinterlacing is not recommended unless you see combing artifacts on moving objects.",
							 L"除非确定在画面移动时发现了梳子状横条，否则建议不要强制去除隔行扫描。");

			add_localization(L"Play/Pause\t(Space)",				L"播放/暂停\t(Space)");
			add_localization(L"Play\t(Space)",					L"播放\t(Space)");
			add_localization(L"Pause\t(Space)",					L"暂停\t(Space)");
			add_localization(L"Fullscreen\t(Alt+Enter)",				L"全屏\t(Alt+Enter)");
			add_localization(L"Always Show Right Eye",	L"总是显示右眼");
			add_localization(L"Swap Left/Right\t(Tab)",		L"左右眼对调\t(Tab)");
			add_localization(L"Exit",					L"退出");

			add_localization(L"No BD Drive Found",		L"未找到蓝光驱动器");
			add_localization(L"Folder...",				L"文件夹...");
			add_localization(L"None",					L"无");
			add_localization(L"Load Subtitle File...",	L"从文件载入...");
			add_localization(L"Load Audio Track...",	L"载入外部音轨...");
			add_localization(L"Font...",				L"字体...");
			add_localization(L"Color...",				L"颜色...");
			add_localization(L"Latency && Stretch...",	L"延迟与拉伸...");
			add_localization(L" (No Disc)",				L" (无光盘)");
			add_localization(L" (No Movie Disc)",		L" (非电影光盘)");
			add_localization(L"Select Folder..",		L"选择文件夹..");
			add_localization(L"Open File",				L"打开文件");
			add_localization(L"Language",				L"语言");
			add_localization(L"Feature under development",L"尚未完成的功能");
			add_localization(L"Open Failed",			L"打开失败");
			add_localization(L"Display Subtitle",		L"显示字幕");
			add_localization(L"Red - Cyan (Red - Blue, Red- Green)",	L"红青 (红蓝 红绿)");
			add_localization(L"Custom Color",			L"自定义颜色");
			add_localization(L"Anaglyph Left Eye Color...",L"左眼镜颜色...");
			add_localization(L"Anaglyph Right Eye Color...",L"右眼镜颜色...");

			add_localization(L"Enter User ID",			L"输入用户号");
			add_localization(L"This program will exit now, Restart it to use new user id.",	L"激活完成，现在程序将自动退出，新用户号在下次启动后生效");
			add_localization(L"Exiting",				L"正在退出");
			add_localization(L"Activation Failed, Server Response:\n.", L"激活失败, 服务器反馈信息：\n");

			add_localization(L"Error",					L"错误");
			add_localization(L"Warning",				L"警告");
			add_localization(L"You selected the same monitor!",L"选择了相同的显示器!");
			add_localization(L"Use CUDA Accelaration",		L"使用CUDA硬解加速");
			add_localization(L"CUDA setting will apply on next file play.",
														L"CUDA设置将在下一个文件播放时生效");
			add_localization(L"System initialization failed : monitor detection error, the program will exit now.",
														L"初始化失败：显示设备检测失败，程序将退出。");
			add_localization(L"System initialization failed : server not found, the program will exit now.",
														L"初始化失败：服务器无响应，程序将退出。");
			add_localization(L"You are using a trial copy of DWindow, each clip will play normally for 10 minutes, after that the picture will become grayscale.\nYou can reopen it to play normally for another 10 minutes.\nRegister to remove this limitation.",
							 L"你正在使用免费版的DWindow, 影片播放10分钟后会逐渐变为黑白，其他方面与正式版没有区别\n而且你也可以重新打开影片即可恢复色彩\n注册正式版后无此限制！");

			add_localization(L"Downmix To 16bit Stereo", L"输出16位双声道音频");
			add_localization(L"Logged out, the program will exit now, restart the program to login.",
							 L"注销成功，现在程序将退出，下一次启动时你可以登录其他账号");
			add_localization(L"Are you sure want to logout?", L"确定要注销么？");
			add_localization(L"Logout...", L"注销...");
			add_localization(L"Are you sure?", L"确定?");

			// login window
			add_localization(L"Start Activation", L"开始激活");
			add_localization(L"Trial", L"免费测试");
			add_localization(L"Login", L"登录");
			add_localization(L"OK",    L"确定");
			add_localization(L"Cancel", L"取消");
			add_localization(L"User", L"用户");
			add_localization(L"Password", L"密码");
			
			// latency/stretch window
			add_localization(L"Delay/Stretch", L"延迟/拉伸");
			add_localization(L"Delay", L"延迟");
			add_localization(L"Stretch", L"拉伸");
			add_localization(L"Reset", L"重置");
			add_localization(L"Ms", L"毫秒");
			add_localization(L"Warning: negative delay or stretch less than 1.0 can cause internal subtitle loss, external subtitle is not affected.",
				L"注意，提前/缩短功能对内置字幕支持较差，调整时尽量不要超过10秒，否则可能出现字幕丢失，延迟/拉伸功能不受限制，独立字幕亦不受限制。");

			// color adjustment
			add_localization(L"Adjust Color", L"色彩调节");
			add_localization(L"Preview", L"预览");
			add_localization(L"SyncAdjust", L"双眼同时调节");
			add_localization(L"Left Eye", L"左眼");
			add_localization(L"Right Eye", L"右眼");
			add_localization(L"Luminance", L"亮度");
			add_localization(L"Saturation", L"饱和度");
			add_localization(L"Hue", L"色调");
			add_localization(L"Contrast", L"对比度");


			// free version
			add_localization(L"Dual projector and IZ3D mode is only available in registered version.", L"双投影与IZ3D功能仅在付费版可用");
			add_localization(L"External audio track support is only available in registered version.", L"载入外部音轨功能仅在付费版可用");


			// ass fonts
			add_localization(L"(Fonts Loading)", L"（字体正在载入中）");

			// Media Info
			add_localization(L"Media Infomation...", L"文件信息...");
			add_localization(L"MediaInfoLanguageFile", L"language\\MediaInfoCN");
			add_localization(L"Reading Infomation ....", L"正在读取信息");

			add_localization(L"Display Orientation", L"显示器方向");
			add_localization(L"Horizontal", L"横向");
			add_localization(L"Vertical", L"纵向");


			// Open Double File
			add_localization(L"Open Left And Right File...", L"打开左右分离文件...");
			add_localization(L"Left File:", L"左眼文件");
			add_localization(L"Right File:", L"右眼文件");

		}
		break;
	}

	dump();

	return S_OK;
}

bool wcs_replace(wchar_t *to_replace, const wchar_t *searchfor, const wchar_t *replacer);

void dump()
{
	char bom[2] = {0xff, 0xfe};
	wchar_t tmp[10240];

	FILE *f = fopen("Z:\\lang.txt", "wb");
	if (f)
	{
		fwrite(bom, 1, 2, f);
		fwprintf(f, L"中文\r\n");

		for(int i=0; i<n_localization_element_count; i++)
		{
			wcscpy(tmp, localization_table[i].english);
			wcs_replace(tmp, L"\\", L"\\\\");
			wcs_replace(tmp, L"\t", L"\\t");
			wcs_replace(tmp, L"\n", L"\\n");
			fwprintf(f, L"%s\r\n", tmp);

			wcscpy(tmp, localization_table[i].localized);
			wcs_replace(tmp, L"\n", L"\\n");		
			fwprintf(f, L"%s\r\n", tmp);
		}

		fclose(f);
	}
}