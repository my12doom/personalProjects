#include "SampleEngine.h"
#include "d3dx9.h"

#pragma comment(lib, "d3dx9.lib")

class ResourceTypeTester : public ID3D9UIResource
{
public:
	int m_type;
};

class SampleImageResource : public ResourceTypeTester
{
public:
	SampleImageResource(const wchar_t* filename, IDirect3DDevice9 *device, int width, int height);
	~SampleImageResource();

	HRESULT CommitGPU();
	HRESULT DecommitGPU();

	HRESULT Release();


	IDirect3DDevice9 *m_device;
	CComPtr<IDirect3DTexture9> m_texture_cpu;
	CComPtr<IDirect3DTexture9> m_texture_gpu;
	bool m_commited;
	int m_width;
	int m_height;
};

class SampleFontResource : public ResourceTypeTester
{
public:
	SampleFontResource(IDirect3DDevice9 *device, void *data);
	~SampleFontResource();

	HRESULT CommitGPU();
	HRESULT DecommitGPU();
	HRESULT Release();


	IDirect3DDevice9 *m_device;
	CComPtr<ID3DXFont> m_font;
};

class RootObject : public D3D9UIObject
{
public:
	RootObject(ID3D9UIEngine * engine);
	~RootObject();

	virtual HRESULT RenderThis(bool HitTest);
	friend class SampleEngine;
protected:
};


struct MyVertex
{
	float x , y, z;
	float w;
	float tu, tv;
};
ID3D9UIResource *resource = NULL;
const DWORD FVF_Flags = D3DFVF_XYZRHW | D3DFVF_TEX1;

void SampleEngine::init_d3d9()
{
	m_D3D = Direct3DCreate9( D3D_SDK_VERSION );

	D3DDISPLAYMODE d3ddm;

	m_D3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm );

	ZeroMemory( &m_pp, sizeof(m_pp) );

	m_pp.Windowed               = TRUE;
	m_pp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
	m_pp.BackBufferFormat       = d3ddm.Format;
	//g_d3dpp.EnableAutoDepthStencil = TRUE;
	//g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	m_pp.PresentationInterval   = D3DPRESENT_INTERVAL_ONE;
	m_pp.BackBufferWidth = 1920;
	m_pp.BackBufferHeight = 1080;

	m_D3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
		g_hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
		&m_pp, &m_device );

	m_device->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE);
	m_device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	m_device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	m_device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	m_device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	m_device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

	m_device->GetSwapChain(0, &m_swap);
	HRESULT hr = m_device->CreateRenderTarget(1, 1, m_pp.BackBufferFormat, D3DMULTISAMPLE_NONE, 0, FALSE, &m_hittest_gpu, NULL);
	hr = m_device->CreateOffscreenPlainSurface(1, 1, m_pp.BackBufferFormat, D3DPOOL_SYSTEMMEM, &m_hittest_cpu, NULL);

	LPD3DXBUFFER pCode = NULL;
	LPD3DXBUFFER pErrorMsgs = NULL;
	hr = E_FAIL;
 	hr = D3DXCompileShaderFromFileA("lanczos.txt", NULL, NULL, "hittest", "ps_2_0", D3DXSHADER_OPTIMIZATION_LEVEL3,  &pCode, &pErrorMsgs, NULL);
	if (pErrorMsgs != NULL)
	{
		unsigned char* message = (unsigned char*)pErrorMsgs->GetBufferPointer();
		OutputDebugStringA((LPSTR)message);
	}
	if ((FAILED(hr)))
	{
		OutputDebugStringA("error : compile failed.\n");
	}
	else
	{
		m_device->CreatePixelShader((DWORD*)pCode->GetBufferPointer(), &m_lanczos);
	}
}

void SampleEngine::render()
{
	//CAutoLock lck(&m_cs);

	// resize when needed
	CComPtr<IDirect3DSurface9> rt;
	m_swap->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &rt);
	D3DSURFACE_DESC desc;
	rt->GetDesc( &desc);

	RECT rect;
	GetClientRect(g_hWnd, &rect);
	if ((rect.right - rect.left > LONG(desc.Width) || rect.bottom - rect.top > LONG(desc.Height))
		&& rect.right != rect.left && rect.bottom != rect.top)
	{
		// resize
		m_swap = NULL;
		rt = NULL;

		m_pp.BackBufferWidth = rect.right - rect.left;
		m_pp.BackBufferHeight = rect.bottom - rect.top;
		m_device->CreateAdditionalSwapChain(&m_pp, &m_swap);
		m_swap->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &rt);
		rt->GetDesc( &desc);

