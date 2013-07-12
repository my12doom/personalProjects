// SetDlg.cpp : 定义控制台应用程序的入口点。
//

#include <stdio.h>
#include <tchar.h>
#include <Windows.h>
#include <commctrl.h>
#include "resource3.h"
#include "..\lua\my12doom_lua.h"


enum
{
    ORDINARY_CLICK = 0,
    VOICE_CLICK,
    VIDEO1_CLICK,
    VIDEO2_CLICK,
    SET3D_CLICK,
    SUBTILE_CLICK,
    ABOUT_CLICK,
    RELATION_CLICK,
    VIDEO_CLICK
};
enum contrl_type
{
    TYPE_RADIO,
    TYPE_CHECK,
    TYPE_COMBO,
    TYPE_EDIT,
    TYPE_SLIDER
};
enum aspect_mode_types
{
    aspect_letterbox,
    aspect_stretch,
    aspect_horizontal_fill,
    aspect_vertical_fill,
    aspect_mode_types_max,
};
enum output_mode_types
{
    NV3D, masking, anaglyph, mono, pageflipping, iz3d,
    dual_window, out_sbs, out_tb,
    out_hsbs, out_htb, 
    hd3d, 
    intel3d,
    multiview,
    output_mode_types_max
};

enum mask_mode_types
{
    row_interlace, 
    line_interlace, 
    checkboard_interlace,
    subpixel_row_interlace,
    subpixel_45_interlace,
    subpixel_x2_interlace,
    subpixel_x2_45_interlace,
    subpixel_x2_minus45_interlace,
    mask_mode_types_max,
};
enum input_layout_types
{
    side_by_side, 
    top_bottom, mono2d, 
    frame_sequence,
    input_layout_types_max, 
    input_layout_auto = 0x10000,
};

struct tagLuaSetting
{
    int nCtrlID;
    char arrSettingName[100];
    int nCtrlType;
    //lua_const& luaVal;
};

DWORD color = 16777215;

HWND Ordinary[11] = {0};
HWND Voice[4] = {0};
HWND VideoPage1[18] = {0};
HWND VideoPage2[23] = {0};
HWND Set3D[20] = {0};
HWND Subtitle[20] = {0};
HWND About[20] = {0};
HWND Relation[20] = {0};

// lua_const& FullMode;

HWND *AllCtrl[10]  = {Ordinary, Voice, VideoPage1, VideoPage2, Set3D, Subtitle, About, Relation};
tagLuaSetting TestSet[] = 
{
    //
//     {IDC_ORD_RADIO_ALWAY, "", TYPE_RADIO},
//     {IDC_ORD_RADIO_WHENPLAY, "", TYPE_RADIO},
//     {IDC_ORD_RADIO_NEVER, "", TYPE_RADIO},
//     {IDC_ORD_COM_LANGUAGE, "", TYPE_COMBO},
    //Video page 1
    {IDC_VIDEO_COM_FULLMODE, "AspectRatioMode", TYPE_COMBO},        //0
    {IDC_VIDEO_COM_IMAGERATE, "Aspect", TYPE_COMBO},                //1
    {IDC_VIDEO_CHECK_LOW_CPU, "GPUIdle", TYPE_CHECK},               //2
    {IDC_VIDEO_CHECK_FULLSCREEN, "ExclusiveMode", TYPE_CHECK},      //3
    //{IDC_VIDEO_CHECK_WIN_2_VIDEO, "", TYPE_CHECK},
    {IDC_VIDEO_CHECK_VIDEO_2_WIN, "OnOpen", TYPE_CHECK},            //4
//     {IDC_VIDEO_CHECK_LOAD_LAST_POS, "", TYPE_CHECK},
    {IDC_VIDEO_CHECK_HARD_SPEED, "CUDA", TYPE_CHECK},               //5
    //{IDC_VIDEO_CHECK_DEINTERLACE, "", TYPE_CHECK},
    //Video page 2
    {IDC_VIDEO_RADIO_SAME_CONTROL, "SyncAdjust", TYPE_CHECK},       //6
    {IDC_VIDEO_SLIDER_LEFT_BRIGHTNESS, "Luminance", TYPE_SLIDER},   //7  GET_CONt(m[7].c) = 
    {IDC_VIDEO_SLIDER_LEFT_CONTRAST, "Contrast", TYPE_SLIDER},      //8
    {IDC_VIDEO_SLIDER_LEFT_SATURATION, "Saturation", TYPE_SLIDER},  //9
    {IDC_VIDEO_SLIDER_LEFT_HUE, "Hue", TYPE_SLIDER},                //10
    {IDC_VIDEO_SLIDER_RIGHT_BRIGHTNESS, "Luminance2", TYPE_SLIDER}, //11
    {IDC_VIDEO_SLIDER_RIGHT_CONTRAST, "Contrast2", TYPE_SLIDER},    //12
    {IDC_VIDEO_SLIDER_RIGHT_SATURATION, "Saturation2", TYPE_SLIDER},//13
    {IDC_VIDEO_SLIDER_RIGHT_HUE, "Hue2", TYPE_SLIDER},              //14
    //3D page
    {IDC_3D_CHECK_SHOW2D, "Force2D", TYPE_CHECK},                   //15
    {IDC_3D_CHECK_EXCHANGE, "SwapEyes", TYPE_CHECK},                //16
    {IDC_3D_COM_INPUT_MODE, "InputLayout", TYPE_COMBO},             //17
    {IDC_3D_COM_OUTPUT_MODE, "OutputMode", TYPE_COMBO},             //18
    {IDC_3D_COM_FALL_MODE_ONE, "Screen1", TYPE_COMBO},              //19
    {IDC_3D_COM_FALL_MODE_TWO, "Screen2", TYPE_COMBO},              //20

    //voice 
    {IDC_VOICE_COM_CHANNEL, "AudioChannel", TYPE_COMBO},            //21
    {IDC_VOICE_CHECK_AUTO_VOICE_DECODE, "InternalAudioDecoder", TYPE_CHECK},//22
    {IDC_VOICE_COM_MAX_VOICE, "NormalizeAudio", TYPE_CHECK}, //16 or 1      //23

    //Subtitle
    //DisplaySubtitle
    {IDC_SUBTITLE_CHECK_SHOWSUB, "DisplaySubtitle", TYPE_CHECK},            //24
    {IDC_SUBTITLE_COM_STYLE, "Font", TYPE_COMBO},                           //25
    {IDC_SUBTITLE_COM_SIZE, "FontSize", TYPE_COMBO},                        //26
    {IDC_SUBTITLE_COM_COLOR, "FontColor", TYPE_COMBO},                      //27
    {IDC_SUBTITLE_EDIT_LATE_SHOW, "SubtitleLatency", TYPE_EDIT},            //28
    {IDC_SUBTITLE_EDIT_LATE_SHOW_SIZE, "SubtitleRatio", TYPE_EDIT},         //29
    //add
    {IDC_VIDEO_CHECK_DEINTERLACE, "ForcedDeinterlace", TYPE_CHECK}

    //Relationship
//     {IDC_RELA_CHECK_MP4, "", TYPE_CHECK},
//     {IDC_RELA_CHECK_AVI, "", TYPE_CHECK},
//     {IDC_RELA_CHECK_RMVB, "", TYPE_CHECK},
//     {IDC_RELA_CHECK_3DV, "", TYPE_CHECK},
//     {IDC_RELA_CHECK_TS, "", TYPE_CHECK},
//     {IDC_RELA_CHECK_MKV, "", TYPE_CHECK},
//     {IDC_RELA_CHECK_WMV, "", TYPE_CHECK},
//     {IDC_RELA_CHECK_VOB, "", TYPE_CHECK},
};

