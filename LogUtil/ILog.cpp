// ClassFactory.cpp : ���� DLL Ӧ�ó���ĵ���������
//

#include "stdafx.h"
#include "ILog.h"
#include "CLogImp.h"
#include "Common.h"

// ʹ�õ���ģʽ
CLog * g_instance = NULL;
// ��
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
