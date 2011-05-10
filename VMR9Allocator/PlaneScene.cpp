//------------------------------------------------------------------------------
// File: PlaneScene.cpp
//
// Desc: DirectShow sample code - implementation of the CPlaneScene class
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "util.h"
#include "PlaneScene.h"
#include "3dvideo.h"

#define D3DFVF_CUSTOMVERTEX ( D3DFVF_XYZ /*| D3DFVF_DIFFUSE*/ | D3DFVF_TEX1 )


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPlaneScene::CPlaneScene()
{
    m_vertices[0].position = CUSTOMVERTEX::Position( 1.0f,  0.95f, 0.0f); // top right
    m_vertices[1].position = CUSTOMVERTEX::Position( 1.0f, -0.95f, 0.0f); // bottom right
    m_vertices[2].position = CUSTOMVERTEX::Position(-1.0f,  0.95f, 0.0f); // top left
    m_vertices[3].position = CUSTOMVERTEX::Position(-1.0f, -0.95f, 0.0f); // bottom left

	m_3D = false;

    // set up diffusion:
	
	/*
    m_vertices[0].color = 0xffffffff;
    m_vertices[1].color = 0xffffffff;
    m_vertices[2].color = 0xffffffff;
    m_vertices[3].color = 0xffffffff;
	*/

}

CPlaneScene::~CPlaneScene()
{

}

void CPlaneScene::set_ps(IDirect3DDevice9* d3ddev)
{
	return;

	// Assemble, create, and set the pixel shader 
	LPD3DXBUFFER pCode = NULL;
	LPD3DXBUFFER pErrorMsgs = NULL;

	HRESULT hRes = D3DXCompileShaderFromFile(L"E:\\Extras\\DirectShow\\Samples\\C++\\DirectShow\\VMR9\\VMR9Allocator\\pps.txt", NULL, NULL, "main", "ps_2_0", 0,
		&pCode, &pErrorMsgs, NULL);

	// If assembly failed, then pErrorMsgs will hopefully be non-null
	// and it will contain some useful error message from the
	// assembler.  Setting a breakpoint on the "return false" statement
	// is probably a good idea while you're debugging. 
	if ((FAILED(hRes)) && (pErrorMsgs != NULL))
	{
		unsigned char* message = 
			(unsigned char*)pErrorMsgs->GetBufferPointer();

		MessageBoxA(NULL, (LPSTR)message, "", MB_OK);
		return;
	}
	else if (FAILED(hRes))
		return;

	IDirect3DPixelShader9* ps = NULL;
	// Create the pixel shader
	int codesize = pCode->GetBufferSize();
	hRes = d3ddev->CreatePixelShader((DWORD*)pCode->GetBufferPointer(), 
		&ps);
	if (FAILED(hRes)) return;


	// Now set the pixel shader
	hRes = d3ddev->SetPixelShader(ps);
	if (FAILED(hRes)) return;

	// Done with the pixel shader interface, so release it.
	ps->Release();
	ps = NULL;
}

HRESULT 
CPlaneScene::Init(IDirect3DDevice9* d3ddev)
{
    HRESULT hr = S_OK;

    if( ! d3ddev )
        return E_POINTER;

	FAIL_RET(hr = d3ddev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
    FAIL_RET(hr = d3ddev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));
	FAIL_RET(hr = d3ddev->SetRenderState(D3DRS_LIGHTING, FALSE));
	//FAIL_RET(hr = d3ddev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE));
	FAIL_RET(hr = d3ddev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE));
	FAIL_RET(hr = d3ddev->SetRenderState(D3DRS_ALPHAREF, 0x10));
	//FAIL_RET(hr = d3ddev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER));
    //FAIL_RET(hr = d3ddev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));			// two-sided stenciling
	/*
	*/

	/*
    FAIL_RET(hr = d3ddev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP));
    FAIL_RET(hr = d3ddev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP));
	*/
    FAIL_RET(hr = d3ddev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
    FAIL_RET(hr = d3ddev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR));
    FAIL_RET(hr = d3ddev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR));

    CComPtr<IDirect3DSurface9> backBuffer;
    FAIL_RET( d3ddev->GetBackBuffer( 0, 0,
                                    D3DBACKBUFFER_TYPE_MONO,
                                    & backBuffer.p ) );

    D3DSURFACE_DESC backBufferDesc;
    backBuffer->GetDesc( & backBufferDesc );

	
    // Set the projection matrix
   /*
   D3DXMATRIX matProj;
    FLOAT fAspect = backBufferDesc.Width / 
                    (float)backBufferDesc.Height;
    D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, fAspect, 
                                1.0f, 100.0f );
    FAIL_RET( d3ddev->SetTransform( D3DTS_PROJECTION, &matProj ) );

    D3DXVECTOR3 from( 0.5f, 0.5f, -1.0f );
    D3DXVECTOR3 at( 0.5f, 0.5f, 0.0f );
    D3DXVECTOR3 up( 0.0f, 1.0f, 0.0f );

    D3DXMATRIX matView;
    D3DXMatrixLookAtLH( &matView, & from, & at, & up);
    FAIL_RET( d3ddev->SetTransform( D3DTS_VIEW, &matView ) );
	
	*/

    m_time = timeGetTime();

	set_ps(d3ddev);

	// my plain
	CAutoLock lck(&m_sec);
	m_my_surface = NULL;
	m_my_surface1 = NULL;
	hr = d3ddev->CreateRenderTarget(1680*2, 1050, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &m_my_surface, NULL);
	hr = d3ddev->CreateRenderTarget(1680*2, 1050+1, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &m_my_surface1, NULL);
	add_nv_3d(d3ddev);
    return S_OK;
}

