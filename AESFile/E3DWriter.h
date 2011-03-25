#pragma once
#include <windows.h>
#include "rijndael.h"

int encode_file(wchar_t *in, wchar_t *out);

class my12doom_leaf
{
public:
	__int64 m_leaf8cc;
	__int64 m_child_size;

	enum
	{
		leaf_is_not_set,
		leaf_is_memory,
		leaf_is_file_pos,
		leaf_is_childs,
	} m_leaf_type;

	struct  
	{
		void * data;
		__int64 size;
	} m_memory_data;

	struct
	{
		HANDLE file;
		LONGLONG pos;
		LONGLONG size;
	} m_file_data;

	int m_leaves_count;
	my12doom_leaf **m_leaves;
	my12doom_leaf();
	~my12doom_leaf();
	void AddLeaf(my12doom_leaf* leaf);
	void SetData(HANDLE file, LONGLONG pos, LONGLONG size);
	void SetData(void *data, LONGLONG size);
	void WriteTo(HANDLE file);
	void WriteTo(FILE *file);
};