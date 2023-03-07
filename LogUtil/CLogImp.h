#pragma once

#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>
#include "ILog.h"

// 最大日志大小
#define MAX_LOG_MSG_LEN (1 * 1024 * 1024)

class CLog : public ILog
{
public:
	CLog(__in const LPCTSTR lpszLogFilename, __in const BOOL bPrintQueueSize = TRUE);
	~CLog();
	size_t info(__in_opt const TCHAR *fmt, ...);
	size_t error(__in_opt const TCHAR *fmt, ...);
	size_t debug(__in_opt const TCHAR *fmt, ...);
	size_t warning(__in_opt const TCHAR *fmt, ...);
	void print_queue_size(__in const BOOL bPrintQueueSize);
private:
	TCHAR m_szFilename[BUFSIZ];
	FILE * m_fpLog;
	BOOL m_bPrintQueueSize;
	HANDLE m_hWriteThread;
	HANDLE m_hWriteThreadEvent;
	static DWORD WINAPI m_fnWriteThread(LPVOID lpParam);
	size_t parse(LPSYSTEMTIME lpSystemTime, __in LPCTSTR lpszLogTypeFlag, __in_opt const TCHAR *fmt, va_list args);
	int m_fnGetSystemPreformanceCoefficient();
};