// 		char tmp[200];
// 		sprintf(tmp, "resizing to %dx%d", m_pp.BackBufferWidth, m_pp.BackBufferHeight);
// 		OutputDebugStringA(tmp);
	}


	m_device->SetRenderTarget(0, rt);
	m_device->BeginScene();

	bool mouse_in_client_region = false;
	POINT mouse = {-1,-1};
	GetCursorPos(&mouse);
	ScreenToClient(g_hWnd, &mouse);
	GetClientRect(g_hWnd, &rect);
	if (mouse.x > 0 && mouse.x < rect.right
		&& mouse.y > 0 && mouse.y < rect.bottom)
	{
		// hittest drawing on need
		mouse_in_client_region = true;

		m_device->Clear( 0, NULL, D3DCLEAR_TARGET,
			D3DCOLOR_ARGB(255,255,255,255), 1.0f, 0 );

		D3DVIEWPORT9 vp;
		vp.X      = mouse.x;
		vp.Y      = mouse.y;
		vp.Width  = 1;
		vp.Height = 1;
		vp.MinZ   = 0.0f;
		vp.MaxZ   = 1.0f;

//   		m_device->SetViewport(&vp);
		((RootObject*)m_root)->Render(true);


		RECT mouse_rect = {mouse.x, mouse.y, mouse.x+1, mouse.y+1};
		RECT target_rect = {0,0,1,1};
		HRESULT hr = m_device->StretchRect(rt, &mouse_rect, m_hittest_gpu, &target_rect, D3DTEXF_LINEAR);

		vp.X = 0;
		vp.Y = 0;
		vp.Width = m_pp.BackBufferWidth;
		vp.Height = m_pp.BackBufferHeight;
//  		m_device->SetViewport(&vp);
	}
	else
	{
		//printf("mouse out of client region\n");
	}

	// normal drawing
	m_device->Clear( 0, NULL, D3DCLEAR_TARGET,
		D3DCOLOR_COLORVALUE(0.35f,0.53f,0.7f,1.0f), 1.0f, 0 );
	((RootObject*)m_root)->Render(false);


	// present
	m_device->EndScene();

	if (m_pp.Windowed)
		m_swap->Present(&rect, NULL, g_hWnd, NULL, NULL);
	else
		m_swap->Present(NULL, NULL, NULL, NULL, NULL);

	// read back after present
	if (mouse_in_client_region)
	{
		HRESULT hr = m_device->GetRenderTargetData(m_hittest_gpu, m_hittest_cpu);
		D3DLOCKED_RECT locked;
		m_hittest_cpu->LockRect(&locked, NULL, NULL);
		m_last_hittest_result = *(int*)locked.pBits & 0xffffff;
		m_hittest_cpu->UnlockRect();
	}
}

HRESULT SampleEngine::RenderNow()
{
	render();
	return S_OK;
}

SampleEngine::SampleEngine(HWND hwnd)
:D3D9UIEngine(hwnd)
{
	m_root = new RootObject(this);
	g_hWnd = hwnd;
	m_last_hittest_result = -1;
	init_d3d9();
	m_exit = false;
	m_rendering_thread = CreateThread(NULL, NULL, rendering_thread, this, NULL, NULL);
}

SampleEngine::~SampleEngine()
{
	m_exit = true;
	WaitForSingleObject(m_rendering_thread, INFINITE);

	delete m_root;
}

DWORD WINAPI SampleEngine::rendering_thread(LPVOID param)
{
	SampleEngine *_this = (SampleEngine*)param;

	SetThreadPriority(GetCurrentThread(), IDLE_PRIORITY_CLASS);

	while (!_this->m_exit)
		_this->render();

	return 0;
}

// device lost / device reset handling need to be called from window proc thread
HRESULT SampleEngine::HandleDeviceState()
{
	return S_FALSE;
}


