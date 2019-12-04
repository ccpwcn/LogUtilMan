// ClassFactory.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "ILog.h"
#include "CLogImp.h"

// 使用单例模式
CLog * g_instance = NULL;

ILog * GetClassObject(__in const LPCTSTR lpszLogFilename)
{
	if (g_instance == NULL)
	{
		g_instance = new CLog(lpszLogFilename);
	}

	return g_instance;
}

void ReleaseClassObject(__in const ILog * instance)
{
	if (g_instance != NULL)
	{
		delete g_instance;
		g_instance = NULL;
	}
}

ILog::~ILog()
{}
