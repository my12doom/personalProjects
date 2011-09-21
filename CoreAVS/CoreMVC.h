#pragma  once

// {09571A4B-F1FE-4C60-9760-DE6D310C7C31}
//DEFINE_GUID(CLSID_CoreAVC, 
//			0x09571A4B, 0xF1FE, 0x4C60, 0x97, 0x60, 0xDE, 0x6D, 0x31, 0x0C, 0x7C, 0x31);
extern CLSID CLSID_CoreAVC;
extern CLSID CLSID_my12doomSource;

// {1365BE7A-C86A-473C-9A41-C0A6E82C9FA3}
DEFINE_GUID(CLSID_3dtvMPEGSource, 
			0x1365BE7A, 0xC86A, 0x473C, 0x9A, 0x41, 0xC0, 0xA6, 0xE8, 0x2C, 0x9F, 0xA3);

//static const GUID CLSID_my12doomSource = { 0x8FD7B1DE, 0x3B84, 0x4817, { 0xA9, 0x6F, 0x4C, 0x94, 0x72, 0x8B, 0x1A, 0xAE } };

// helper define
typedef HRESULT (__stdcall *pDllGetClassObject) (REFCLSID rclsid, REFIID riid, LPVOID*ppv);

// helper function
HRESULT write_property(IPropertyBag *bag, const wchar_t *property_to_write);
HRESULT ActiveCoreMVC(IBaseFilter *decoder);
HRESULT CreateCoreMVC(IBaseFilter **out);
HRESULT Create_my12doomSource(IBaseFilter **out);