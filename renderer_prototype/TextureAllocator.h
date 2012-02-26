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

typedef struct stuct_PooledSurface
{
	IDirect3DSurface9 *surface;
	D3DLOCKED_RECT locked_rect;

	int width;
	int height;
	D3DFORMAT format;
	D3DPOOL pool;

	HRESULT hr;
	IDirect3DDevice9 *creator;
}PooledSurface;

class CPooledTexture : public PooledTexture
{
public:
	CPooledTexture(CTextureAllocator *pool);
	~CPooledTexture();

	HRESULT get_first_level(IDirect3DSurface9 **out);
	HRESULT Unlock();

protected:
	CTextureAllocator *m_allocator;
};

class CPooledSurface : public PooledSurface
{
public:
	CPooledSurface(CTextureAllocator *pool);
	~CPooledSurface();

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
	HRESULT CreateOffscreenSurface(int width, int height, D3DFORMAT format, D3DPOOL pool, CPooledSurface **out);
	HRESULT DeleteSurface(CPooledSurface * surface);
	HRESULT DestroyPool(D3DPOOL pool2destroy);
	HRESULT UpdateTexture(CPooledTexture *src, CPooledTexture *dst);
protected:
	IDirect3DDevice9 *m_device;
	PooledTexture m_texture_pool[1024];
	PooledSurface m_surface_pool[1024];
	int m_texture_count; // = 0
	int m_surface_count; // = 0
	CCritSec m_texture_pool_lock;
	CCritSec m_surface_pool_lock;
};