#include <windows.h>
enum BaseUIResourceType
{
	ResourceType_FileImage = 0,
	ResourceType_MemoryImage = 1,
	ResourceType_FileModel = 2,
	ResourceType_MemoryModel = 3,
	ResourceType_Font = 4,
	ResourceType_SolidColor = 5,

	ResourceType_FORCE32 = 0xffffff,
};

enum BaseUIHRESULT
{
	S_BASEUI_OK = S_OK,
	S_BASEUI_DUPLICATED = 0x0f0000 + 1,

	E_BASEUI_FAIL = E_FAIL,
};

typedef struct
{
	float left;
	float top;
	float right;
	float bottom;
} RECTF;

typedef struct
{
	int height;
	int width;
	int weight;
	int mip_levels;
	bool italic;
	DWORD char_set;
	DWORD output_precision;
	DWORD quality;
	DWORD pitch_and_family;

	wchar_t font_name[200];
}BaseUIFontDesc;