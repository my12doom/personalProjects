#include <d3d9.h>
#include <streams.h>
#include <atlbase.h>

class CTextureAllocator;

typedef struct stuct_PooledTexture
{
	IDirect3DTexture9 *texture;
	D3DLOCKED_RECT locked_rect;

	int width;
	int height;
	D3DFORMAT format;
	D3DPOOL pool;
	DWORD usage;

	HRESULT hr;
	IDirect3DDevice9 *creator;
}PooledTexture;

class CPooledTexture : public PooledTexture
{
public:
	CPooledTexture(CTextureAllocator *pool);
	~CPooledTexture();

	HRESULT Unlock();

protected:
	CTextureAllocator *m_allocator;
};

class CTextureAllocator
{
public:
	CTextureAllocator(IDirect3DDevice9 *device);
	~CTextureAllocator();
	HRESULT CreateTexture(int width, int height, DWORD flag, D3DFORMAT format, D3DPOOL pool, CPooledTexture **out);
	HRESULT DeleteTexture(CPooledTexture *texture);
	HRESULT DestroyPool(D3DPOOL pool2destroy);
protected:
	IDirect3DDevice9 *m_device;
	PooledTexture m_pool[1024];
	int m_pool_count; // = 0
	CCritSec m_pool_lock;
};