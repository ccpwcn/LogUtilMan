#include "stdafx.h"
#include "CLogImp.h"
#include <stdlib.h>
#include <assert.h>
#include <queue> // std::queue
#include <exception> // std::exception
#include "Common.h"

// 本文件内全局变量初始化，之所以要添加为全局锁而不是放在类中，是为了防止调用DLL的客户端初始化多个日志实例而导致错误
CLock g_Lock;
BOOL g_quitNow = FALSE;
std::queue<LPTSTR> g_myLogQueue;


CLog::CLog(const LPCTSTR lpszLogFilename, const BOOL bPrintQueueSize)
{
	// 初始化的时候，先预置值，防止意外重复初始化
	ZeroMemory(m_szFilename, BUFSIZ);
	m_fpLog = NULL;
	m_bPrintQueueSize = FALSE;
	m_hWriteThread = NULL;
	m_hWriteThreadEvent = NULL;

	g_quitNow = FALSE;

	assert(lpszLogFilename != NULL && _T("日志文件名称无效"));
	assert(lpszLogFilename[0] != _T('\r') && lpszLogFilename[0] != _T('\n') && _T("日志文件名称无效"));
	StringCchCopy(m_szFilename, BUFSIZ, lpszLogFilename);

	_tfopen_s(&m_fpLog, m_szFilename, _T("a"));
	assert(m_fpLog != NULL && _T("无法打开日志文件"));

	if (bPrintQueueSize != NULL) {
		m_bPrintQueueSize = bPrintQueueSize;
	}
	
	DWORD dwThread = 0;
	m_hWriteThread = CreateThread(NULL, 0, m_fnWriteThread, this, CREATE_SUSPENDED, &dwThread);
	assert(m_hWriteThread != NULL && _T("创建日志线程失败"));
	// 降低日志写入线程的优先级
	SetThreadPriority(m_hWriteThread, THREAD_PRIORITY_LOWEST);
	ResumeThread(m_hWriteThread);

	// 自动复原，初始状态为无信号状态，无信号就代表无日志
	m_hWriteThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(m_hWriteThreadEvent != NULL && _T("线程通信机制异常"));
}

CLog::~CLog()
{
	g_quitNow = TRUE;
	// SetEvent(m_hWriteThreadEvent);
	// 清理回收资源
	if (m_hWriteThread != NULL) {
		WaitForSingleObject(m_hWriteThread, INFINITE);
		CloseHandle(m_hWriteThread);
		m_hWriteThread = NULL;
	}

	if (m_fpLog != NULL) {
		fclose(m_fpLog);
		m_fpLog = NULL;
	}

	if (m_hWriteThreadEvent != NULL)
	{
		CloseHandle(m_hWriteThreadEvent);
		m_hWriteThreadEvent = NULL;
	}
	
	// g_Lock.~CLock();
}

size_t CLog::info(__in_opt const TCHAR *fmt, ...)
{
	if (fmt == NULL || fmt[0] == _T('\0')) {
		return -1;
	}
	
	try {
		va_list args;
		va_start(args, fmt);

		SYSTEMTIME st = { 0 };
		GetLocalTime(&st);
		size_t result = parse(&st, _T("INFO"), fmt, args);

		va_end(args);
		return result;
	}
	catch (std::exception & e)
	{
		char szBuf[64] = { 0 };
		StringCchPrintfA(szBuf, 64, "write info log exception: %s\n", e.what());
		printf(szBuf);
		OutputDebugStringA(szBuf);
		return -1;
	}
}

size_t CLog::error(__in_opt const TCHAR *fmt, ...)
{
	if (fmt == NULL || fmt[0] == _T('\0')) {
		return -1;
	}
	
	try {
		va_list args;
		va_start(args, fmt);

		SYSTEMTIME st = { 0 };
		GetLocalTime(&st);
		size_t result = parse(&st, _T("ERROR"), fmt, args);

		va_end(args);

		return result;
	}
	catch (std::exception & e)
	{
		char szBuf[64] = { 0 };
		StringCchPrintfA(szBuf, 64, "write error log exception: %s\n", e.what());
		OutputDebugStringA(szBuf);
		return -1;
	}
}

size_t CLog::debug(__in_opt const TCHAR *fmt, ...)
{
	if (fmt == NULL || fmt[0] == _T('\0')) {
		return -1;
	}
	
	try {
		va_list args;
		va_start(args, fmt);

		SYSTEMTIME st = { 0 };
		GetLocalTime(&st);
		size_t result = parse(&st, _T("DEBUG"), fmt, args);

		va_end(args);

		return result;
	}
	catch (std::exception & e)
	{
		char szBuf[64] = { 0 };
		StringCchPrintfA(szBuf, 64, "write debug log exception: %s\n", e.what());
		printf(szBuf);
		OutputDebugStringA(szBuf);
		return -1;
	}
}

size_t CLog::warning(__in_opt const TCHAR *fmt, ...)
{
	if (fmt == NULL || fmt[0] == _T('\0')) {
		return -1;
	}
	
	try {
		va_list args;
		va_start(args, fmt);

		SYSTEMTIME st = { 0 };
		GetLocalTime(&st);
		size_t result = parse(&st, _T("WARNING"), fmt, args);

		va_end(args);

		return result;
	}
	catch (std::exception& e)
	{
		char szBuf[64] = { 0 };
		StringCchPrintfA(szBuf, 64, "write warning log exception: %s\n", e.what());
		OutputDebugStringA(szBuf);
		return -1;
	}
}

void CLog::print_queue_size(__in const BOOL bPrintQueueSize)
{
	m_bPrintQueueSize = bPrintQueueSize;
}

