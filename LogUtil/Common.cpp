#include "stdafx.h"
#include <Windows.h>
#include "Common.h"

CLock::CLock()
{
	InitializeCriticalSection(&m_cs);
}
CLock::~CLock()
{
	DeleteCriticalSection(&m_cs);
}

void CLock::Lock()
{
	EnterCriticalSection(&m_cs);
}
void CLock::Unlock()
{
	LeaveCriticalSection(&m_cs);
}