#include "mlog.h"
#include <stdint.h>
#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <mutex>
#include <string.h>
#include <stdarg.h>
#include <condition_variable>

#ifndef _MSC_VER
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <dirent.h>
#include <semaphore.h>
#define sz_sprintf snprintf
#define s_access access
#else
#include <io.h>
#include <direct.h>
#include <Shlobj.h>
#include <windows.h>
#define sz_sprintf sprintf_s
#define s_access _access
#endif

static const unsigned int ms = 1000000;
CHelper::CHelper()
	: msg_size(0)
{
	memset(buffer, 0x00, LINE_SIZE);
}

CHelper::CHelper(int line, int thread_num, const char* file, const char* func, const char* level, const char* format, ...)
	: msg_size(0)
{
	if(nullptr == format){return ;}
	uint64_t sec = 0;
#ifdef _MSC_VER
	const char* fileName = strrchr(file, '\\') + 1;
	sec = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock().now().time_since_epoch()).count();
#else

	struct timeval tv;
	gettimeofday(&tv, NULL);
	sec = tv.tv_sec * ms + tv.tv_usec;

	const char* fileName = strrchr(file, '/');
	if(nullptr == fileName)
		fileName = file;

#endif

	char sztime[32] = {0};
	time_t nowsec = sec / ms;
	struct tm *st_time = localtime(&nowsec);
	strftime(sztime, sizeof(sztime), "%Y%m%d %H:%M:%S", st_time);
	
	int tlen = strlen(sztime);
	unsigned int msec = static_cast<unsigned int>(sec % ms);

	const char* pFunc = strrchr(func, ':');
	if(nullptr == pFunc)
	{
		pFunc = func;
	}
	else 
	{
		pFunc+= 1;
	}

	sz_sprintf(sztime + tlen, sizeof(sztime) - tlen, ".%06u", msec);
	sz_sprintf(buffer, sizeof(buffer) - 1, "[%s][%s][%s][%d][%d][%s]:", sztime, fileName, pFunc, thread_num, line, level);

	msg_size = strlen(buffer);
	va_list ap = nullptr;
	va_start(ap, format);
	vprintf(format, ap);
	int n = vsnprintf(buffer + msg_size, LINE_SIZE - msg_size - 1, format, ap);
	msg_size += n;
	buffer[msg_size] = '\n';
	msg_size++;
	va_end(ap);
	ap = nullptr;

}

const int CHelper::size() const
{
	return msg_size;
}

const char* CHelper::data() const
{
	return buffer;
}


class CLineBuffer
{
public:
	int size;
	char buffer[LINE_SIZE];
	CLineBuffer()
		: size(0)
	{
	}
};

class CSem 
{
#ifdef _MSC_VER
#define sem HANDLE
#else
#define sem sem_t
#endif

	sem m_sem;
public:
	CSem(int initNum, int maxNum)
	{
#ifdef _MSC_VER
		m_sem = CreateSemaphore(NULL, initNum, maxNum, NULL);
#else
		sem_init(&m_sem, 0, initNum);
#endif
	}

	void Down()
	{
#ifdef _MSC_VER
		WaitForSingleObject(m_sem, INFINITE);
#else
		sem_wait(&m_sem);
#endif
	}

	void Post()
	{
#ifdef _MSC_VER
		ReleaseSemaphore(m_sem, 1, NULL);
#else
		sem_post(&m_sem);
#endif
	}

	~CSem()
	{
#ifdef _MSC_VER
		CloseHandle(m_sem);
#else
		sem_destroy(&m_sem);
#endif
	}
};

template<int64_t size>
class CircularQueue
{
	bool m_bQuit;
	int m_front;
	int m_tail;
	int m_line_size;
	CSem m_semPush;
	CSem m_semPop;
	std::mutex m_mux;
	CLineBuffer m_lines[size];
public:
	CircularQueue()
		: m_front(0)
		, m_tail(0)
		, m_bQuit(false)
		, m_line_size(size)
		, m_semPop(0, size)
		, m_semPush(size, size)
	{

	}

