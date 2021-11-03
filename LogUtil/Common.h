#pragma once

#include <Windows.h>

class CLock
{
public:
	CLock();
	~CLock();

	void Lock();
	void Unlock();

private:
	CRITICAL_SECTION m_cs;
};

typedef CLock THREAD_SAFE_LOCK;