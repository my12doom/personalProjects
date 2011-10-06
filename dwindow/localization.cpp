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
			add_localization(L"Default",				L"默认");

			add_localization(L"Output 1",					L"输出1");
			add_localization(L"Output 2",					L"输出2");
			add_localization(L"Output Mode",					L"输出模式");
			add_localization(L"Nvidia 3D Vision",				L"Nvidia 3D Vision");
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
			add_localization(L"Naked eye 3D",					L"裸眼3D");
			add_localization(L"Anaglyph",						L"有色眼镜");
			add_localization(L"IZ3D Displayer",					L"IZ3D 显示器");
			add_localization(L"Gerneral 120Hz Glasses",			L"普通120Hz眼镜");

			add_localization(L"Audio",					L"音频");
			add_localization(L"Use LAV Audio Decoder",		L"使用内置音频解码器");
			add_localization(L"Audio Decoder setting may not apply until next file play or audio swtiching.",
				L"音频解码器已设置，可能需要重新播放或音轨切换时生效");
			add_localization(L"Use Bitstreaming",			L"使用源码输出");
			add_localization(L"Bitstreaming setting may not apply until next file play or audio swtiching.",
				L"源码输出已设置，可能需要重新播放或音轨切换时生效");

			add_localization(L"Subtitle",				L"字幕");

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
		}
		break;
	}
	return S_OK;
}
