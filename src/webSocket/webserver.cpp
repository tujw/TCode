#include "webserver.h"
#include <stdio.h>
#include <stdlib.h>
#include <mutex>
#include <thread>
#include "sha1.h"
#include "base64.h"

#ifdef LINUX
#include <unistd.h>  
#include <errno.h>  
#include <string.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h> 

#define sock_close close 
#elif _MSC_VER
#include <WS2tcpip.h>
#pragma comment(lib,"Ws2_32.lib")
#define sock_close closesocket 
#endif

#define MAX_FD 5

template<class T>
class CallbackFunc {
public:
	T m_callbackFunc;
	void* m_userParams;

	CallbackFunc(){}
	CallbackFunc(T cb, void* userParams)
		: m_callbackFunc(cb), m_userParams(userParams) {
	}
};

typedef struct SockBuffer_t{
	int		m_size;
	char*	m_buf;
	std::mutex m_mux;

	SockBuffer_t()
		: m_size(0), m_buf(nullptr){}
	~SockBuffer_t()
	{
		if (nullptr != m_buf) {
			free(m_buf);
		}
		m_size = 0;
		m_buf = nullptr;
	}
}SockBuffer;

typedef struct SocketData_t{
	SOCK_FD m_sock;
	SockBuffer m_read;

	SocketData_t(): m_sock(0), m_read(){}
}SocketData;

class CWebServer::BaseImpl {
public:
	bool				m_bQuit;
	int				m_index;
	std::mutex	m_mux;
	SOCK_FD	m_listen;
	SocketData	m_chlids[MAX_FD];
	CallbackFunc<onQuitCallback>	m_callbackQuit;
	CallbackFunc<onReadCallback> m_callbackRead;

	BaseImpl()
		: m_bQuit(false), m_listen(0), m_mux(), m_index(0){

	}
};

bool IsRealConnect(SOCK_FD fd, char* buffer, int size);
bool Handshake(SOCK_FD fd, char* msg, int size);

CWebServer::CWebServer()
	: m_baseImpl(new BaseImpl)
{

}

CWebServer::~CWebServer()
{
	if (nullptr != m_baseImpl){
		delete m_baseImpl;
	}
	m_baseImpl = nullptr;
}

int CWebServer::Init(const char* addr, const int port)
{
	if ((nullptr == addr) || (0 > port)) {
		return -1;
	}

	SOCK_FD  sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock_fd == -1) {
		perror("create listen socket error\n");
		return -1;
	}else {
		printf("create listen socket successful %d\n", sock_fd);
	}

	// 设置成非阻塞
#ifdef _MSC_VER
	u_long nNoBlock = 1;
	if (ioctlsocket(sock_fd, FIONBIO, &nNoBlock) == SOCKET_ERROR){
		perror("set listen socket nonblock error\n");
		return -1;
	}
	else {
		printf("set listen socket nonblock successful\n");
	}
#else
	if(fcntl(sock_fd, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK) == -1){
		perror("set listen socket nonblock error\n");
		return -1;
	}
	else {
		printf("set listen socket nonblock successful\n");
	}
#endif

	// 设置监听套接字端口号复用
	int opt = 1;
	if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int)) == -1){
		perror("use setsockopt function error\n");
		return -1;
	}
	else {
		printf("set socket opt successfu\n");
	}

	struct sockaddr_in sock_addr = { 0 };
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(port);
	sock_addr.sin_addr.s_addr = INADDR_ANY;
	if (-1 == bind(sock_fd, (const sockaddr*)&sock_addr, sizeof(sock_addr))) {
		perror("bind listen socket error\n");
		return -1;
	}
	else {
		printf("bind successful\n");
	}

	if (-1 == listen(sock_fd, 5)) {
		perror("listen socket error\n");
		return -1;
	}

	m_baseImpl->m_bQuit = false;
	m_baseImpl->m_listen = sock_fd;
	printf("listen socket %d\n", sock_fd);

	return 0;
}

int CWebServer::Uninit()
{
	{
		std::lock_guard<std::mutex> locker(m_baseImpl->m_mux);
		m_baseImpl->m_bQuit = true;
	}

	for (int i = 0; i < m_baseImpl->m_index; ++i) {
		sock_close(m_baseImpl->m_chlids[i].m_sock);
		m_baseImpl->m_chlids[i].m_sock = 0;
	}

	sock_close(m_baseImpl->m_listen);
	m_baseImpl->m_listen = 0;
	m_baseImpl->m_index = 0;
	return 0;
}

