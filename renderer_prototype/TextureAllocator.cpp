#include "TextureAllocator.h"

extern int lockrect_texture;
extern __int64 lockrect_texture_cycle;

// texture class
CPooledTexture::~CPooledTexture()
{
	m_allocator->DeleteTexture(this);
}

CPooledTexture::CPooledTexture(CTextureAllocator *pool)
{
	m_allocator = pool;
}

HRESULT CPooledTexture::Unlock()
{
	locked_rect.pBits = NULL;
	return texture->UnlockRect(0);
}

// texture pool
CTextureAllocator::CTextureAllocator(IDirect3DDevice9 *device):
m_pool_count(0),
m_device(device)
{

}
CTextureAllocator::~CTextureAllocator()
{
	DestroyPool(D3DPOOL_DEFAULT);
	DestroyPool(D3DPOOL_MANAGED);
	DestroyPool(D3DPOOL_SYSTEMMEM);
}
HRESULT CTextureAllocator::CreateTexture(int width, int height, DWORD usage, D3DFORMAT format, D3DPOOL pool, CPooledTexture **out)
{
	if (out == NULL)
		return E_POINTER;

	CPooledTexture *& o = *out;
	o = new CPooledTexture(this);

	{
		// find in pool
		CAutoLock lck(&m_pool_lock);
		for(int i=0; i<m_pool_count; i++)
		{
			PooledTexture &t = m_pool[i];
			if (t.width == width && t.height == height && t.pool == pool && t.format == format)
			{
				*(PooledTexture*)o = t;
				m_pool_count --;
				memcpy(m_pool+i, m_pool+i+1, (m_pool_count-i)*sizeof(PooledTexture));
				//for(int j=i; j<m_pool_count; j++)
				//	m_pool[j] = m_pool[j+1];

				return S_OK;
			}
		}
	}

	// no match, just create new one and lock it
	o->width = width;
	o->height = height;
	o->pool = pool;
	o->usage = usage;
	o->format = format;
	o->creator = m_device;
	o->hr = m_device->CreateTexture(width, height, 1, usage, format, pool, &o->texture, NULL);
	if (FAILED(o->hr))
		return o->hr;
	if (pool == D3DPOOL_SYSTEMMEM || usage & D3DUSAGE_DYNAMIC)
		o->hr = o->texture->LockRect(0, &o->locked_rect, NULL, usage & D3DUSAGE_DYNAMIC ? D3DLOCK_DISCARD : NULL);

	lockrect_texture ++;

	if (FAILED(o->hr))
	{
		o->texture->Release();
		o->texture = NULL;
	}

	return o->hr;
}
HRESULT CTextureAllocator::DeleteTexture(CPooledTexture *texture)
{
	if (FAILED(texture->hr))
		return S_OK;

	if (texture->locked_rect.pBits == NULL && (texture->pool == D3DPOOL_SYSTEMMEM || texture->usage & D3DUSAGE_DYNAMIC))
	{
		LARGE_INTEGER li, l2;
		QueryPerformanceCounter(&li);
		texture->hr = texture->texture->LockRect(0, &texture->locked_rect, NULL, texture->usage & D3DUSAGE_DYNAMIC ? D3DLOCK_DISCARD : NULL);
		QueryPerformanceCounter(&l2);
		lockrect_texture ++;
		lockrect_texture_cycle += l2.QuadPart - li.QuadPart;
	}
	if (FAILED(texture->hr))
	{
		texture->texture->Release();
		return S_FALSE;
	}
	
	CAutoLock lck(&m_pool_lock);
	m_pool[m_pool_count++] = *texture;

	return S_OK;
}
HRESULT CTextureAllocator::DestroyPool(D3DPOOL pool2destroy)
{
	CAutoLock lck(&m_pool_lock);
	int i = 0;
	int j = 0;
	int c = m_pool_count;
	while (i<c)
	{
		if (m_pool[i].pool == pool2destroy)
		{
			m_pool[i++].texture->Release();
			m_pool_count--;
		}
		else
			m_pool[j++] = m_pool[i++];
	}

	/*
	for(int i=0; i<m_pool_count; i++)
		m_pool[i].texture->Release();

	m_pool_count = 0;
	*/

	return S_OK;
}