void GetCtrolItemhWnd(HWND hwndDlg);
void SetShowCtrol(int nItem, int nSubItem = 0);
void LoadSetting(HWND hWnd);
void ApplySetting(HWND hWnd);
int CALLBACK MyEnumFontProc(ENUMLOGFONTEX* lpelf,NEWTEXTMETRICEX* lpntm,DWORD nFontType,long lParam)
{
    HWND hWnd = (HWND)lParam;
    if (SendMessageW(GetDlgItem(hWnd, IDC_SUBTITLE_COM_STYLE), CB_FINDSTRING, 0, WPARAM(lpelf->elfLogFont.lfFaceName)))
        SendMessageW(GetDlgItem(hWnd, IDC_SUBTITLE_COM_STYLE), CB_INSERTSTRING, 0, (WPARAM)lpelf->elfLogFont.lfFaceName);
    return 1;
}

BOOL CALLBACK SetProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            GetCtrolItemhWnd(hwndDlg);
            SetShowCtrol(ORDINARY_CLICK, 0);
            //Language init
            SendMessageA(GetDlgItem(hwndDlg, IDC_ORD_COM_LANGUAGE), CB_INSERTSTRING, 0, (WPARAM)"English");
            SendMessageA(GetDlgItem(hwndDlg, IDC_ORD_COM_LANGUAGE), CB_INSERTSTRING, 0, (WPARAM)"中文");
            //image format init 
            SendMessageA(GetDlgItem(hwndDlg, IDC_VIDEO_COM_IMAGERATE), CB_INSERTSTRING, 0, (WPARAM)"2.35:1");
            SendMessageA(GetDlgItem(hwndDlg, IDC_VIDEO_COM_IMAGERATE), CB_INSERTSTRING, 0, (WPARAM)"16:9");
            SendMessageA(GetDlgItem(hwndDlg, IDC_VIDEO_COM_IMAGERATE), CB_INSERTSTRING, 0, (WPARAM)"4:3");
            SendMessageA(GetDlgItem(hwndDlg, IDC_VIDEO_COM_IMAGERATE), CB_INSERTSTRING, 0, (WPARAM)"原比例");

            //Stretch mode
            SendMessageA(GetDlgItem(hwndDlg, IDC_VIDEO_COM_FULLMODE), CB_INSERTSTRING, 0, (WPARAM)"垂直填充");
            SendMessageA(GetDlgItem(hwndDlg, IDC_VIDEO_COM_FULLMODE), CB_INSERTSTRING, 0, (WPARAM)"水平填充");
            SendMessageA(GetDlgItem(hwndDlg, IDC_VIDEO_COM_FULLMODE), CB_INSERTSTRING, 0, (WPARAM)"拉伸");
            SendMessageA(GetDlgItem(hwndDlg, IDC_VIDEO_COM_FULLMODE), CB_INSERTSTRING, 0, (WPARAM)"默认(黑边)");

            //input mode
            SendMessageA(GetDlgItem(hwndDlg, IDC_3D_COM_INPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)"自动识别");
            SendMessageA(GetDlgItem(hwndDlg, IDC_3D_COM_INPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)"2D影片");
            SendMessageA(GetDlgItem(hwndDlg, IDC_3D_COM_INPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)"上下半高");
            SendMessageA(GetDlgItem(hwndDlg, IDC_3D_COM_INPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)"左右半宽");

            //output mode
            SendMessageA(GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)"3D电视-裸眼3D");
            SendMessageA(GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)"3D电视-棋盘交错");
            SendMessageA(GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)"3D电视-被动偏振");
            SendMessageA(GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)"Intel 立体显示");
            SendMessageA(GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)"AMD HD 3D");
            SendMessageA(GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)"3D电视-上下半高");
            SendMessageA(GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)"3D电视-左右半宽");
            SendMessageA(GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)"IZ3D显示器");
            SendMessageA(GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)"普通120Hz眼镜");
            SendMessageA(GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)"平面(2D)");
            SendMessageA(GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)"有色眼镜");
            SendMessageA(GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)"子像素交织");
            SendMessageA(GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)"Nvidia 3D Vision");

            // audio channel
            SendMessageA(GetDlgItem(hwndDlg, IDC_VOICE_COM_CHANNEL), CB_INSERTSTRING, 0, (WPARAM)"源码输出");
            SendMessageA(GetDlgItem(hwndDlg, IDC_VOICE_COM_CHANNEL), CB_INSERTSTRING, 0, (WPARAM)"杜比耳机");
            SendMessageA(GetDlgItem(hwndDlg, IDC_VOICE_COM_CHANNEL), CB_INSERTSTRING, 0, (WPARAM)"7.1声道");
            SendMessageA(GetDlgItem(hwndDlg, IDC_VOICE_COM_CHANNEL), CB_INSERTSTRING, 0, (WPARAM)"5.1声道");
            SendMessageA(GetDlgItem(hwndDlg, IDC_VOICE_COM_CHANNEL), CB_INSERTSTRING, 0, (WPARAM)"立体声");
            SendMessageA(GetDlgItem(hwndDlg, IDC_VOICE_COM_CHANNEL), CB_INSERTSTRING, 0, (WPARAM)"来源");

            //font 
            HDC dc = GetDC(hwndDlg);
            LOGFONT lf;
            lf.lfCharSet=DEFAULT_CHARSET;
            lf.lfFaceName[0]=NULL;
            lf.lfPitchAndFamily=0;
            EnumFontFamiliesEx(dc, &lf, (FONTENUMPROC)MyEnumFontProc, (LPARAM)hwndDlg, 0);


            //font size
            char szSize[10] = {0};
            for (int i = 10; i >=1; i--)
            {
                sprintf(szSize, "%d", (i + 1) * 10);
                SendMessageA(GetDlgItem(hwndDlg, IDC_SUBTITLE_COM_SIZE), CB_INSERTSTRING, 0, (WPARAM)szSize);
            }

            SendMessageA(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_BRIGHTNESS), TBM_SETRANGE, TRUE, MAKELPARAM(0, 1000));
            SendMessageA(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_CONTRAST), TBM_SETRANGE, TRUE, MAKELPARAM(0, 1000));
            SendMessageA(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_SATURATION), TBM_SETRANGE, TRUE, MAKELPARAM(0, 1000));
            SendMessageA(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_HUE), TBM_SETRANGE, TRUE, MAKELPARAM(0, 1000));
            SendMessageA(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_BRIGHTNESS), TBM_SETRANGE, TRUE, MAKELPARAM(0, 1000));
            SendMessageA(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_CONTRAST), TBM_SETRANGE, TRUE, MAKELPARAM(0, 1000));
            SendMessageA(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_SATURATION), TBM_SETRANGE, TRUE, MAKELPARAM(0, 1000));
            SendMessageA(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_HUE), TBM_SETRANGE, TRUE, MAKELPARAM(0, 1000));


            LoadSetting(hwndDlg);

