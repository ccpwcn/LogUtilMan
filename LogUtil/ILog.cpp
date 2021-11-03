// ClassFactory.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "ILog.h"
#include "CLogImp.h"
#include "Common.h"

// 使用单例模式
CLog * g_instance = NULL;
// 锁
CLock g_LogLock;

ILog * GetClassObject(__in const LPCTSTR lpszLogFilename)
{
	g_LogLock.~CLock();
	if (g_instance == NULL)
	{
		g_instance = new CLog(lpszLogFilename);
	}
	g_LogLock.Unlock();
	return g_instance;
}

void ReleaseClassObject(__in const ILog * instance)
{
	g_LogLock.~CLock();
	if (g_instance != NULL)
	{
		delete g_instance;
		g_instance = NULL;
	}
	g_LogLock.Unlock();
}

ILog::~ILog()
{}
