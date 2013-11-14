#pragma once
#include <map>

class IDWindowReader
{
public:
	virtual ~IDWindowReader() {}			// automatically close()
	virtual __int64 size() = 0;
	virtual __int64 get(void *buf, __int64 offset, int size) = 0;
	virtual void close() = 0;				// cancel all pending get() operation and reject all further get()
	virtual void release(){delete this;}
	virtual std::map<__int64, __int64> buffer() = 0;
};