//             SendMessageA(GetDlgItem(hwndDlg, IDC_ORD_RADIO_ALWAY), BM_SETCHECK, BST_CHECKED, 0); 
//             SendMessageA(GetDlgItem(hwndDlg, IDC_ORD_RADIO_WHENPLAY), BM_SETCHECK, BST_CHECKED, 0); 
//             SendMessageA(GetDlgItem(hwndDlg, IDC_ORD_RADIO_NEVER), BM_SETCHECK, BST_CHECKED, 0); 
//             SendMessageA(GetDlgItem(hwndDlg, IDC_3D_CHECK_EXCHANGE), BM_SETCHECK, BST_UNCHECKED, 0);
            
            //SetCheck()
            //KillbtnFocus(ORDINARY_CLICK, hwndDlg);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDOK:
                EndDialog(hwndDlg, wParam);
                break;
            case IDCANCEL:
                EndDialog(hwndDlg, wParam);
                break;
            case IDC_BTN_ORDINARY:
                SetShowCtrol(ORDINARY_CLICK, 0);
                //KillbtnFocus(ORDINARY_CLICK, hwndDlg);
                //                 ShowWindow(BtnHwnd, SW_SHOW);
                break;
            case IDC_BTN_SOUND:
                SetShowCtrol(VOICE_CLICK, 0);
                //KillbtnFocus(VOICE_CLICK, hwndDlg);
                break;
            case IDC_BTN_3D:
                SetShowCtrol(SET3D_CLICK, 0);
                //KillbtnFocus(SET3D_CLICK, hwndDlg);
                break;
            case IDC_BTN_SUBTITLE:
                SetShowCtrol(SUBTILE_CLICK, 0);
                //KillbtnFocus(SUBTILE_CLICK, hwndDlg);
                break;
            case IDC_BTN_VIDEO:
                SetShowCtrol(VIDEO1_CLICK, 0);
                //KillbtnFocus(VIDEO1_CLICK, hwndDlg);
                break;
            case IDC_VIDEO_BTN_PAGE_ONE:
                SetShowCtrol(VIDEO1_CLICK, 0);
                break;
            case IDC_VIDEO_BTN_PAGE_TWO:
                SetShowCtrol(VIDEO2_CLICK, 0);
                break;
