#include "BaseUI.h"

const int MAX_CHILD = 32;

// D3D9UIEngine
D3D9UIEngine::D3D9UIEngine(HWND hwnd)
{
	m_unique_id = 0;
}

D3D9UIEngine::~D3D9UIEngine()
{
}

HRESULT D3D9UIEngine::RegisterObject(int *out, ID3D9UIObject *obj)
{
	if(!out)
		return E_POINTER;

	*out = m_unique_id ++;
	return S_OK;
}

HRESULT D3D9UIEngine::GetUIRoot(ID3D9UIObject **root)
{
	if (!root)
		return E_POINTER;
	*root = m_root;
	return S_OK;
}


//D3D9UIObject
HRESULT D3D9UIObject::GetParent(ID3D9UIObject **parent)
{
	if (!parent)
		return E_POINTER;
	*parent = m_parent;
	return S_OK;
}
HRESULT D3D9UIObject::AddChild(ID3D9UIObject *child)
{
	if (m_childs_count >= MAX_CHILD)
		return E_UNEXPECTED;

	if (!child)
		return E_POINTER;

	m_childs[m_childs_count++] = (D3D9UIObject*)child;
	((D3D9UIObject*)child)->m_parent = this;
	return S_OK;
}
HRESULT D3D9UIObject::RemoveChild(int pos)
{
	if (pos < 0 || pos >= m_childs_count)
		return E_INVALIDARG;

	m_childs[pos]->m_parent = NULL;
	memmove(m_childs+pos, m_childs+pos+1, (m_childs_count-pos-1) * sizeof(void*));
	m_childs_count--;
	return S_OK;

}
HRESULT D3D9UIObject::GetChild(int pos, ID3D9UIObject **child)
{
	if (pos < 0 || pos >= m_childs_count)
		return E_INVALIDARG;

	if (!child)
		return E_POINTER;

	*child = m_childs[pos];
	return S_OK;
}
HRESULT D3D9UIObject::GetChildCount(int *count)
{
	if (!count)
		return E_POINTER;
	*count = m_childs_count;
	return S_OK;
}
HRESULT D3D9UIObject::GetID(int *id)
{
	if (!id)
		return E_POINTER;
	*id = m_id;
	return S_OK;
}
HRESULT D3D9UIObject::GetHittestObject(int id, ID3D9UIObject **hit)
{
	if (!hit)
		return E_POINTER;

	if (id == m_id)
	{
		*hit = this;
		return S_OK;
	}

	for(int i=0; i<m_childs_count; i++)
		if (m_childs[i]->GetHittestObject(id, hit) == S_OK)
			return S_OK;

	return E_FAIL;
}
HRESULT D3D9UIObject::DefaultFocusHandling()
{
	return E_NOTIMPL;
}
D3D9UIObject::D3D9UIObject(ID3D9UIEngine *engine)
{
	m_engine = (D3D9UIEngine*)engine;
	engine->RegisterObject(&m_id, this);
	m_parent = NULL;
	m_childs_count = 0;
	m_childs = (D3D9UIObject**)malloc(sizeof(D3D9UIObject*)*MAX_CHILD);
}

D3D9UIObject::~D3D9UIObject()
{
	if (m_parent)
	{
		int c;
		m_parent->GetChildCount(&c);

		for(int i=0; i<c; i++)
		{
			D3D9UIObject *object;
			m_parent->GetChild(i, (ID3D9UIObject**)&object);
			if (object->m_id == m_id)
			{
				m_parent->RemoveChild(i);
				break;
			}
		}
	}

	while (m_childs_count)
		RemoveChild(0);
	free(m_childs);
}

HRESULT D3D9UIObject::Render(bool HitTest)
{
	HRESULT hr;
	if (FAILED(hr = RenderThis(HitTest)))
		return hr;

	for(int i=0; i<m_childs_count; i++)
		if (FAILED(hr = m_childs[i]->Render(HitTest)))
			return hr;

	return S_OK;
}