	void Quit()
	{
		m_bQuit = true;
		m_semPop.Post();
		m_semPush.Post();
	}

	void Push(const char* msg, int len)
	{
		if(m_bQuit) 
		{
			return ;
		}
		m_semPush.Down();

		if(m_bQuit) 
		{
			return ;
		}

		{
			std::lock_guard<std::mutex> locker(m_mux);
			m_lines[m_front].size = (len > LINE_SIZE ? LINE_SIZE : len);
			memcpy(m_lines[m_front].buffer, msg,  m_lines[m_front].size);
			m_front = (m_front + 1) % m_line_size;
		}
		m_semPop.Post();
	}

	CLineBuffer& Top()
	{
		m_semPop.Down();
		return m_lines[m_tail];
	}

	void Pop()
	{
		m_tail = (m_tail + 1) % m_line_size;
		m_semPush.Post();
	}
};


class CMLog::CMLogImpl
{
public:
	bool m_bQuit;
	bool m_bInit;
	int  m_nFileIndex;

	FILE* m_fp;
	char  m_szTime[9];
	configure m_conf;
	std::mutex m_mux;
	int64_t m_file_size;
	CircularQueue<300> m_qeue;
	std::thread m_consumer_thread;
public:
	CMLogImpl()
	{
		m_bQuit = false;
		m_bInit = false;
		m_fp = nullptr;
		m_file_size = 0;
		m_nFileIndex = 0;
		m_conf.level = DEBUG;
		m_conf.nFileDays = 7;
		m_conf.nMaxFileSize = 10;
		memset(m_szTime, 0x00, sizeof(m_szTime));
		memset(m_conf.szDirPath, 0x00, sizeof(m_conf.szDirPath));
		memset(m_conf.szFileName, 0x00, sizeof(m_conf.szFileName));
		strcpy(m_conf.szFileName, "LOG");
	}

	void initDir()
	{
		int len = strlen(m_conf.szDirPath);
#ifdef _MSC_VER
		if(len <= 0)
		{
			wchar_t szDir[MAX_PATH] = {0};
			wchar_t szDoc[MAX_PATH] = {0};
			LPITEMIDLIST pidl = nullptr;
			SHGetSpecialFolderLocation(NULL, CSIDL_LOCAL_APPDATA, &pidl);
			if(pidl && SHGetPathFromIDList(pidl, szDoc))
			{   
				GetShortPathName(szDoc,szDir, _MAX_PATH);
			}

			wcscat(szDir, L"\\cloudwalk\\");
			wcstombs(m_conf.szDirPath, szDir, sizeof(m_conf.szDirPath) - 1);
			_mkdir(m_conf.szDirPath);
		}
#else
		if(len <= 0)
		{	
			printf("dir path empty\n");
			sz_sprintf(m_conf.szDirPath, sizeof(m_conf.szDirPath), "%s", "/tmp/cloudwalk/log/");
		}
		else if(m_conf.szDirPath[len - 1] != '/')
		{
			m_conf.szDirPath[len] = '/';
		}
#endif

		char buffer[1024] = {0};
		for(int i = 0; i < len; ++i)
		{
#ifdef _MSC_VER
			if(m_conf.szDirPath[i] == '\\')
#else
			if(m_conf.szDirPath[i] == '/')
#endif
			{
				buffer[i] = m_conf.szDirPath[i];
#ifdef _MSC_VER
				if(_access(buffer, 0) != 0)
				{
					_mkdir(buffer);
				}
#else
				if(access(buffer, 0) != 0)
				{
					mkdir(buffer, 0755);
				}
#endif
			}
			else
			{
				buffer[i] = m_conf.szDirPath[i];
			}
		}

		if(strlen(m_conf.szFileName) <= 0)
		{
			strcat(m_conf.szFileName, "LOG");
		}
	}

