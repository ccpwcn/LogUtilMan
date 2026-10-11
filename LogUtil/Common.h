#pragma once

#include <Windows.h>

class CLock
{
public:
	CLock();
	~CLock();

    // Disable copy and move to prevent accidental copying of the
	// underlying CRITICAL_SECTION which would lead to double-delete
	// and undefined behavior.
	CLock(const CLock&) = delete;
	CLock& operator=(const CLock&) = delete;
	CLock(CLock&&) = delete;
	CLock& operator=(CLock&&) = delete;

	void Lock();
	void Unlock();

private:
	CRITICAL_SECTION m_cs;
};

typedef CLock THREAD_SAFE_LOCK;