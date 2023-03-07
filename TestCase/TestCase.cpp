// TestCase.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <time.h>

#define REFERENCE_DLL
#include "..\LogUtil\ILog.h"

unsigned int nThreadCount = 100;
unsigned int nRunTimes = 200;
BOOL bPrintQueueSize = TRUE;

DWORD test();

int _tmain(int argc, TCHAR *argv[])
{
	if (argc != 4)
	{
		_tprintf(_T("TestCase.exe nThreadCount nWriteCount bPrintQueueSize\n"));
		return 0;
	}
	nThreadCount = _tstoi(argv[1]);
	nRunTimes = _tstoi(argv[2]);
	bPrintQueueSize = _tcsicmp(argv[3], _T("0")) != 0;

	// 开始执行测试
	_tprintf(_T("The tester is running, please wait...\n"));
	DWORD dwRet = test();
	_tprintf(_T("The tester done, return %d\n"), dwRet);

#ifdef _DEBUG
	_tprintf(_T("Now, you can check the memory leak information, then press ENTER key for exit...\n"));
	getchar();
#endif
    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct {
	DWORD dwThreadId;
	DWORD dwPushTime;
	DWORD dwWriteTime;
	ILog * op;
} THREAD_DATA;
DWORD WINAPI MyThreadFunction(LPVOID lpParam);

int seh_filer(int code);


DWORD WINAPI MyThreadFunction(LPVOID lpParam)
{
	THREAD_DATA * ptd = (THREAD_DATA *)lpParam;
	if (ptd == NULL)
	{
		_tprintf(_T("Thread failed.\n"));
		return ERROR_INVALID_PARAMETER;
	}

	DWORD dwStart = GetTickCount();
	size_t r = 0;
	for (size_t i = 0; i < nRunTimes; i++)
	{
		r = ptd->op->info(_T("Thread %ld example random log message(%zu) for dll model testcase"), ptd->dwThreadId, i);
		if (!r)
		{
			printf("Thread %ld of message %zu error\n", ptd->dwThreadId, i);
		}
	}
	// printf("Test Thread %d DONE!!!\n", ptd->dwThreadId);
	ptd->dwPushTime = GetTickCount() - dwStart;

	return NO_ERROR;
}

DWORD test()
{
	DWORD dwRet = NO_ERROR;

	THREAD_DATA * ptd = NULL;
	DWORD   * pdwThreadIdArray = NULL;
	HANDLE  * phThreadArray = NULL;

	__try
	{
		__try
		{
			srand((unsigned int)time(0));
			DWORD dwRet = 0;
			HMODULE hDllLib = LoadLibrary(_T("LogUtil.dll"));
			if (hDllLib == NULL)
			{
				_tprintf(_T("Load dll failed, error code:%u\n"), dwRet = GetLastError());
				return dwRet;
			}
			typedef ILog* (FN_GetClassObject)(__in const LPCTSTR lpszLogFilename);
			typedef void (FN_ReleaseClassObject)(__in const ILog* instance);
			FN_GetClassObject * fpGetClassObject = (FN_GetClassObject *)GetProcAddress(hDllLib, "GetClassObject");
			FN_ReleaseClassObject * fpReleaseClassObject = (FN_ReleaseClassObject *)GetProcAddress(hDllLib, "ReleaseClassObject");

			if (fpGetClassObject == NULL || fpReleaseClassObject == NULL)
			{
				_tprintf(_T("Get function from dll failed, error code:%u\n"), dwRet = GetLastError());
				return dwRet;
			}

			ptd = new THREAD_DATA[nThreadCount];
			memset(ptd, 0, sizeof(THREAD_DATA) * nThreadCount);
			pdwThreadIdArray = new DWORD[nThreadCount];
			memset(pdwThreadIdArray, 0, sizeof(DWORD) * nThreadCount);
			phThreadArray = new HANDLE[nThreadCount];
			memset(phThreadArray, 0, sizeof(DWORD) * nThreadCount);
			// 所有线程全部就绪
			DWORD dwStart = GetTickCount();
			for (unsigned int i = 0; i < nThreadCount; i++)
			{
				ptd[i].dwThreadId = i;
				ptd[i].op = fpGetClassObject(_T("text.log"));
				phThreadArray[i] = CreateThread(
					NULL,                   // default security attributes
					0,                      // use default stack size  
					MyThreadFunction,       // thread function name
					ptd + i,          // argument to thread function 
					CREATE_SUSPENDED,                      // do not run 
					&pdwThreadIdArray[i]);   // returns the thread identifier
				if (phThreadArray[i] == NULL)
				{
					_tprintf(_T("CreateThread %d failed, error code:%u\n"), i, dwRet = GetLastError());
					continue;
				}
			}
			// ptd[0].op->print_queue_size(TRUE);
			for (unsigned int i = 0; i < nThreadCount; i++)
			{
				if (phThreadArray[i] != NULL)
				{
					ResumeThread(phThreadArray[i]);
				}
			}

			// 等待压力线程全部返回
			WaitForMultipleObjects(nThreadCount, phThreadArray, TRUE, INFINITE);
			
			// 关闭句柄
			for (unsigned int i = 0; i < nThreadCount; i++)
			{
				if (phThreadArray[i] != NULL)
				{
					CloseHandle(phThreadArray[i]);
				}
			}
			// 等待日志处理任务全部完成
			DWORD time = 0;
			for (unsigned int i = 0; i < nThreadCount; i++)
			{
				time < ptd[i].dwPushTime ? time = ptd[i].dwPushTime : 0;
				fpReleaseClassObject(ptd[i].op);
			}
			// time = time / MAX_THREADS;


			FreeLibrary(hDllLib);
			DWORD dwEnd = GetTickCount();

			// 计算时间，分析性能
			DWORD h = time / 1000 / 3600;
			DWORD m = time / 1000 / 60;
			DWORD s = time / 1000 % 60;
			DWORD ms = time % 1000;
			_tprintf(_T("Push %d log message left time:%02d:%02d:%02d.%03d\n"), nThreadCount * nRunTimes, h, m, s, ms);

			DWORD time1 = dwEnd - dwStart;
			DWORD h1 = time1 / 1000 / 3600;
			DWORD m1 = time1 / 1000 / 60;
			DWORD s1 = time1 / 1000 % 60;
			DWORD ms1 = time1 % 1000;
			_tprintf(_T("Save %d log meesage left time:%02d:%02d:%02d.%03d\n"), nThreadCount * nRunTimes, h1, m1, s1, ms1);
		}
		__except (seh_filer(GetExceptionCode()))
		{
			_tprintf(_T("exception...\n"));
		}
	}
	__finally
	{
		if (ptd != NULL)
			delete[] ptd;
		if (pdwThreadIdArray != NULL)
			delete[] pdwThreadIdArray;
		if (phThreadArray != NULL)
			delete[] phThreadArray;

		_tprintf(_T("DONE\n"));
	}

	return dwRet;
}


int seh_filer(int code)
{
	switch (code)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		printf("存储保护异常，错误代码：%x\n", code);
		break;
	case EXCEPTION_DATATYPE_MISALIGNMENT:
		printf("数据类型未对齐异常，错误代码：%x\n", code);
		break;
	case EXCEPTION_BREAKPOINT:
		printf("中断异常，错误代码：%x\n", code);
		break;
	case EXCEPTION_SINGLE_STEP:
		printf("单步中断异常，错误代码：%x\n", code);
		break;
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		printf("数组越界异常，错误代码：%x\n", code);
		break;
	case EXCEPTION_FLT_DENORMAL_OPERAND:
		printf("非正规操作异常，错误代码：%x\n", code);
		break;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		printf("被浮点数0除异常，错误代码：%x\n", code);
		break;
	case EXCEPTION_FLT_INEXACT_RESULT:
		printf("不准确浮点数结果异常，错误代码：%x\n", code);
		break;
	case EXCEPTION_FLT_INVALID_OPERATION:
		printf("无效的浮点数操作异常，错误代码：%x\n", code);
		break;
	case EXCEPTION_FLT_OVERFLOW:
		printf("浮点数溢出异常，错误代码：%x\n", code);
		break;
	case EXCEPTION_FLT_STACK_CHECK:
		printf("浮点数栈异常，错误代码：%x\n", code);
		break;
	case EXCEPTION_FLT_UNDERFLOW:
		printf("浮点数计算异常（下溢出），错误代码：%x\n", code);
		break;
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		printf("被整数0除异常，错误代码：%x\n", code);
		break;
	case EXCEPTION_INT_OVERFLOW:
		printf("整数数据溢出异常，错误代码：%x\n", code);
		break;
	case EXCEPTION_IN_PAGE_ERROR:
		printf("页错误异常，错误代码：%x\n", code);
		break;
	case EXCEPTION_ILLEGAL_INSTRUCTION:
		printf("非法指令异常，错误代码：%x\n", code);
		break;
	case EXCEPTION_STACK_OVERFLOW:
		printf("堆栈溢出异常，错误代码：%x\n", code);
		break;
	case EXCEPTION_INVALID_HANDLE:
		printf("无效句柄异常，错误代码：%x\n", code);
		break;
	case CONTROL_C_EXIT:
		printf("CTRL + C要求退出\n");
		break;
	default:
		if (code & (1 << 29))
			printf("用户自定义的软件异常，错误代码：%x\n", code);
		else
			printf("其它异常，错误代码：%x\n", code);
		break;
	}

	return 1;
}