DWORD CLog::m_fnWriteThread(LPVOID lpParam)
{
	CLog * pLogInstance = (CLog *)lpParam;
	assert(pLogInstance != NULL && _T("日志文件对象无效"));

	int nCoefficient = pLogInstance->m_fnGetSystemPreformanceCoefficient();
	int nOriginCoefficient = nCoefficient;
	while (TRUE)
	{
		if (g_myLogQueue.empty())
		{
			if (g_quitNow)
			{
				break;
			}
			Sleep(nCoefficient);
			continue;
		}

		size_t count = g_myLogQueue.size();
		if (pLogInstance->m_bPrintQueueSize)
		{
			_ftprintf_s(pLogInstance->m_fpLog, _T("queue size:%zu\n"), count);
		}

		LPTSTR lpszMsg = NULL;
		for (size_t i = 0; i < count; i++)
		{
			g_Lock.Lock();
			lpszMsg = g_myLogQueue.front();
			g_myLogQueue.pop();
			g_Lock.Unlock();

			if (lpszMsg != NULL) {
				size_t len = _tcslen(lpszMsg);
				_ftprintf_s(pLogInstance->m_fpLog, lpszMsg);
				if (lpszMsg[len - 1] != _T('\n'))
					_ftprintf_s(pLogInstance->m_fpLog, _T("\n"));
				// 及时回收内存
				delete[] lpszMsg;
			}
		}
	}

	return NO_ERROR;
}


size_t CLog::parse(LPSYSTEMTIME lpSystemTime, __in LPCTSTR lpszLogTypeFlag, __in_opt const TCHAR *fmt, va_list args)
{
	if (fmt == NULL || fmt[0] == _T('\0')) {
		return -1;
	}
	size_t result = 0;

	const int TIME_BUF_LEN = 64;
	const int BUF_SIZE = 4096;
	TCHAR szBuffer[BUF_SIZE] = { 0 };
	TCHAR szTimeFlag[TIME_BUF_LEN] = { 0 };
	StringCchPrintf(szTimeFlag, TIME_BUF_LEN, _T("%04d-%02d-%02d %02d:%02d:%02d[%s] "),
		lpSystemTime->wYear, lpSystemTime->wMonth, lpSystemTime->wDay, lpSystemTime->wHour, lpSystemTime->wMinute, lpSystemTime->wSecond, lpszLogTypeFlag);

	StringCchCopy(szBuffer, BUF_SIZE, szTimeFlag);
	HRESULT hr = StringCchVPrintf(szBuffer + _tcslen(szBuffer), BUF_SIZE - _tcslen(szBuffer) - 1, fmt, args);
	if (SUCCEEDED(hr))
	{
		// 内存申请成功时
		result = _tcslen(szBuffer);
		LPTSTR lpszBuffer = new TCHAR[result+1];
		memset(lpszBuffer, 0, result + 1);
		StringCchCopy(lpszBuffer, result + 1, szBuffer);
		g_Lock.Lock();
		g_myLogQueue.push(lpszBuffer);
		g_Lock.Unlock();
	}

	return result;
}

int CLog::m_fnGetSystemPreformanceCoefficient()
{
	int nCoefficient = 2;

	// 默认电脑有4核CPU、8GB内存
	int nProcessorCount = 4;
	__int64 nMemorySize = 8 * 1024 * 1024 * 1024LL;

	typedef struct _SYSTEM_BASIC_INFORMATION
	{
		ULONG Unknown; //Always contains zero
		ULONG MaximumIncrement; //一个时钟的计量单位
		ULONG PhysicalPageSize; //一个内存页的大小
		ULONG NumberOfPhysicalPages; //系统管理着多少个页
		ULONG LowestPhysicalPage; //低端内存页
		ULONG HighestPhysicalPage; //高端内存页
		ULONG AllocationGranularity;
		ULONG LowestUserAddress; //地端用户地址
		ULONG HighestUserAddress; //高端用户地址
		ULONG ActiveProcessors; //激活的处理器
		UCHAR NumberProcessors; //有多少个处理器
	}SYSTEM_BASIC_INFORMATION, *PSYSTEM_BASIC_INFORMATION;
	SYSTEM_BASIC_INFORMATION SysBaseInfo = { 0 };
	HMODULE ntdllMod = GetModuleHandle(_T("ntdll.dll"));
	if (ntdllMod != NULL)
	{
		typedef LONG(WINAPI* FN_NtQuerySystemInformation)(UINT, PVOID, ULONG, PULONG);
		FN_NtQuerySystemInformation pfnNtQuerySystemInformation = (FN_NtQuerySystemInformation)GetProcAddress(ntdllMod, "NtQuerySystemInformation");
		TCHAR szDebugBuff[MAX_PATH] = { 0 };
		StringCchPrintf(szDebugBuff, MAX_PATH - 1, _T("FN_NtQuerySystemInformation address %p"), pfnNtQuerySystemInformation);
		OutputDebugString(szDebugBuff);
		if (pfnNtQuerySystemInformation != NULL)
		{
#define SystemBasicInformation 0
			LONG status = pfnNtQuerySystemInformation(SystemBasicInformation, &SysBaseInfo, sizeof(SysBaseInfo), NULL);
			if (status == NO_ERROR)
			{
				nProcessorCount = SysBaseInfo.NumberProcessors;
				nMemorySize = SysBaseInfo.PhysicalPageSize * SysBaseInfo.NumberOfPhysicalPages;
			}
		}

		nCoefficient = (nProcessorCount + 1) * int((double)nMemorySize / 1024 / 1024 / 1024 * 0.6) * 20;
		StringCchPrintf(szDebugBuff, MAX_PATH - 1, _T("FN_NtQuerySystemInformation nCoefficient %d"), nCoefficient);
		OutputDebugString(szDebugBuff);
	}
	return nCoefficient;
}