int CWebServer::Dispatch()
{
	struct timeval tv = {0};
	fd_set m_set_read;
	SOCK_FD max_fd = m_baseImpl->m_listen;
	while(!m_baseImpl->m_bQuit) {
		FD_ZERO(&m_set_read);
		FD_SET(m_baseImpl->m_listen, &m_set_read);

		tv.tv_sec = 5;
		tv.tv_usec = 0;
		int ret = select(max_fd + 1, &m_set_read, NULL, NULL, &tv);
		if(ret < 0) {
			perror("select error");
			break;
		}
		else if(ret == 0) {
			continue;
		}
		else {
				int count = 0;
				{
					std::lock_guard<std::mutex> locker(m_baseImpl->m_mux);
					count = m_baseImpl->m_index;
				}

				if(FD_ISSET(m_baseImpl->m_listen, &m_set_read)) {
					struct sockaddr_in addr = {0};
					auto len = sizeof(addr);
					SOCK_FD cli_fd = accept(m_baseImpl->m_listen, (sockaddr*)&addr, (int*)&len);
					if(cli_fd < 0) {
#ifdef _MSC_VER
						if(WSAEWOULDBLOCK == WSAGetLastError()) {
							continue;
						}
#else
						if(EWOULDBLOCK == errno) {
							continue;
						}
#endif				
						break;
					}
					else if((count + 1) >= MAX_FD){
						printf("max connections arrive, exit\n");
						sock_close(cli_fd);
					}
					else {
#ifdef _MSC_VER
						u_long opt = 1;
						ioctlsocket(cli_fd, FIONBIO, (u_long *)&opt);
#else
						fcntl(cli_fd, F_SETFL, fcntl(cli_fd, F_GETFL, 0) | O_NONBLOCK);
#endif
						// 验证连接是否合法
						char buf[1024] = {0};
						if(!IsRealConnect(cli_fd, buf, sizeof(buf))){
							sock_close(cli_fd);
							continue;
						}

						// 握手
						if(!Handshake(cli_fd, buf, strlen(buf))) {
							sock_close(cli_fd);
							continue;
						}

						if(max_fd < cli_fd) {max_fd = cli_fd;}
						std::lock_guard<std::mutex> locker(m_baseImpl->m_mux);
						m_baseImpl->m_chlids[count].m_sock = cli_fd;
						printf("new connection client %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
					}
				}
		}
	}

	return 0;
}

int CWebServer::SetQuitCallback(onQuitCallback cb, void* userParams)
{
	if ((nullptr == cb)) { return -1; }

	m_baseImpl->m_callbackQuit.m_callbackFunc = cb;
	m_baseImpl->m_callbackQuit.m_userParams = userParams;
	return 0;
}

int CWebServer::SetReadCallback(onReadCallback cb, void* userParams)
{
	if ((nullptr == cb)) { return -1; }

	m_baseImpl->m_callbackRead.m_callbackFunc = cb;
	m_baseImpl->m_callbackRead.m_userParams = userParams;
	return 0;
}

int CWebServer::SendSockMessage(SOCK_FD fd, const char* msg, const int size)
{
	if ((nullptr == msg) || (size <= 0)) { return -1; }


	unsigned char buf[10] = { 0 };
	buf[0] = 130;
	int head = 0, number = size;
	if (size < 126) {
		head = 2;
		buf[1] = size;
	}
	else if (size < 65536) {
		head = 4; 
		buf[1] = 126;
		buf[2] = number >> 8;
		buf[3] = size & 0xFF;
	}
	else {
		head = 10;
		buf[1] = 127;
		buf[2] = 0;
		buf[3] = 0;
		buf[4] = 0;
		buf[5] = 0;
		buf[6] = number >> 24;
		buf[7] = number >> 16;
		buf[8] = number >> 8;
		buf[9] = number & 0xFF;
	}

	int nSendSize = send(fd, (const char*)buf, head, 0);
	if(nSendSize <= 0) {
#ifdef _MSC_VER
		if(nSendSize == SOCKET_ERROR) {
			sock_close(fd);
			return -1;
		}
#else
		if((nSendSize == EINTR ) || (nSendSize == EWOULDBLOCK ) || (nSendSize == EAGAIN)) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			nSendSize = send(fd, (const char*)buf, head, 0);
		}

		if(nSendSize <= 0) {
			sock_close(fd);
			return -1;
		}
#endif
	}
	else if(nSendSize > 0 && nSendSize < head) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		nSendSize = send(fd, (const char*)buf + nSendSize, head - nSendSize, 0);
		if(nSendSize <= 0) {sock_close(fd);return -1;}
	}

	int pos = 0;
	while((pos = send(fd, (const char*)msg + pos, size - pos, 0)) > 0){
		if(pos == size) {
			break;
		}
		else if(pos <= 0) {
#ifdef _MSC_VER
			if(pos == SOCKET_ERROR) {
				sock_close(fd);
				return -1;
			}
#else
			if((pos == EINTR ) || (pos == EWOULDBLOCK ) || (pos == EAGAIN)){
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}
			else {
				sock_close(fd);
				return -1;
			}
#endif
		}
	}
	return 0;
}


bool IsRealConnect(SOCK_FD fd, char* buffer, int size)
{
	const char* szKey = "GET";
	const char* szWebKey = "Sec-WebSocket-Key";

	int pos = 0;
	char buf[2048] = {0};
	int length = sizeof(buf);
	
	while((pos = recv(fd, buf + pos, length - pos, 0)) > 0){
		if(pos == length) {break;}
	}

	char* index = strstr(buf, szKey);
	if(nullptr == index) {return false;}

	index = strstr(buf, szWebKey);
	if(nullptr == index) {return false;}

	strncpy(buffer, index + 19, 24);
	if(strlen(buffer) <= 0) {return false;}

	return true;
}

bool Handshake(SOCK_FD fd, char* msg, int size)
{
	strcat(msg, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

	unsigned int buf[5] = {0};
	SHA1 sha;
	sha.Reset();
	sha << msg;
	sha.Result(buf);

	for(int i = 0; i < 5; ++i){
		buf[i] = htonl(buf[i]);
	}

	base64 base;
	char buffer[1024] = {0};
	const char* szBase = base.base64_encode((const unsigned char*)buf, 20).c_str();
	strcat(buffer, "HTTP/1.1 101 Switching Protocols\r\nConnection: upgrade\r\nSec-WebSocket-Accept: ");
	strcat(buffer, szBase);
	strcat(buffer, "\r\n");
	strcat(buffer, "Upgrade: websocket\r\n\r\n");

	int length = strlen(buffer);
	int ret = send(fd, (const char*)buffer, length, 0);
	if(ret <= 0) {
		return false;
	}
	else if(ret < length) {
		ret = send(fd, (const char*)buffer + ret, length - ret, 0);
		if(ret <= 0) {return false;}
	}
	return true;
}