//             case IDC_BTN_HOTBTN:
//                 SetShowCtrol(HOTKEY_CLICK, 0);
//                 //KillbtnFocus(HOTKEY_CLICK, hwndDlg);
//                 break;
//             case IDC_BTN_PRTSCR:
//                 SetShowCtrol(PRTSRC_CLICK, 0);
//                 //KillbtnFocus(PRTSRC_CLICK, hwndDlg);
//                 break;
            case IDC_BTN_ABOUT:
                SetShowCtrol(ABOUT_CLICK, 0);
                //KillbtnFocus(ABOUT_CLICK, hwndDlg);
                break;
            case IDC_BTN_RELATIONSHIP:
                SetShowCtrol(RELATION_CLICK, 0);
                //KillbtnFocus(RELATION_CLICK, hwndDlg);
                break;
            case IDC_SUBTITLE_BTN_SELECT_COLOR:
                {
                    static COLORREF customColor[16];
                    customColor[0] = color;
                    CHOOSECOLOR cc = {sizeof(CHOOSECOLOR), hwndDlg, NULL, color, customColor, CC_RGBINIT, 0, NULL, NULL};
                    BOOL rtn = ChooseColor(&cc);
                    if (rtn)
                        color = cc.rgbResult;
                }
                break;
            case IDC_BTN_APPLY:
                {
                    ApplySetting(hwndDlg);
                }
				break;
            case IDC_BTN_OK:
                {
                    ApplySetting(hwndDlg);
                    EndDialog(hwndDlg, wParam);
                }
                break;
            case IDC_BTN_CANCEL:
                EndDialog(hwndDlg, wParam);
                break;
            case IDC_SUBTITLE_BTN_RESET:
                SetDlgItemTextA(hwndDlg, IDC_SUBTITLE_EDIT_LATE_SHOW, "0");
                SetDlgItemTextA(hwndDlg, IDC_SUBTITLE_EDIT_LATE_SHOW_SIZE, "100");
                break;
            }
        }
            break;
    }
    return FALSE;
}

int setting_dlg(HWND parent)
{
    DialogBoxA(NULL, MAKEINTRESOURCEA(IDD_DLG_MAIN), parent, SetProc);
	return 0;
}

