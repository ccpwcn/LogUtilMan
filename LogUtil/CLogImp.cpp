#include "stdafx.h"
#include "CLogImp.h"
#include "CLogImp.h"
#include <stdlib.h>
#include <assert.h>
#include <queue> // std::queue
#include "Common.hpp"

// 本文件内全局变量初始化
BOOL g_bWaitForQuit = FALSE;
CLock g_Lock;
std::queue<LPTSTR> g_myLogQueue;

CLog::CLog(const LPCTSTR lpszLogFilename)
{
	assert(lpszLogFilename != NULL && lpszLogFilename[0] != _T('\n'));
	StringCchCopy(m_szFilename, MAX_PATH, lpszLogFilename);
	
	_tfopen_s(&m_fpLog, m_szFilename, _T("a"));
	assert(m_fpLog != NULL);

	DWORD dwThread = 0;
	m_hWriteThread = CreateThread(NULL, 0, m_fnWriteThread, this, CREATE_SUSPENDED, &dwThread);
	assert(m_hWriteThread != NULL);
	SetThreadPriority(m_hWriteThread, THREAD_PRIORITY_LOWEST);
	ResumeThread(m_hWriteThread);
	
	// 自动复原，初始状态为无信号状态
	//m_hWriteThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	//assert(m_hWriteThreadEvent != NULL);
}

CLog::~CLog()
{
	g_bWaitForQuit = TRUE;
	WaitForSingleObject(m_hWriteThread, INFINITE);
	CloseHandle(m_hWriteThread);
	m_hWriteThread = NULL;

	fclose(m_fpLog);
	m_fpLog = NULL;

	CloseHandle(m_hWriteThreadEvent);
	m_hWriteThreadEvent = NULL;
	
	g_Lock.~CLock();
}

size_t CLog::info(__in_opt const TCHAR *fmt, ...)
{
	assert(fmt != NULL && fmt[0] != _T('\0'));
	
	va_list args;
	va_start(args, fmt);

	SYSTEMTIME st = { 0 };
	GetLocalTime(&st);
	size_t result = parse(&st, _T("INFO"), fmt, args);

	va_end(args);

	return result;
}

size_t CLog::error(__in_opt const TCHAR *fmt, ...)
{
	assert(fmt != NULL && fmt[0] != _T('\0'));
	
	va_list args;
	va_start(args, fmt);

	SYSTEMTIME st = { 0 };
	GetLocalTime(&st);
	size_t result = parse(&st, _T("ERROR"), fmt, args);

	va_end(args);

	return result;
}

size_t CLog::debug(__in_opt const TCHAR *fmt, ...)
{
	assert(fmt != NULL && fmt[0] != _T('\0'));
	
	va_list args;
	va_start(args, fmt);

	SYSTEMTIME st = { 0 };
	GetLocalTime(&st);
	size_t result = parse(&st, _T("DEBUG"), fmt, args);

	va_end(args);

	return result;
}

size_t CLog::warning(__in_opt const TCHAR *fmt, ...)
{
	assert(fmt != NULL && fmt[0] != _T('\0'));
	
	va_list args;
	va_start(args, fmt);

	SYSTEMTIME st = { 0 };
	GetLocalTime(&st);
	size_t result = parse(&st, _T("WARNING"), fmt, args);

	va_end(args);

	return result;
}

DWORD CLog::m_fnWriteThread(LPVOID lpParam)
{
	CLog * pLogInstance = (CLog *)lpParam;
	assert(pLogInstance != NULL);

	int nCoefficient = pLogInstance->m_fnGetSystemPreformanceCoefficient();

	while (!g_bWaitForQuit || !g_myLogQueue.empty())
	{
		// WaitForSingleObject(m_hWriteThreadEvent, INFINITE);
		if (g_myLogQueue.empty())
		{
			Sleep(nCoefficient);
			continue;
		}

		size_t count = g_myLogQueue.size();
		_ftprintf_s(pLogInstance->m_fpLog, _T("queue size:%u\n"), count);

		LPTSTR lpszMsg = NULL;
		for (size_t i = 0; i < count; i++)
		{
			g_Lock.Lock();
			lpszMsg = g_myLogQueue.front();
			g_myLogQueue.pop();
			g_Lock.Unlock();

			int len = _tcslen(lpszMsg);
			_ftprintf_s(pLogInstance->m_fpLog, lpszMsg);
			if (lpszMsg[len - 1] != _T('\n'))
				_ftprintf_s(pLogInstance->m_fpLog, _T("\n"));
			delete[] lpszMsg;
		}
	}

	return 0;
}

