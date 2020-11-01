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

	// ��ʼ������
	int Init(const char* addr, const int port);
	// �ͷŷ���
	int Uninit();
	// ѭ��
	int Dispatch();
	// ����socket �رջص�
	int SetQuitCallback(onQuitCallback cb, void* userParams);
	// ����socket�ɶ��¼��ص�
	int SetReadCallback(onReadCallback cb, void* userParams);
	// ����fd ��д
	int SendSockMessage(SOCK_FD fd, const char* msg, const int size);

private:
	class BaseImpl;
	BaseImpl* m_baseImpl;
};
#endif