void GetCtrolItemhWnd(HWND hwndDlg)
{
    Ordinary[0] = GetDlgItem(hwndDlg, IDC_ORD_STATIC_ONTOP);
    Ordinary[1] = GetDlgItem(hwndDlg, IDC_ORD_RADIO_ALWAY);
    Ordinary[2] = GetDlgItem(hwndDlg, IDC_ORD_RADIO_WHENPLAY);
    Ordinary[3] = GetDlgItem(hwndDlg, IDC_ORD_RADIO_NEVER);
//     Ordinary[4] = GetDlgItem(hwndDlg, IDC_ORD_STATIC_CLOSE);
//     Ordinary[5] = GetDlgItem(hwndDlg, IDC_ORD_RADIO_ONTRAY);
//     Ordinary[6] = GetDlgItem(hwndDlg, IDC_ORD_RADIO_CLOSE);
//     Ordinary[7] = GetDlgItem(hwndDlg, IDC_ORD_STATIC_BOOT);
//     Ordinary[8] = GetDlgItem(hwndDlg, IDC_ORD_RADIO_BOOT);
    Ordinary[4] = GetDlgItem(hwndDlg, IDC_ORD_STATIC_LANGUAGE);
    Ordinary[5] = GetDlgItem(hwndDlg, IDC_ORD_COM_LANGUAGE);

    //声音页面控件信息
    Voice[0] = GetDlgItem(hwndDlg, IDC_VOICE_STATIC_CHANNEL);
    Voice[1] = GetDlgItem(hwndDlg, IDC_VOICE_COM_CHANNEL);
    Voice[2] = GetDlgItem(hwndDlg, IDC_VOICE_CHECK_AUTO_VOICE_DECODE);
    Voice[3] = GetDlgItem(hwndDlg, IDC_VOICE_COM_MAX_VOICE);



    //视频页面控件1信息
//     VideoPage1[0] = GetDlgItem(hwndDlg, IDC_VIDEO_STATIC_VIDEOLIST);
//     VideoPage1[1] = GetDlgItem(hwndDlg, IDC_VIDEO_RADIO_ALWAY_EXPEND);
//     VideoPage1[2] = GetDlgItem(hwndDlg, IDC_VIDEO_RADIO_AUTO_HIDE);
//     VideoPage1[3] = GetDlgItem(hwndDlg, IDC_VIDEO_RADIO_OPEN_SAME);
//     VideoPage1[4] = GetDlgItem(hwndDlg, IDC_VIDEO_RADIO_LOAD_FLODER);
    VideoPage1[0] = GetDlgItem(hwndDlg, IDC_VIDEO_STATIC_FULLMODE);
    VideoPage1[1] = GetDlgItem(hwndDlg, IDC_VIDEO_COM_FULLMODE);
    VideoPage1[2] = GetDlgItem(hwndDlg, IDC_VIDEO_STATIC_IMAGERATE);
    VideoPage1[3] = GetDlgItem(hwndDlg, IDC_VIDEO_COM_IMAGERATE);
    VideoPage1[4] = GetDlgItem(hwndDlg, IDC_VIDEO_CHECK_LOW_CPU);
    VideoPage1[5] = GetDlgItem(hwndDlg, IDC_VIDEO_CHECK_FULLSCREEN);
    VideoPage1[6] = GetDlgItem(hwndDlg, IDC_VIDEO_CHECK_VIDEO_2_WIN);
    VideoPage1[7] = GetDlgItem(hwndDlg, IDC_VIDEO_CHECK_LOAD_LAST_POS);
    VideoPage1[8] = GetDlgItem(hwndDlg, IDC_VIDEO_CHECK_HARD_SPEED);
    VideoPage1[9] = GetDlgItem(hwndDlg, IDC_VIDEO_CHECK_DEINTERLACE);
    VideoPage1[10] = GetDlgItem(hwndDlg, IDC_VIDEO_BTN_PAGE_TWO);

    //视频页面控件2信息
    VideoPage2[0] = GetDlgItem(hwndDlg, IDC_VIDEO_GROUP_COLOR_CTROL);
    VideoPage2[1] = GetDlgItem(hwndDlg, IDC_VIDEO_RADIO_SAME_CONTROL);
    VideoPage2[2] = GetDlgItem(hwndDlg, IDC_VIDEO_BTN_RESET);
    VideoPage2[3] = GetDlgItem(hwndDlg, IDC_VIDEO_GROUP_COLOR_CTROL_LEFT);
    VideoPage2[4] = GetDlgItem(hwndDlg, IDC_VIDEO_GROUP_COLOR_CTROL_RIGHT);
    VideoPage2[5] = GetDlgItem(hwndDlg, IDC_VIDEO_STATIC_LEFT);
    VideoPage2[6] = GetDlgItem(hwndDlg, IDC_VIDEO_STATIC_RIGHT);
    VideoPage2[7] = GetDlgItem(hwndDlg, IDC_VIDEO_STATIC_LEFT_BRIGHTNESS);
    VideoPage2[8] = GetDlgItem(hwndDlg, IDC_VIDEO_STATIC_LEFT_CONTRAST);
    VideoPage2[9] = GetDlgItem(hwndDlg, IDC_VIDEO_STATIC_LEFT_SATURATION);
    VideoPage2[10] = GetDlgItem(hwndDlg, IDC_VIDEO_STATIC_LEFT_HUE);
    VideoPage2[11] = GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_BRIGHTNESS);
    VideoPage2[12] = GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_CONTRAST);
    VideoPage2[13] = GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_SATURATION);
    VideoPage2[14] = GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_HUE);
    VideoPage2[15] = GetDlgItem(hwndDlg, IDC_VIDEO_STATIC_RIGHT_BRIGHTNESS);
    VideoPage2[16] = GetDlgItem(hwndDlg, IDC_VIDEO_STATIC_RIGHT_CONTRAST);
    VideoPage2[17] = GetDlgItem(hwndDlg, IDC_VIDEO_STATIC_RIGHT_SATURATION);
    VideoPage2[18] = GetDlgItem(hwndDlg, IDC_VIDEO_STATIC_RIGHT_HUE);
    VideoPage2[19] = GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_BRIGHTNESS);
    VideoPage2[20] = GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_CONTRAST);
    VideoPage2[21] = GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_SATURATION);
    VideoPage2[22] = GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_HUE);
    VideoPage2[23] = GetDlgItem(hwndDlg, IDC_VIDEO_BTN_PAGE_ONE);


    //3D页面控件信息
    Set3D[0] = GetDlgItem(hwndDlg, IDC_3D_CHECK_SHOW2D);
    Set3D[1] = GetDlgItem(hwndDlg, IDC_3D_CHECK_EXCHANGE);
    Set3D[2] = GetDlgItem(hwndDlg, IDC_3D_STATIC_INPUT_MODE);
    Set3D[3] = GetDlgItem(hwndDlg, IDC_3D_STATIC_OUTPUT_MODE);
    Set3D[4] = GetDlgItem(hwndDlg, IDC_3D_STATIC_FALL_MODE_ONE);
    Set3D[5] = GetDlgItem(hwndDlg, IDC_3D_STATIC_FALL_MODE_TWO);
    Set3D[6] = GetDlgItem(hwndDlg, IDC_3D_COM_INPUT_MODE);
    Set3D[7] = GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE);
    Set3D[8] = GetDlgItem(hwndDlg, IDC_3D_COM_FALL_MODE_ONE);
    Set3D[9] = GetDlgItem(hwndDlg, IDC_3D_COM_FALL_MODE_TWO);
    //Set3D[10] = GetDlgItem(hwndDlg, IDC_3D_SLIDER_INPUT_MODE);

    //字幕页面控件信息
    Subtitle[0] = GetDlgItem(hwndDlg, IDC_SUBTITLE_CHECK_SHOWSUB);
    Subtitle[1] = GetDlgItem(hwndDlg, IDC_SUBTITLE_STATIC_STYLE);
    Subtitle[2] = GetDlgItem(hwndDlg, IDC_SUBTITLE_STATIC_SIZE);
    Subtitle[3] = GetDlgItem(hwndDlg, IDC_SUBTITLE_STATIC_COLOR);
    Subtitle[4] = GetDlgItem(hwndDlg, IDC_SUBTITLE_GROUP_LATE);
    Subtitle[5] = GetDlgItem(hwndDlg, IDC_SUBTITLE_STATIC_LATE_SHOW);
    Subtitle[6] = GetDlgItem(hwndDlg, IDC_SUBTITLE_STATIC_LATE_SHOW_SIZE);
    Subtitle[7] = GetDlgItem(hwndDlg, IDC_SUBTITLE_EDIT_LATE_SHOW);
    Subtitle[8] = GetDlgItem(hwndDlg, IDC_SUBTITLE_EDIT_LATE_SHOW_SIZE);
    Subtitle[9] = GetDlgItem(hwndDlg, IDC_SUBTITLE_COM_STYLE);
    Subtitle[10] = GetDlgItem(hwndDlg, IDC_SUBTITLE_BTN_RESET);
    Subtitle[11] = GetDlgItem(hwndDlg, IDC_SUBTITLE_COM_SIZE);
    Subtitle[12] = GetDlgItem(hwndDlg, IDC_SUBTITLE_BTN_SELECT_COLOR);
    Subtitle[13] = GetDlgItem(hwndDlg, IDC_SUBTITLE_STATIC_LATE_SHOW_MINI_SEC);
    Subtitle[14] = GetDlgItem(hwndDlg, IDC_SUBTITLE_STATIC_LATE_SHOW_CODE);

    //热键页面控件信息