HRESULT SampleEngine::LoadResource(ID3D9UIResource **out, void *data, int type)
{
	if (out == NULL)
		return E_POINTER;

	switch(type)
	{
	case ResourceType_FileImage:
		*out = new SampleImageResource((wchar_t*)data, m_device, D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2);
		break;
	case ResourceType_Font:
		*out = new SampleFontResource(m_device, data);
		break;
	default:
		return E_UNEXPECTED;
	}

	return S_OK;
}

HRESULT SampleEngine::Draw(ID3D9UIResource *resource, RECT *src, RECT *dst, int hittest_id)
{
	HRESULT hr;

	if (!resource)
		return E_INVALIDARG;


	SampleImageResource *res = (SampleImageResource*)resource;
	res->CommitGPU();

	// Rect
	RECT s = {0,0,res->m_width, res->m_height};
	if (src)
		s = *src;

	RECT d = {0,0,m_pp.BackBufferWidth, m_pp.BackBufferHeight};
	if (dst)
		d = *dst;

	// coordinate
	MyVertex vertex[4];	
	vertex[0].x = (float)d.left;
	vertex[0].y = (float)d.top;
	vertex[1].x = (float)d.right;
	vertex[1].y = (float)d.top;
	vertex[2].x = (float)d.left;
	vertex[2].y = (float)d.bottom;
	vertex[3].x = (float)d.right;
	vertex[3].y = (float)d.bottom;
	for(int i=0;i <4; i++)
	{
		vertex[i].x -= 0.5f;
		vertex[i].y -= 0.5f;
		vertex[i].z = 1.0f;
		vertex[i].w = 1.0f;
	}

	vertex[0].tu = (float)s.left / res->m_width;
	vertex[0].tv = (float)s.top / res->m_height;
	vertex[1].tu = (float)s.right / res->m_width;
	vertex[1].tv = (float)s.top / res->m_height;
	vertex[2].tu = (float)s.left / res->m_width;
	vertex[2].tv = (float)s.bottom / res->m_height;
	vertex[3].tu = (float)s.right / res->m_width;
	vertex[3].tv = (float)s.bottom / res->m_height;
	
	// resize rate
	if (hittest_id != -1)
	{
		m_device->SetPixelShader(m_lanczos);
		BYTE *p = (BYTE*)&hittest_id;
		float color[4] = {(float)p[2]/255, (float)p[1]/255, (float)p[0]/255, 0};
		m_device->SetPixelShaderConstantF(0, color, 1);
	}

	// draw it
	m_device->SetTexture(0, res->m_texture_gpu);
	hr = m_device->SetFVF( FVF_Flags );
	hr = m_device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertex, sizeof(MyVertex));

	m_device->SetPixelShader(NULL);
	return S_OK;
}

HRESULT SampleEngine::Draw(ID3D9UIResource *resource, RECTF *src, RECTF *dst, int hittest_id)
{
	HRESULT hr;

	if (!resource)
		return E_INVALIDARG;

	ResourceTypeTester *t = (ResourceTypeTester*)resource;
	if (t->m_type != ResourceType_FileImage)
		return E_INVALIDARG;


	SampleImageResource *res = (SampleImageResource*)resource;
	res->CommitGPU();

	// Rect
	RECTF s = {0,0,(float)res->m_width, (float)res->m_height};
	if (src)
		s = *src;

	RECTF d = {0,0,(float)m_pp.BackBufferWidth, (float)m_pp.BackBufferHeight};
	if (dst)
		d = *dst;

	// coordinate
	MyVertex vertex[4];	
	vertex[0].x = (float)d.left;
	vertex[0].y = (float)d.top;
	vertex[1].x = (float)d.right;
	vertex[1].y = (float)d.top;
	vertex[2].x = (float)d.left;
	vertex[2].y = (float)d.bottom;
	vertex[3].x = (float)d.right;
	vertex[3].y = (float)d.bottom;
	for(int i=0;i <4; i++)
	{
		vertex[i].x -= 0.5f;
		vertex[i].y -= 0.5f;
		vertex[i].z = 1.0f;
		vertex[i].w = 1.0f;
	}

	vertex[0].tu = (float)s.left / res->m_width;
	vertex[0].tv = (float)s.top / res->m_height;
	vertex[1].tu = (float)s.right / res->m_width;
	vertex[1].tv = (float)s.top / res->m_height;
	vertex[2].tu = (float)s.left / res->m_width;
	vertex[2].tv = (float)s.bottom / res->m_height;
	vertex[3].tu = (float)s.right / res->m_width;
	vertex[3].tv = (float)s.bottom / res->m_height;

	// draw it
	m_device->SetTexture(0, res->m_texture_gpu);
	hr = m_device->SetFVF( FVF_Flags );
	hr = m_device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertex, sizeof(MyVertex));

	m_device->SetPixelShader(NULL);
	return S_OK;
}

