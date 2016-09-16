#LogUtilMan
### 日志类
* 支持多线程高并发，实测达到200W级，300W级因为我的电脑太烂了，测不了，如果谁发现BUG，请告诉我
* 支持日志分级，有information、error、debug、warning四个级别
* 支持以printf函数的语法输入日志
* 智能性能并发让步机制，根据计算机性能自动测算延迟保存系数，不影响业务模块资源引用
* 单例模式运行，不会造成冲突
* 支持Windows环境下的多进程引用

#### 已经明确的使用规则：
 1. 按照printf函数的语法提供输入
 2. 日志处理速度和要写入的数量量有关，数量量特别大时，会有轻微的延迟
 3. info、error、debug、warning四个级别的日志输出函数，会返回已经处理完毕的此条日志消息的长度，处理失败返回0

#### 异步写入，支持高并发，测试数据如下：
|  并发线程数  |  总日志量  |  入队消耗总时长  |   保存消耗总时长   |
| ------------:| ----------:| ----------------:| ------------------:|
|      50      |    15W     |      0.312s      |      1.529s        |
|     500      |    50W     |      1.029s      |      5.086s        |
|    1000      |   100W     |      2.012s      |     10.124s        |
 
### 性能表现
日志进入队列时的性能表现：
![总完成时长](https://github.com/ccpwcn/LogUtilMan/blob/master/%E6%9C%80%E5%A4%A7%E5%85%A5%E9%98%9F%E6%97%B6%E9%95%BF.png)

日志保存到磁盘中的性能表现：
![最大入队时长](https://github.com/ccpwcn/LogUtilMan/blob/master/%E6%80%BB%E5%AE%8C%E6%88%90%E6%97%B6%E9%95%BF.png)

### 公开的接口
```
class DLL_API ILog
{
public:
	virtual ~ILog() = 0;
	virtual size_t info(__in_opt const TCHAR *fmt, ...) = 0;
	virtual size_t error(__in_opt const TCHAR *fmt, ...) = 0;
	virtual size_t debug(__in_opt const TCHAR *fmt, ...) = 0;
	virtual size_t warning(__in_opt const TCHAR *fmt, ...) = 0;
};

extern "C" {
	DLL_API ILog * GetClassObject(__in const LPCTSTR lpszLogFilename);
	DLL_API void ReleaseClassObject(__in const ILog * instance);
}
```