//     Hotkey[0] = GetDlgItem(hwndDlg, IDC_HOTKEY_STATIC_HOTKEY);
//     Hotkey[1] = GetDlgItem(hwndDlg, IDC_HOTKEY_STATIC_BACK_FIVE);
//     Hotkey[2] = GetDlgItem(hwndDlg, IDC_HOTKEY_STATIC_EXCHANGE);
//     Hotkey[3] = GetDlgItem(hwndDlg, IDC_HOTKEY_STATIC_STEP_FIVE);
//     Hotkey[4] = GetDlgItem(hwndDlg, IDC_HOTKEY_STATIC_SUB);
//     Hotkey[5] = GetDlgItem(hwndDlg, IDC_HOTKEY_STATIC_FULL_SCREEN_MODE);
//     Hotkey[6] = GetDlgItem(hwndDlg, IDC_HOTKEY_STATIC_ADD);
//     Hotkey[7] = GetDlgItem(hwndDlg, IDC_HOTKEY_STATIC_PAUSE);
//     Hotkey[8] = GetDlgItem(hwndDlg, IDC_HOTKEY_STATIC_MEANING);
//     Hotkey[9] = GetDlgItem(hwndDlg, IDC_HOTKEY_EDIT_BACK_FIVE);
//     Hotkey[10] = GetDlgItem(hwndDlg, IDC_HOTKEY_EDIT_EXCHANGE);
//     Hotkey[11] = GetDlgItem(hwndDlg, IDC_HOTKEY_EDIT_STEP_FIVE);
//     Hotkey[12] = GetDlgItem(hwndDlg, IDC_HOTKEY_EDIT_SUB);
//     Hotkey[13] = GetDlgItem(hwndDlg, IDC_HOTKEY_EDIT_FULL_SCREEN_MODE);
//     Hotkey[14] = GetDlgItem(hwndDlg, IDC_HOTKEY_EDIT_ADD);
//     Hotkey[15] = GetDlgItem(hwndDlg, IDC_HOTKEY_EDIT_PAUSE);
//     Hotkey[16] = GetDlgItem(hwndDlg, IDC_HOTKEY_GROUP_NULL);
//     Hotkey[17] = GetDlgItem(hwndDlg, IDC_HOTKEY_BTN_RESET);
//     Hotkey[18] = GetDlgItem(hwndDlg, IDC_HOTKEY_BTN_APPLY);

    //截图页面控件信息
//     PrtSrc[0] = GetDlgItem(hwndDlg, IDC_PRTSCR_STATIC_SAVE_PATH);
//     PrtSrc[1] = GetDlgItem(hwndDlg, IDC_PRTSCR_STATIC_SAVE_FORMAT);
//     PrtSrc[2] = GetDlgItem(hwndDlg, IDC_PRTSCR_EDIT_SAVE_PATH);
//     PrtSrc[3] = GetDlgItem(hwndDlg, IDC_PRTSCR_BTN_OPEN_PATH);
//     PrtSrc[4] = GetDlgItem(hwndDlg, IDC_PRTSCR_COM_SAVE_CONT);
//     PrtSrc[5] = GetDlgItem(hwndDlg, IDC_PRTSCR_COM_SAVE_FORMAT);

    //文件关联页面控件信息
    Relation[0] = GetDlgItem(hwndDlg, IDC_RELA_GROUP_FILE_RELA);
    Relation[1] = GetDlgItem(hwndDlg, IDC_RELA_CHECK_MP4);
    Relation[2] = GetDlgItem(hwndDlg, IDC_RELA_CHECK_AVI);
    Relation[3] = GetDlgItem(hwndDlg, IDC_RELA_CHECK_RMVB);
    Relation[4] = GetDlgItem(hwndDlg, IDC_RELA_CHECK_3DV);
    Relation[5] = GetDlgItem(hwndDlg, IDC_RELA_CHECK_TS);
    Relation[6] = GetDlgItem(hwndDlg, IDC_RELA_CHECK_MKV);
    Relation[7] = GetDlgItem(hwndDlg, IDC_RELA_CHECK_WMV);
    Relation[8] = GetDlgItem(hwndDlg, IDC_RELA_CHECK_VOB);
    Relation[9] = GetDlgItem(hwndDlg, IDC_RELA_BTN_ALL_SELECT);
    Relation[10] = GetDlgItem(hwndDlg, IDC_RELA_BTN_ALL_CANCEL);
}

void SetShowCtrol(int nItem, int nSubItem)
{
    for (int nIndex = 0; nIndex < 8; nIndex++)
    {
        //         printf("%d", sizeof(AllCtrl[nIndex]) / sizeof(HWND));
        for (int i = 0; i < 30; i++)
        {
            if (nItem == nIndex)
            {
                if ( AllCtrl[nIndex][i] != NULL && IsWindow(AllCtrl[nIndex][i]))
                    ShowWindow(AllCtrl[nIndex][i], SW_SHOW);
                else
                    break;
            }
            else
            {
                if ( AllCtrl[nIndex][i] != NULL && IsWindow(AllCtrl[nIndex][i]))
                    ShowWindow(AllCtrl[nIndex][i], SW_HIDE);
                else
                    break;
            }
        }
    }
}

