#ifndef WEBSERVER_H_
#define WEBSERVER_H_
#include <memory>
#ifdef _MSC_VER
#include<WinSock2.h>
#define SOCK_FD SOCKET
#else
#define SOCK_FD int
#endif

#ifdef WIN32
#define WEBSERVER_EXPORTS
#endif

#ifdef WEBSERVER_EXPORTS
#define WEBSERVER_API __declspec(dllexport)
#elif _LINUX_
#define WEBSERVER_API __attribute__((visibility("default")))
#else
#define WEBSERVER_API
#endif

typedef void(*onQuitCallback)(SOCK_FD fd, void* userParams);
typedef void(*onReadCallback)(SOCK_FD fd, const char* msg, int size, void* userParams);

class WEBSERVER_API CWebServer {
public:
	CWebServer();
	~CWebServer();

	// 初始化服务
	int Init(const char* addr, const int port);
	// 释放服务
	int Uninit();
	// 循环
	int Dispatch();
	// 设置socket 关闭回调
	int SetQuitCallback(onQuitCallback cb, void* userParams);
	// 设置socket可读事件回调
	int SetReadCallback(onReadCallback cb, void* userParams);
	// 设置fd 可写
	int SendSockMessage(SOCK_FD fd, const char* msg, const int size);

private:
	class BaseImpl;
	BaseImpl* m_baseImpl;
};
#endif