HRESULT CPlaneScene::UnInit()
{
	m_vertexBuffer = NULL;
	CAutoLock lck(&m_sec);
	m_my_surface = NULL;
	m_my_surface1 = NULL;
	return S_OK;
}

HRESULT 
CPlaneScene::draw_to_my_surface( IDirect3DDevice9* d3ddev, IDirect3DTexture9* texture, bool cut_to_2d)
{
    HRESULT hr;

    if( !( d3ddev) )
    {
        return E_POINTER;
    }

	m_vertexBuffer = NULL;

	d3ddev->CreateVertexBuffer(sizeof(m_vertices),D3DUSAGE_WRITEONLY,D3DFVF_CUSTOMVERTEX,D3DPOOL_MANAGED,& m_vertexBuffer.p, NULL );

    if( m_vertexBuffer == NULL )
    {
        return D3DERR_INVALIDCALL;
    }

	static int n = 0;
	n++;
	// set up texture coordinates
	if (nv3d_enabled && nv3d_actived &&!cut_to_2d)
	//if (n<24)
	{
		m_vertices[0].tu = 1.0f; m_vertices[2].tv = 0.0f; // low right
		m_vertices[1].tu = 1.0f; m_vertices[3].tv = 1.0f; // high right
	}
	else
	{
		m_vertices[0].tu = 0.5f; m_vertices[2].tv = 0.0f; // low right
		m_vertices[1].tu = 0.5f; m_vertices[3].tv = 1.0f; // high right
	}
	m_vertices[2].tu = 0.0f; m_vertices[0].tv = 0.0f; // low left
	m_vertices[3].tu = 0.0f; m_vertices[1].tv = 1.0f; // high left

	// get the difference in time
    DWORD dwCurrentTime;
    dwCurrentTime = timeGetTime();
    double difference = m_time - dwCurrentTime ;
    
    // figure out the rotation of the plane
    float x = (float) ( -cos(difference / 2000.0 ) ) ;
    float y = (float) ( cos(difference / 2000.0 ) ) ;
    float z = (float) ( sin(difference / 2000.0 ) ) ;

    // update the two rotating vertices with the new position
    //m_vertices[0].position = CUSTOMVERTEX::Position(x,  y, z);   // top left
    //m_vertices[3].position = CUSTOMVERTEX::Position(-x, -y, -z); // bottom right

    // Adjust the color so the blue is always on the bottom.
    // As the corner approaches the bottom, get rid of all the other
    // colors besides blue
    DWORD mask0 = (DWORD) (255 * ( ( y + 1.0  )/ 2.0 ));
    DWORD mask3 = (DWORD) (255 * ( ( -y + 1.0  )/ 2.0 ));
    //m_vertices[0].color = 0xff0000ff | ( mask0 << 16 ) | ( mask0 << 8 );
    //m_vertices[3].color = 0xff0000ff | ( mask3 << 16 ) | ( mask3 << 8 );

    // write the new vertex information into the buffer
    void* pData;
    FAIL_RET( m_vertexBuffer->Lock(0,sizeof(pData), &pData,0) );
    memcpy(pData,m_vertices,sizeof(m_vertices));
    FAIL_RET( m_vertexBuffer->Unlock() );  

	CAutoLock lck(&m_sec);
	m_3D = false;
	d3ddev->BeginScene();
	d3ddev->SetRenderTarget(0, m_my_surface);
    // clear the scene so we don't have any articats left
    d3ddev->Clear( 0L, NULL, D3DCLEAR_TARGET, 
                   D3DCOLOR_XRGB(255,0,0), 1.0f, 0L );

    //FAIL_RET( d3ddev->BeginScene() );
    if (texture)
		FAIL_RET( d3ddev->SetTexture( 0, texture));
		FAIL_RET( d3ddev->SetTexture( 1, texture));

    //FAIL_RET(hr = d3ddev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE));
    //FAIL_RET(hr = d3ddev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE));
    //FAIL_RET(hr = d3ddev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE));
    //FAIL_RET(hr = d3ddev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE));

    FAIL_RET( d3ddev->SetStreamSource(0, m_vertexBuffer.p, 0, sizeof(CPlaneScene::CUSTOMVERTEX)  ) );            //set next source ( NEW )
    FAIL_RET( d3ddev->SetFVF( D3DFVF_CUSTOMVERTEX ) );
    FAIL_RET( d3ddev->DrawPrimitive(D3DPT_TRIANGLESTRIP,0,2) );  //draw quad 
    //FAIL_RET( d3ddev->SetTexture( 0, NULL));
	
	//make_backbuffer_3d(d3ddev);

	RECT tar = {0,0, 1680*2, 1050-0};
	hr = d3ddev->StretchRect(m_my_surface, NULL, m_my_surface1, &tar, D3DTEXF_NONE);

	// get back buffer and blit to my surface 
    FAIL_RET( d3ddev->EndScene());

    return hr;
}