void LoadSetting(HWND hWnd)
{
    bool bVal = false;
    int nVal = -1;
    wchar_t szVal[100] = {0};
    char szVal_s[100] = {0};
    double dVal = - 1.00000;

//     lua_const &l = GET_CONST("AudioChannel");
//     lua_const &ggg = GET_CONST("AudioChannel");
    //int n = sizeof(TestSet) / sizeof(tagLuaSetting);
    for (int i = 0; i < sizeof(TestSet) / sizeof(tagLuaSetting); i++)
    {
        if (TestSet[i].nCtrlType == TYPE_CHECK || TestSet[i].nCtrlType == TYPE_CHECK)
        {
            lua_const& test = GET_CONST(TestSet[i].arrSettingName);
            bVal = test;
            if (bVal)
                SendMessageA(GetDlgItem(hWnd, TestSet[i].nCtrlID), BM_SETCHECK, BST_CHECKED, 0);
            else
                SendMessageA(GetDlgItem(hWnd, TestSet[i].nCtrlID), BM_SETCHECK, BST_UNCHECKED, 0);
        }
        if (TestSet[i].nCtrlType == TYPE_SLIDER)
        {
            lua_const& slider = GET_CONST(TestSet[i].arrSettingName);
            dVal = slider;
            SendMessageA(GetDlgItem(hWnd, TestSet[i].nCtrlID), TBM_SETPOS, TRUE, (WPARAM)(dVal * 1000));
        }
        //printf("%d\r\n", test);
    }
    
    lua_const& val0 = GET_CONST(TestSet[0].arrSettingName);
    nVal = val0;
    SendMessageA(GetDlgItem(hWnd, TestSet[0].nCtrlID), CB_SETCURSEL, (WPARAM)nVal, 0);

    //imgae rate
    lua_const& val1 = GET_CONST(TestSet[1].arrSettingName);
    dVal = val1;
    int nSel = -1;
    if (dVal < 0)
        nSel = 0;
    else if (abs(dVal - 4 / 3) < 0.000001)
        nSel = 1;
    else if (abs (dVal - 16 / 9) < 0.000001)
        nSel = 2;
    else if (abs(dVal - 2.35) < 0.000001)
        nSel = 3;
    SendMessageA(GetDlgItem(hWnd, TestSet[1].nCtrlID), CB_SETCURSEL, (WPARAM)nSel, 0);

    //input layout
    lua_const& val17 = GET_CONST(TestSet[17].arrSettingName);
    nVal = val17;
    if (nVal == 0x10000)
        SendMessageA(GetDlgItem(hWnd, TestSet[17].nCtrlID), CB_SETCURSEL, (WPARAM)3, 0);
    else
        SendMessageA(GetDlgItem(hWnd, TestSet[17].nCtrlID), CB_SETCURSEL, (WPARAM)nVal, 0);

    //output layout
    lua_const& val18 = GET_CONST(TestSet[18].arrSettingName);
    lua_const& val18Mask = GET_CONST("MaskMode");
    nVal = val18;
    switch (nVal)
    {
    case 0:
    case 2:
    case 3:
    case 4:
    case 5:
        nSel = nVal;
    	break;
    case 1:
        nVal = val18Mask;
        if (nVal == 0)
            nSel = 12;
        else if (nVal == 1)
            nSel = 10;
        else if (nVal == 2)
            nSel = 11;
        else if (nVal == 3)
            nSel = 1;
        break;
    case 9:
    case 10:
    case 11:
    case 12:
        nSel = nVal - 3;
        break;
    }
    SendMessageA(GetDlgItem(hWnd, TestSet[18].nCtrlID), CB_SETCURSEL, (WPARAM)nSel, 0);

    lua_const& val21 = GET_CONST(TestSet[21].arrSettingName);
    nVal = val21;
    if (nVal == 0)
        nSel = 0;
    else if (nVal == 2)
        nSel = 1;
    else if (nVal == 6)
        nSel = 2;
    else if (nVal == 8)
        nSel = 3;
    else if (nVal == 1)
        nSel = 4;
    else if (nVal == -1)
        nSel = 5;
    SendMessageA(GetDlgItem(hWnd, TestSet[21].nCtrlID), CB_SETCURSEL, (WPARAM)nSel, 0);



    lua_const& val25 = GET_CONST(TestSet[25].arrSettingName);
    wcscpy(szVal, val25);
    WideCharToMultiByte(CP_ACP, 0, szVal, -1, szVal_s, 100, NULL, NULL);
    nSel = SendMessageA(GetDlgItem(hWnd, TestSet[25].nCtrlID), CB_FINDSTRING, 0, (LPARAM)szVal_s);
    SendMessageA(GetDlgItem(hWnd, TestSet[25].nCtrlID), CB_SETCURSEL, (WPARAM)nSel, 0);

    lua_const& val26 = GET_CONST(TestSet[26].arrSettingName);
    nVal = val26;
    sprintf(szVal_s, "%d", nVal);
    SendMessageA(GetDlgItem(hWnd, TestSet[26].nCtrlID), CB_SETCURSEL, SendMessageA(GetDlgItem(hWnd, TestSet[26].nCtrlID), CB_FINDSTRING, 0, (LPARAM)szVal_s), 0);

    lua_const& val27 = GET_CONST(TestSet[27].arrSettingName);
    color = val27;

    lua_const& val28 = GET_CONST(TestSet[28].arrSettingName);
    nVal = val28;
    sprintf(szVal_s, "%d", nVal);
    SetDlgItemTextA(hWnd, TestSet[28].nCtrlID, szVal_s);

    lua_const& val29 = GET_CONST(TestSet[29].arrSettingName);
    nVal = val29;
    sprintf(szVal_s, "%d", nVal * 100);
    SetDlgItemTextA(hWnd, TestSet[29].nCtrlID, szVal_s);

    lua_const& val24 = GET_CONST(TestSet[23].arrSettingName);
    nVal = val24;
    if (nVal == 16)
        SendMessageA(GetDlgItem(hWnd, TestSet[23].nCtrlID), BM_SETCHECK, BST_CHECKED, 0);
    else if (nVal == 1)
        SendMessageA(GetDlgItem(hWnd, TestSet[23].nCtrlID), BM_SETCHECK, BST_UNCHECKED, 0);
}

