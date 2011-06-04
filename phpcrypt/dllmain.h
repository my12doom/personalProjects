// dllmain.h : 模块类的声明。

class CphpcryptModule : public CAtlDllModuleT< CphpcryptModule >
{
public :
	DECLARE_LIBID(LIBID_phpcryptLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_PHPCRYPT, "{7D93F045-1C57-41BF-A885-E4E5ED80FA5C}")
};

extern class CphpcryptModule _AtlModule;
