#include "nal.h"

const unsigned int max_nal_size = 1024000;
unsigned char *delimeter_buffer = (unsigned char*)malloc(max_nal_size);

int nal_type(unsigned char *data)
{
	if (data[2] == 0x0 && data[3] == 0x1)
		return data[4] & 0x1f;
	else if (data[1] == 0x0 && data[2] == 0x1)
		return data[3] & 0x1f;

	return -1;
}

int read_a_nal(CFileBuffer *f)
{
	unsigned data_read = f->wait_for_data(max_nal_size);
	unsigned char* read_buffer = f->m_the_buffer + f->m_data_start;

	if (data_read == 0)
		return 0;
	for(unsigned int i=4; i<data_read-3; i++)
		//if (read_buffer[i] == 0)
		//if (read_buffer[i] == 0x0 && read_buffer[i+1] == 0x0 && read_buffer[i+2] == 0x0 && read_buffer[i+3] == 0x1)
		if (*(unsigned int *)(read_buffer+i) == 0x01000000)
		{
			return i;
		}
	return data_read;
}

int read_a_delimeter(CFileBuffer *f)
{
	int size = 0;
	int current_start = false;

	while(true)
	{
		int nal_size = read_a_nal(f);
		unsigned char *read_buffer = f->m_the_buffer + f->m_data_start;
		int _nal_type = nal_type(read_buffer);

		/*
		if (f == &left)
			printf("(l)");
		else
			printf("(r)");
		printf("%x ", _nal_type);
		*/

		if (_nal_type == 31) //my12doom's watermark
		{
			f->remove_data(nal_size);
			continue;
		}

		if (nal_size == 0 ||	
			nal_type(read_buffer) == 0x9 ||
			nal_type(read_buffer) == 0x18)
		{
			if (current_start || nal_size == 0)
			{
				return size;
			}
			else
			{
				current_start = true;
				memcpy(delimeter_buffer + size, read_buffer, nal_size);
				size += nal_size;

				f->remove_data(nal_size);
			}
		}
		else
		{
			memcpy(delimeter_buffer + size, read_buffer, nal_size);
			size += nal_size;

			f->remove_data(nal_size);
		}
	}
}