	void createFile(int64_t roll_size)
	{
		removeFile();

		time_t now = time(nullptr);
		struct tm* ptm = localtime(&now);
		char buffer[100] = {0};
		if(nullptr != m_fp)
		{
			fflush(m_fp);
			fclose(m_fp);
			m_fp = nullptr;
		}

		char szNow[10] = {0};
		sz_sprintf(szNow, sizeof(szNow) - 1, "%04d%02d%02d", ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday);
		if(strcmp(m_szTime, szNow) != 0)
		{
			m_nFileIndex = 0;
			memset(m_szTime, 0, sizeof(m_szTime));
			strncpy(m_szTime, szNow, 8);
		}
		
		if(0 == m_nFileIndex)
		{
			sz_sprintf(buffer, sizeof(buffer) - 1, "%s%s_%s.log", m_conf.szDirPath, m_conf.szFileName, m_szTime);
		}
		else
		{
			sz_sprintf(buffer, sizeof(buffer) - 1, "%s%s_%s_%d.log", m_conf.szDirPath, m_conf.szFileName, m_szTime, m_nFileIndex++);
		}
		
		while(0 == s_access(buffer, 0))
		{
			m_fp = fopen(buffer, "a+");
			if(nullptr != m_fp)
			{
				fseek(m_fp,0,SEEK_END);
				m_file_size = ftell(m_fp);
			}

			if(m_file_size < roll_size)
			{
				break;
			}

			if(m_fp)
			{
				fclose(m_fp);
			}

			m_fp = nullptr;
			memset(buffer, 0x00, sizeof(buffer));
			sz_sprintf(buffer, sizeof(buffer) - 1, "%s%s_%s_%d.log", m_conf.szDirPath, m_conf.szFileName, m_szTime, m_nFileIndex++);
		}
		if(m_fp == nullptr)
		{
			m_fp = fopen(buffer, "a+");
			if(nullptr != m_fp)
			{
				fseek(m_fp,0,SEEK_END);
				m_file_size = ftell(m_fp);
			}
		}
	}

	void removeFile()
	{
		bool bHas = false;
		char buffer[1024] = {0};
		const int SECONDS_OF_DAY= 86400;
		int fileLen = strlen(m_conf.szDirPath);
#ifdef _MSC_VER
		if(m_conf.szDirPath[fileLen - 1] == '\\') {
			bHas = true;
			sprintf_s(buffer, sizeof(buffer) - 1, "%s*.*", m_conf.szDirPath);
		}
		else
		{
			sprintf_s(buffer, sizeof(buffer) - 1, "%s%c*.*", m_conf.szDirPath, '\\');
		}

		intptr_t handle;
		_finddata_t findData;
		handle = _findfirst(buffer, &findData);
		if(-1 == handle) 
		{
			return ;
		}

		do
		{
			const char* pos = strstr(findData.name, ".log");
			if(((findData.attrib & _A_SUBDIR) == 0) && (strcmp(findData.name, ".") != 0) && (strcmp(findData.name, "..") != 0) && pos)
			{
				char fileName[1024] = {0};
				if(bHas)
				{
					sprintf_s(fileName, sizeof(fileName) - 1, "%s%s", m_conf.szDirPath, findData.name);
				}
				else
				{
					sprintf_s(fileName, sizeof(fileName) - 1, "%s\\%s", m_conf.szDirPath, findData.name);
				}
				
				time_t now = time(nullptr);
				long diff_sec = difftime(now, findData.time_access);
				int diff_day = diff_sec / SECONDS_OF_DAY;

				if(diff_day >= m_conf.nFileDays)
				{
					remove(fileName);
				}
			}
		} while (_findnext(handle, &findData) == 0);
#else
		DIR *pDir = nullptr;
		struct dirent* ptr = nullptr;
		snprintf(buffer, sizeof(buffer) - 1, "%s", m_conf.szDirPath);
		if(buffer[fileLen - 1] != '/') 
		{
			buffer[fileLen] = '/';
		}

		if(!(pDir = opendir(buffer)))
		{
			return;
		}

		while((ptr = readdir(pDir))!=0)
		{
			const char* pos = strstr(ptr->d_name, ".log");
			if (strcmp(ptr->d_name, ".") != 0 && strcmp(ptr->d_name, "..") != 0 && pos)
			{
				char fileName[1024] = {0};
				snprintf(fileName, sizeof(fileName) - 1, "%s%s", buffer, ptr->d_name);

				struct stat buf;
				stat(fileName,&buf);

				time_t now = time(nullptr);
				long diff_sec = difftime(now, buf.st_atime);
				int diff_day = diff_sec / SECONDS_OF_DAY;

				if(diff_day >= m_conf.nFileDays)
				{
					remove(fileName);
				}
			}
		}
		closedir(pDir);
#endif
	}