HRESULT SampleEngine::DrawText(ID3D9UIResource *resource, const wchar_t *text, RECT *rect, DWORD format, DWORD color)
{
	if (!resource)
		return E_INVALIDARG;
	ResourceTypeTester *t = (ResourceTypeTester*)resource;
	if (t->m_type != ResourceType_Font)
		return E_INVALIDARG;

	SampleFontResource* font = (SampleFontResource*)resource;

	return font->m_font->DrawTextW(NULL, text, wcslen(text), rect, format, color  );
}

HRESULT SampleEngine::Hittest(int x, int y, ID3D9UIObject **hit)
{
	return E_NOTIMPL;
}

HRESULT SampleEngine::Hittest(ID3D9UIObject **hit)
{
	return ((RootObject*)m_root)->GetHittestObject(m_last_hittest_result, hit);
}

HRESULT SampleEngine::ReleaseResource(ID3D9UIResource *res)
{
	return E_NOTIMPL;
}

// RootObject
RootObject::RootObject(ID3D9UIEngine * engine)
:D3D9UIObject(engine)
{
}

RootObject::~RootObject()
{

}

HRESULT RootObject::RenderThis(bool HitTest)
{
	return S_OK;
}

// SampleFontResource
SampleFontResource::SampleFontResource(IDirect3DDevice9 *device, void *p)
{
	m_type = ResourceType_Font;

	BaseUIFontDesc *f = (BaseUIFontDesc*)p;

	D3DXCreateFontW(device, f->height, f->width, f->weight, f->mip_levels,
					f->italic, f->char_set, f->output_precision, f->quality, 
					f->pitch_and_family, f->font_name, &m_font);
}

SampleFontResource::~SampleFontResource()
{
	Release();
}

HRESULT SampleFontResource::Release()
{
	m_font = NULL;
	return S_OK;
}

// SampleImageResource
SampleImageResource::SampleImageResource(const wchar_t* filename, IDirect3DDevice9 *device, int width, int height)
{
	m_width = width;
	m_height = height;
	m_device = device;
	m_commited = false;
	m_type = ResourceType_FileImage;

	HRESULT hr = D3DXCreateTextureFromFileExW( m_device, filename, width, height, 1, NULL, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, D3DX_DEFAULT,
		D3DX_DEFAULT,0,0,0, &m_texture_cpu );

	if (width == D3DX_DEFAULT_NONPOW2 || height == D3DX_DEFAULT_NONPOW2 && m_texture_cpu)
	{
		D3DSURFACE_DESC desc;
		m_texture_cpu->GetLevelDesc(0, &desc);

		m_width = desc.Width;
		m_height = desc.Height;
	}

	hr = E_FAIL;
}
SampleImageResource::~SampleImageResource()
{
}

HRESULT SampleImageResource::CommitGPU()
{
	if (m_commited)
		return S_OK;

	HRESULT hr;
	hr = m_device->CreateTexture(m_width, m_height, 0, D3DUSAGE_AUTOGENMIPMAP, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_texture_gpu, NULL);
	CComPtr<IDirect3DSurface9> src;
	CComPtr<IDirect3DSurface9> dst;

	m_texture_cpu->GetSurfaceLevel(0, &src);
	m_texture_gpu->GetSurfaceLevel(0, &dst);
	hr = m_device->UpdateSurface(src, NULL, dst, NULL);
	m_commited = true;

	return S_OK;
}
HRESULT SampleImageResource::DecommitGPU()
{
	if (!m_commited)
		return S_OK;

	m_texture_gpu = NULL;
	m_commited = false;

	return S_OK;
}

HRESULT SampleImageResource::Release()
{
	m_texture_gpu = NULL;
	m_texture_cpu = NULL;

	return S_OK;
}

ID3D9UIEngine * create_engine(HWND hwnd)
{
	return new SampleEngine(hwnd);
}