int CPlaneScene::copy_to_back_buffer(IDirect3DDevice9* d3ddev)
{
	CAutoLock lck(&m_sec);
	//if(!m_3D)
	//	add_nv_3d(d3ddev);

	// Stretch my surface back to back buffer.
	RECT tar = {0,0, 1680*2, 1050-0};
	CComPtr<IDirect3DSurface9> back_buffer;
	HRESULT hr = d3ddev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);
	hr = d3ddev->StretchRect(m_my_surface1, &tar, back_buffer, NULL, D3DTEXF_NONE);

	return S_OK;
}

void CPlaneScene::add_nv_3d(IDirect3DDevice9* d3ddev)
{
	CAutoLock lck(&m_sec);
	if (m_my_surface == NULL || m_my_surface1 == NULL)
		return;

	//and add stereo tag
	char tmp[256];
	sprintf(tmp, "(%d)Adding NV Tag.", GetCurrentThreadId());
	OutputDebugStringA(tmp);
	int width = 1680;
	int height = 1050;
	D3DLOCKED_RECT lr;
	RECT lock_tar={0, height, 1680*2, height+1};
	m_my_surface1->LockRect(&lr,&lock_tar,0);
	LPNVSTEREOIMAGEHEADER pSIH = (LPNVSTEREOIMAGEHEADER)(((unsigned char *) lr.pBits) + (lr.Pitch * (0)));	
	pSIH->dwSignature = NVSTEREO_IMAGE_SIGNATURE;
	pSIH->dwBPP = 32;
	pSIH->dwFlags = SIH_SIDE_BY_SIDE;
	pSIH->dwWidth = width;
	pSIH->dwHeight = height;
	m_my_surface1->UnlockRect();

	m_3D = true;
}
/*
HRESULT 
CPlaneScene::DrawScene( IDirect3DDevice9* d3ddev,
                        IDirect3DSurface9* surface ) 
{
	// blit to my surface 
	RECT tar = {0,96, 1680*2, 1050-96};
	HRESULT hr = d3ddev->StretchRect(surface, NULL, m_my_surface, &tar, D3DTEXF_LINEAR);

	//and add stereo tag
	int width = 1680;
	int height = 1050;
	D3DLOCKED_RECT lr;
	m_my_surface->LockRect(&lr,NULL,0);

	LPNVSTEREOIMAGEHEADER pSIH = (LPNVSTEREOIMAGEHEADER)(((unsigned char *) lr.pBits) + (lr.Pitch * (height)));	
	pSIH->dwSignature = NVSTEREO_IMAGE_SIGNATURE;
	pSIH->dwBPP = 32;
	pSIH->dwFlags = SIH_SIDE_BY_SIDE;
	//pSIH->dwFlags = SIH_SWAP_EYES; // Src image has left on left and right on right, thats why this flag is not needed.
	pSIH->dwWidth = width;
	pSIH->dwHeight = height;
	
	m_my_surface->UnlockRect();

    // clear the window to a deep blue
    hr = d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 40, 100), 1.0f, 0);

	// begin
	hr = d3ddev->BeginScene();

	// Get the Backbuffer then Stretch the Surface on it.
	CComPtr<IDirect3DSurface9> back_buffer;
	hr = d3ddev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);
	hr = d3ddev->StretchRect(m_my_surface, NULL, back_buffer, NULL, D3DTEXF_NONE);

	hr = back_buffer->LockRect(&lr,NULL,0);

	d3ddev->EndScene();

	return S_OK;
}

*/
void
CPlaneScene::SetSrcRect( float fTU, float fTV )
{
	/*
	if (!nv3d_actived || !nv3d_enabled)
		fTV /= 2;
    m_vertices[0].tu = 0.0f; m_vertices[0].tv = 0.0f; // low left
    m_vertices[1].tu = 0.0f; m_vertices[1].tv = fTV;  // high left
    m_vertices[2].tu = fTU;  m_vertices[2].tv = 0.0f; // low right
    m_vertices[3].tu = fTU;  m_vertices[3].tv = fTV;  // high right
	*/
}