	void onConsumer()
	{
		int64_t roll_file_size = m_conf.nMaxFileSize * 1024 * 1024;

		while(!m_bQuit)
		{
			if(m_file_size >= roll_file_size || !m_fp)
			{
				createFile(roll_file_size);
			}
			
			CLineBuffer& buf = m_qeue.Top();
			if(buf.size == 0 && m_bQuit) {break;}

			if(m_fp)
			{
				fwrite(buf.buffer, 1, buf.size, m_fp);
				fflush(m_fp);
				m_file_size += buf.size;
			}
			buf.size = 0;
			m_qeue.Pop();
		}

		if(m_fp)
		{
			fflush(m_fp);
			fclose(m_fp);
		}
		m_fp = nullptr;
	}

	void push(const char* msg, int len)
	{
		m_qeue.Push(msg, len);
	}
};

CMLog::CMLog(Token token)
	: m_impl(new CMLogImpl)
{

}

CMLog::~CMLog()
{
	m_impl->m_qeue.Quit();

	if(m_impl->m_consumer_thread.joinable())
	{
		m_impl->m_consumer_thread.join();
	}

	if(m_impl)
	{
		delete m_impl;
	}
	m_impl = nullptr;
}

bool CMLog::init(const configure* params)
{
	{
		std::lock_guard<std::mutex> locker(m_impl->m_mux);
		if(m_impl->m_bInit){return true;}
		m_impl->m_bInit = true;
	}

	if(nullptr != params)
	{
		m_impl->m_conf.level = params->level;
		m_impl->m_conf.nFileDays = (params->nFileDays <= 0 ? 7 : params->nFileDays);
		m_impl->m_conf.nMaxFileSize = (params->nMaxFileSize <= 0 ? 10 : params->nMaxFileSize);
		memcpy(m_impl->m_conf.szDirPath, params->szDirPath, sizeof(m_impl->m_conf.szDirPath));
		memcpy(m_impl->m_conf.szFileName, params->szFileName, sizeof(m_impl->m_conf.szFileName));
	}

	m_impl->initDir();
	if(!m_impl->m_consumer_thread.joinable())
	{
		m_impl->m_consumer_thread = std::move(std::thread(&CMLog::CMLogImpl::onConsumer, m_impl));
	}
	return true;
}

void CMLog::uninit()
{
	m_impl->m_bQuit = true;
	m_impl->m_qeue.Quit();
	{
		std::lock_guard<std::mutex> locker(m_impl->m_mux);
		m_impl->m_bInit = false;
	}

	if(m_impl->m_consumer_thread.joinable())
	{
		m_impl->m_consumer_thread.join();
	}
}

Level CMLog::level()const
{
	return m_impl->m_conf.level;
}

void CMLog::push(const CHelper& helper)
{
	m_impl->push(helper.data(), helper.size());
}
