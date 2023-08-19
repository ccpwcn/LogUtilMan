#include "stdafx.h"
#include "CLogImp.h"
#include <stdlib.h>
#include <assert.h>
#include <queue> // std::queue
#include <exception> // std::exception
#include "Common.h"

// ���ļ���ȫ�ֱ�����ʼ����֮����Ҫ���Ϊȫ���������Ƿ������У���Ϊ�˷�ֹ����DLL�Ŀͻ��˳�ʼ�������־ʵ�������´���
CLock g_Lock;
BOOL g_quitNow = FALSE;
std::queue<LPTSTR> g_myLogQueue;


CLog::CLog(const LPCTSTR lpszLogFilename, const BOOL bPrintQueueSize)
{
	// ��ʼ����ʱ����Ԥ��ֵ����ֹ�����ظ���ʼ��
	ZeroMemory(m_szFilename, BUFSIZ);
	m_fpLog = NULL;
	m_bPrintQueueSize = FALSE;
	m_hWriteThread = NULL;
	m_hWriteThreadEvent = NULL;

	g_quitNow = FALSE;

	assert(lpszLogFilename != NULL && _T("��־�ļ�������Ч"));
	assert(lpszLogFilename[0] != _T('\r') && lpszLogFilename[0] != _T('\n') && _T("��־�ļ�������Ч"));
	StringCchCopy(m_szFilename, BUFSIZ, lpszLogFilename);

	_tfopen_s(&m_fpLog, m_szFilename, _T("a"));
	assert(m_fpLog != NULL && _T("�޷�����־�ļ�"));

	if (bPrintQueueSize != NULL) {
		m_bPrintQueueSize = bPrintQueueSize;
	}
	
	DWORD dwThread = 0;
	m_hWriteThread = CreateThread(NULL, 0, m_fnWriteThread, this, CREATE_SUSPENDED, &dwThread);
	assert(m_hWriteThread != NULL && _T("������־�߳�ʧ��"));
	// ������־д���̵߳����ȼ�
	SetThreadPriority(m_hWriteThread, THREAD_PRIORITY_LOWEST);
	ResumeThread(m_hWriteThread);

	// �Զ���ԭ����ʼ״̬Ϊ���ź�״̬�����źžʹ�������־
	m_hWriteThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(m_hWriteThreadEvent != NULL && _T("�߳�ͨ�Ż����쳣"));
}

CLog::~CLog()
{
	g_quitNow = TRUE;
	// SetEvent(m_hWriteThreadEvent);
	// ���������Դ
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
	assert(pLogInstance != NULL && _T("��־�ļ�������Ч"));

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
				// ��ʱ�����ڴ�
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
		// �ڴ�����ɹ�ʱ
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

	// Ĭ�ϵ�����4��CPU��8GB�ڴ�
	int nProcessorCount = 4;
	__int64 nMemorySize = 8 * 1024 * 1024 * 1024LL;

	typedef struct _SYSTEM_BASIC_INFORMATION
	{
		ULONG Unknown; //Always contains zero
		ULONG MaximumIncrement; //һ��ʱ�ӵļ�����λ
		ULONG PhysicalPageSize; //һ���ڴ�ҳ�Ĵ�С
		ULONG NumberOfPhysicalPages; //ϵͳ�����Ŷ��ٸ�ҳ
		ULONG LowestPhysicalPage; //�Ͷ��ڴ�ҳ
		ULONG HighestPhysicalPage; //�߶��ڴ�ҳ
		ULONG AllocationGranularity;
		ULONG LowestUserAddress; //�ض��û���ַ
		ULONG HighestUserAddress; //�߶��û���ַ
		ULONG ActiveProcessors; //����Ĵ�����
		UCHAR NumberProcessors; //�ж��ٸ�������
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
