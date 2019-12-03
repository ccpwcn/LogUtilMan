#pragma once

#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>

#ifndef LOGUTIL_EXPORTS
# define DLL_API __declspec (dllimport)
#else
# define DLL_API __declspec (dllexport)
#endif

/*********************************************************************************************
 * 日志类
 * 已经明确的使用规则：
 *    1、按照printf函数的语法提供输入
 *    2、日志处理速度和要写入的数量量有关，数量量特别大时，会有轻微的延迟
 *    3、info、error、debug、warning四个级别的日志输出函数，会返回已经处理完毕的此条日志消息的长度，处理失败返回0
 * 异步写入，支持高并发，测试数据如下：
 * +--------------+------------+------------------+--------------------+
 * |  并发线程数  |  总日志量  |  入队消耗总时长  |   保存消耗总时长   |
 * |--------------+------------+------------------+--------------------+
 * |      50      |    15W     |      0.312s      |      1.529s        |
 * +--------------+------------+------------------+--------------------+
 * |     500      |    50W     |      1.029s      |      5.086s        |
 * +--------------+------------+------------------+--------------------+
 * |    1000      |   100W     |      2.012s      |     10.124s        |
 * +--------------+------------+------------------+--------------------+
 * 注意：
 *     GetClassObject和GetClassObjectPrintQueueSize只需要调用一个即可，先调用的生效，也没有必要重复调用。
 *********************************************************************************************/
class DLL_API ILog
{
public:
	virtual ~ILog() = 0;
	virtual size_t info(__in_opt const TCHAR *fmt, ...) = 0;
	virtual size_t error(__in_opt const TCHAR *fmt, ...) = 0;
	virtual size_t debug(__in_opt const TCHAR *fmt, ...) = 0;
	virtual size_t warning(__in_opt const TCHAR *fmt, ...) = 0;
	// bPrintQueueSize为TRUE时（默认FALSE）会在日志中不定时打印日志队列的大小，在某些情况下有利于监控日志性能
	virtual void print_queue_size(__in const BOOL bPrintQueueSize) = 0;
};

extern "C" {
	// 获取日志对象
	DLL_API ILog * GetClassObject(__in const LPCTSTR lpszLogFilename);
	// 将日志队列中的所有数据写入日志文件，然后释放日志对象，回收资源
	DLL_API void ReleaseClassObject(__in const ILog * instance);
}