void ApplySetting(HWND hWnd)
{
    BOOL bRet = FALSE;
    double dVal;
    int nVal = -1;
    char szVal[100] = {0};
    wchar_t szwVal[100] = {0};

    for (int i = 0; i < sizeof(TestSet) / sizeof(tagLuaSetting); i++)
    {
        if (TestSet[i].nCtrlType == TYPE_CHECK || TestSet[i].nCtrlType == TYPE_RADIO)
        {
            int nRet = SendMessageA(GetDlgItem(hWnd, TestSet[i].nCtrlID), BM_GETCHECK, 0, 0);
            if (nRet == BST_UNCHECKED)
                GET_CONST(TestSet[i].arrSettingName) = false;
            else if (nRet == BST_CHECKED)
                GET_CONST(TestSet[i].arrSettingName) = true;
        }
        if (TestSet[i].nCtrlType == TYPE_SLIDER)
        {
            dVal = (double)SendMessageA(GetDlgItem(hWnd, TestSet[i].nCtrlID), TBM_GETPOS, 0, 0) / 1000.0;
            GET_CONST(TestSet[i].arrSettingName) = dVal;
        }
        if (TestSet[i].nCtrlType == TYPE_EDIT)
            if (TestSet[i].nCtrlID == IDC_SUBTITLE_EDIT_LATE_SHOW_SIZE)
                GET_CONST(TestSet[i].arrSettingName) = (double)GetDlgItemInt(hWnd, TestSet[i].nCtrlID, &bRet, FALSE) / 100;
            else
                GET_CONST(TestSet[i].arrSettingName) = (double)GetDlgItemInt(hWnd, TestSet[i].nCtrlID, &bRet, FALSE);
    }
    GET_CONST(TestSet[0].arrSettingName) = SendMessageA(GetDlgItem(hWnd, TestSet[0].nCtrlID), CB_GETCURSEL, 0, 0);

    int nRet = SendMessageA(GetDlgItem(hWnd, TestSet[1].nCtrlID), CB_GETCURSEL, 0, 0);
    if (nRet == 0 )
        dVal = -1;
    else if(nRet == 1)
        dVal = (double)4 / 3; 
    else if(nRet == 2)
        dVal = (double)16 / 9; 
    else if(nRet == 3)
        dVal = (double)2.35 / 1; 
    GET_CONST(TestSet[1].arrSettingName) = dVal;

    nRet = SendMessageA(GetDlgItem(hWnd, TestSet[17].nCtrlID), CB_GETCURSEL, 0, 0);
    if (nRet == 3)
        GET_CONST(TestSet[17].arrSettingName) = 0x10000;
    else
        GET_CONST(TestSet[17].arrSettingName) = nRet;

    nRet = SendMessageA(GetDlgItem(hWnd, TestSet[18].nCtrlID), CB_GETCURSEL, 0, 0);
    switch (nRet)
    {
    case 0:
    case 2:
    case 3:
    case 4:
    case 5:
        nVal = nRet;
    	break;
    case 6:
    case 7:
    case 8:
    case 9:
        nVal = nRet + 3;
        break;
    case 1:
        nVal = 1;
        GET_CONST("MaskMode") = 3;
        break;
    case 10:
        nVal = 1;
        GET_CONST("MaskMode") = 1;
        break;
    case 11:
        nVal = 1;
        GET_CONST("MaskMode") = 2;
        break;
    case 12:
        nVal = 1;
        GET_CONST("MaskMode") = 0;
        break;
    }
    GET_CONST(TestSet[18].arrSettingName) = nVal;

    nRet = SendMessageA(GetDlgItem(hWnd, TestSet[21].nCtrlID), CB_GETCURSEL, 0, 0);
    if (nRet == 0)
        nVal = 0;
    else if (nRet == 1)
        nVal = 2;
    else if (nRet == 2)
        nVal = 6;
    else if (nRet == 3)
        nVal = 8;
    else if (nRet == 4)
        nVal = 1;
    else if (nRet == 5)
        nVal = -1;
    GET_CONST(TestSet[21].arrSettingName) = nVal;

    GetDlgItemTextA(hWnd, TestSet[25].nCtrlID, szVal, 100);
    MultiByteToWideChar(CP_ACP, 0, szVal, -1, (LPWSTR)szwVal, 100);
    GET_CONST(TestSet[25].arrSettingName) = szwVal;

    nVal = GetDlgItemInt(hWnd, TestSet[26].nCtrlID, &bRet, FALSE);
    GET_CONST(TestSet[26].arrSettingName) = nVal;

    GET_CONST(TestSet[27].arrSettingName) = color;

    nRet = SendMessageA(GetDlgItem(hWnd, TestSet[23].nCtrlID), BM_GETCHECK, 0, 0);
    if (nRet == BST_UNCHECKED)
        nVal = 1;
    else if (nRet == BST_CHECKED)
        nVal = 16;
    GET_CONST(TestSet[23].arrSettingName) = nVal;
}
