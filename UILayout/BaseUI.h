#pragma once
#include "BaseUITypes.h"

const int max_child_count = 32;

class ID3D9UIObject;
class ID3D9UIEngine;
class ID3D9UIResource;

class ID3D9UIObject
{
public:
	virtual ~ID3D9UIObject() {};
	virtual HRESULT GetParent(ID3D9UIObject **parent) PURE;
	virtual HRESULT AddChild(ID3D9UIObject *child) PURE;
	virtual HRESULT RemoveChild(int pos) PURE;
	virtual HRESULT GetChild(int pos, ID3D9UIObject **child) PURE;
	virtual HRESULT GetChildCount(int *count) PURE;
	virtual HRESULT GetID(int *id) PURE;
};

class ID3D9UIEngine
{
public:
	virtual ~ID3D9UIEngine(){};
	// Get the root UIObject
	// it should be just a empty object that covers all client area, and draws nothing

	virtual HRESULT RenderNow() PURE;

	virtual HRESULT GetUIRoot(ID3D9UIObject **root) PURE;
	virtual HRESULT Hittest(int x, int y, ID3D9UIObject **hit) PURE;
	virtual HRESULT Hittest(ID3D9UIObject **hit) PURE;

	// Create a resource based on resource type;
	// returns S_OK on success, internal failure code on failure.
	virtual HRESULT LoadResource(ID3D9UIResource **out, void *data, int type) PURE;


	// A simple draw function, Draw src part of source to dst part of object rect
	// resource: Texture resource to draw, can not be NULL
	// src: source rect, A NULL for this parameter cause the entire resource to be used
	// dst: destination rect, A NULL for this parameter cause the entire object rect to be used
	//    : you can use larger than entire object rect, but only area within object rect will be drawn by default
	// hittest_id: color override for hittest, -1 means no override
	// returns E_POINTER if resource==NULL, S_OK on success, internal failure code on failure.
	virtual HRESULT Draw(ID3D9UIResource *resource, RECT *src, RECT *dst, int hittest_id) PURE;

	virtual HRESULT Draw(ID3D9UIResource *resource, RECTF *src, RECTF *dst, int hittest_id) PURE;


	// A simple text draw function, Draw text in a rect
	// resource: font resource to draw, can not be NULL
	// rect: destination rect, A NULL for this parameter cause the entire object rect to be used
	//    : you can use larger than entire object rect, but only area within object rect will be drawn by default
	// hittest_id: color override for hittest, -1 means no override
	// returns E_POINTER if resource==NULL, S_OK on success, internal failure code on failure.
	virtual HRESULT DrawText(ID3D9UIResource *resource, const wchar_t *text, RECT *rect, DWORD format, DWORD color) PURE;


	// 
	virtual HRESULT RegisterObject(int *out, ID3D9UIObject *obj) PURE;

	//
	virtual HRESULT ReleaseResource(ID3D9UIResource *res) PURE;

	// handle device state
	virtual HRESULT HandleDeviceState() PURE;

};

class ID3D9UIResource
{
public:
	virtual HRESULT Release() PURE;
};


class D3D9UIObject;
class D3D9UIEngine;

class D3D9UIEngine : public ID3D9UIEngine
{
public:

	D3D9UIEngine(HWND hwnd);
	virtual ~D3D9UIEngine();

protected:
	// ID3D9UIEngine staff
	virtual HRESULT RegisterObject(int *out, ID3D9UIObject *obj);
	virtual HRESULT GetUIRoot(ID3D9UIObject **root);
	virtual HRESULT RenderNow(){return E_NOTIMPL;}

	// variables
	int m_unique_id;
	D3D9UIObject *m_root;
};

class D3D9UIObject : public ID3D9UIObject
{
public:
	D3D9UIObject(ID3D9UIEngine *engine);
	~D3D9UIObject();
protected:

	// ID3D9UIObject staff
	virtual HRESULT GetParent(ID3D9UIObject **parent);
	virtual HRESULT AddChild(ID3D9UIObject *child);
	virtual HRESULT RemoveChild(int pos);
	virtual HRESULT GetChild(int pos, ID3D9UIObject **child);
	virtual HRESULT GetChildCount(int *count);
	virtual HRESULT GetID(int *id);

	// rendering staff
	virtual HRESULT RenderThis(bool HitTest) PURE;
	virtual HRESULT Render(bool HitTest);

	// message handling staff
	virtual HRESULT GetHittestObject(int id, ID3D9UIObject **hit);
	virtual HRESULT DefaultFocusHandling();

	// variables
	D3D9UIEngine *m_engine;
	D3D9UIObject **m_childs;
	D3D9UIObject *m_parent;
	int m_childs_count;
	int m_id;
};