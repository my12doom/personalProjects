#include <Windows.h>
#include "allocator.h"

class session
{
public:
	session();
	virtual ~session();

	virtual HRESULT add_one_frame();
};