size_t CLog::parse(LPSYSTEMTIME lpSystemTime, __in LPCTSTR lpszLogTypeFlag, __in_opt const TCHAR *fmt, va_list args)
{
	size_t result = 0;

	const int MIN_BUF_LEN = 64;
	TCHAR szTimeFlag[MIN_BUF_LEN] = { 0 };
	StringCchPrintf(szTimeFlag, MIN_BUF_LEN, _T("%04d-%02d-%02d %02d:%02d:%02d[%s] "),
		lpSystemTime->wYear, lpSystemTime->wMonth, lpSystemTime->wDay, lpSystemTime->wHour, lpSystemTime->wMinute, lpSystemTime->wSecond, lpszLogTypeFlag);

	// 下面申请的内存，会在写入线程完成之后释放
	// StringCch***函数族都是按指定大小操作目标缓冲区，并且会在操作完成之后在目标字符串尾部添加\0，所以申请的缓冲区不必使用memset清零
	int len = _tcslen(fmt);
	int nBufSize = len * 2 + MIN_BUF_LEN;
	LPTSTR lpszBuffer = new TCHAR[nBufSize];
	StringCchCopy(lpszBuffer, nBufSize, szTimeFlag);
	HRESULT hr = StringCchVPrintf(lpszBuffer + _tcslen(lpszBuffer), nBufSize - _tcslen(lpszBuffer), fmt, args);
	if (hr == STRSAFE_E_INSUFFICIENT_BUFFER)
	{
		delete[] lpszBuffer;
		nBufSize = len * 4 + MIN_BUF_LEN;;
		lpszBuffer = new TCHAR[nBufSize];
		StringCchCopy(lpszBuffer, nBufSize, szTimeFlag);
		hr = StringCchVPrintf(lpszBuffer + _tcslen(lpszBuffer), nBufSize - _tcslen(lpszBuffer), fmt, args);
		if (hr == STRSAFE_E_INSUFFICIENT_BUFFER)
		{
			delete[] lpszBuffer;
			nBufSize = len * 16 + MIN_BUF_LEN;
			lpszBuffer = new TCHAR[nBufSize];
			StringCchCopy(lpszBuffer, nBufSize, szTimeFlag);
			hr = StringCchVPrintf(lpszBuffer + _tcslen(lpszBuffer), nBufSize - _tcslen(lpszBuffer), fmt, args);
			if (hr == STRSAFE_E_INSUFFICIENT_BUFFER)
			{
				delete[] lpszBuffer;
				nBufSize = len * 256 + MIN_BUF_LEN;
				lpszBuffer = new TCHAR[nBufSize];
				StringCchCopy(lpszBuffer, nBufSize, szTimeFlag);
				hr = StringCchVPrintf(lpszBuffer + _tcslen(lpszBuffer), nBufSize - _tcslen(lpszBuffer), fmt, args);
			}
		}
	}
	if (SUCCEEDED(hr))
	{
		g_Lock.Lock();
		g_myLogQueue.push(lpszBuffer);
		g_Lock.Unlock();
		result = _tcslen(lpszBuffer);
	}
	else
	{
		delete[] lpszBuffer;
	}

	//SetEvent(m_hWriteThreadEvent);

	return result;
}

int CLog::m_fnGetSystemPreformanceCoefficient()
{
	int nCoefficient = 2;

	// 默认电脑有2个CPU、4GB内存
	int nProcessorCount = 2;
	__int64 nMemorySize = 4 * 1024 * 1024 * 1024LL;

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
	typedef LONG(WINAPI *FN_NtQuerySystemInformation)(UINT, PVOID, ULONG, PULONG);
	FN_NtQuerySystemInformation pfnNtQuerySystemInformation = (FN_NtQuerySystemInformation)GetProcAddress(GetModuleHandle(_T("ntdll")), "NtQuerySystemInformation");
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

	return nCoefficient;
}
