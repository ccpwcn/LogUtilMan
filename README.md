# LogUtilMan

Windows应用程序日志库，支持printf语法，支持百万级日志快速输出，为您的应用程序加油🍕🍕🍕

### 日志类
* 支持多线程高并发，实测达到200W级，300W级因为我的电脑太烂了，测不了，如果谁发现BUG，请告诉我
* 支持日志分级，有info、error、debug、warning四个级别
* 支持以printf函数的语法输入日志
* 智能性能并发让步机制，根据计算机性能自动测算延迟保存系数，不影响业务模块资源引用
* 进程内单例模式运行，不会造成资源冲突
* 支持Windows环境下的多进程引用，不会出现问题

#### 已经明确的使用规则：
 1. 按照printf函数的语法提供输入
 2. 日志处理速度和要写入的数量量有关，数量量特别大时，会有轻微的延迟
 3. info、error、debug、warning四个级别的日志输出函数，会返回已经处理完毕的此条日志消息的长度，处理失败返回-1

#### 异步写入，支持高并发
|  并发线程数  |  总日志量  |  入队消耗总时长  |   保存消耗总时长   |
| ------------:| ----------:| ----------------:| ------------------:|
|      10      |     5W     |      0.062s      |      0.312s        |
|      50      |    15W     |      0.312s      |      1.529s        |
|     500      |    50W     |      1.029s      |      5.086s        |


上面的表格是上一个版本的过于追求高并发了，此版本调整了这个设计，使用更加严格的策略控制CPU和内存占用。

高并发场景在客户端软件中其实并不常用，系统资源低占用又好用才是王道。所以改了一版。

当然了，新版本的性能我也进行了实测，有轻微下降，但是并不影响使用。

### 性能表现
日志进入队列时的性能表现：
![总完成时长](https://raw.githubusercontent.com/ccpwcn/LogUtilMan/master/%E6%80%BB%E5%AE%8C%E6%88%90%E6%97%B6%E9%95%BF.png)

日志保存到磁盘中的性能表现：
![最大入队时长](https://raw.githubusercontent.com/ccpwcn/LogUtilMan/master/%E6%9C%80%E5%A4%A7%E5%85%A5%E9%98%9F%E6%97%B6%E9%95%BF.png)

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

### 使用方法
LogUtil是日志库文件  
TestCase是测试用例，里面有库文件引用的详细的示例代码  
只需要ILog.h这个头文件和编译好的DLL就可以使用了  
 1. 使用导出函数GetClassObject获得日志对象实例
 2. 调用日志接口方法，info、error、debug、warning写入日志
 3. 调用ReleaseClassObject关闭日志文件，保存日志数据，这一步是必须的，否则有可能造成资源泄漏或者日志数据丢失
 4. 不要在一个进程内反复创建同一个日志文件的读写实例，防止多头写入造成日志文件覆盖或丢失。