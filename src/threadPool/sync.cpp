#include "sync.h"

#ifdef _MSC_VER
#include <Windows.h>
typedef HANDLE SyncEvent;
#else
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#define sem_t SyncEvent;
#endif

struct CSyncEvent::SyncEventImpl{
	SyncEvent event;
	bool bEvent;
	bool bReset;

	CSyncEvent::SyncEventImpl()
	{
		bEvent = false;
		bReset = false;
		event = NULL;
	}
};

CSyncEvent::CSyncEvent()
	: m_impl(new SyncEventImpl){
}

CSyncEvent::~CSyncEvent()
{
	if(nullptr == m_impl) {return ;}

#ifdef _MSC_VER
	CloseHandle(m_impl->event);
#else
	sem_destroy(&m_impl->event);
#endif

}

bool CSyncEvent::Initialize(bool bEvent, bool bReset)
{
#ifdef _MSC_VER
	m_impl->event = CreateEvent(NULL, bReset ? TRUE : FALSE, bEvent ? TRUE : FALSE, NULL);
#else
	m_impl->bEvent = bEvent;
	m_impl->bReset = bReset;
	sem_init(&m_impl->event, 0, bEvent ? 1 : 0);
#endif
	return true;
}

bool CSyncEvent::SetEvent()
{
#ifdef _MSC_VER
	::SetEvent(m_impl->event);
#else
	sem_post(&m_impl->event);
#endif
	return true;
}

int CSyncEvent::WaitEvent(unsigned long millisecond)
{
	int ret  = 0;
#ifdef _MSC_VER
	ret = WaitForSingleObject(m_impl->event, millisecond);
#else
	struct timeval now = {0};
	struct timespec out_time = {0};
	gettimeofday(&now, NULL);
	out_time.tv_sec = 0;
	out_time.tv_nsec = millisecond * 1000;
	ret = sem_timedwait(&m_impl->event, &out_time);
#endif
	if(m_impl->bReset) {SetEvent();}
	return ret;
}



struct CSyncMutex::SyncMutexImpl {
#ifdef _MSC_VER
	typedef CRITICAL_SECTION  SyncMutex;
#else
	typedef pthread_mutex_t  SyncMutex;
#endif

	SyncMutex m_mutex;
};


CSyncMutex::CSyncMutex()
	: m_impl(new SyncMutexImpl)
{
#ifdef _MSC_VER
	InitializeCriticalSection(&m_impl->m_mutex);
#else
	pthread_mutex_init(&m_impl->m_mutex);
#endif
}

CSyncMutex::~CSyncMutex()
{
#ifdef _MSC_VER
	DeleteCriticalSection(&m_impl->m_mutex);
#else
	pthread_mutex_destory(&m_impl->m_mutex);
#endif
}

void CSyncMutex::Lock()
{
#ifdef _MSC_VER
	EnterCriticalSection(&m_impl->m_mutex);
#else
	pthread_mutex_lock(&m_impl->m_mutex);
#endif
}

void CSyncMutex::Unlock()
{
#ifdef _MSC_VER
	LeaveCriticalSection(&m_impl->m_mutex);
#else
	pthread_mutex_unlock(&m_impl->m_mutex);
#endif
}

class CSyncSmart::SyncSmartImpl{
public:
	CSyncMutex* m_mutexPtr;
	SyncSmartImpl() : m_mutexPtr(nullptr){}
};

CSyncSmart::CSyncSmart(CSyncMutex* ptr)
	: m_impl(new SyncSmartImpl){
		m_impl->m_mutexPtr = ptr;
		
		if(m_impl->m_mutexPtr){
			m_impl->m_mutexPtr->Lock();
		}
}

CSyncSmart::~CSyncSmart()
{
	if(m_impl->m_mutexPtr){
		m_impl->m_mutexPtr->Unlock();
	}
}

void CSyncSmart::Lock()
{
		if(m_impl->m_mutexPtr){
			m_impl->m_mutexPtr->Lock();
		}
}

void CSyncSmart::Lock(CSyncMutex* ptr)
{
	if(m_impl->m_mutexPtr){
		m_impl->m_mutexPtr->Unlock();
	}
	m_impl->m_mutexPtr = ptr;
	if(m_impl->m_mutexPtr){
		m_impl->m_mutexPtr->Lock();
	}
}

void CSyncSmart::Unlock()
{
	if(m_impl->m_mutexPtr){
		m_impl->m_mutexPtr->Unlock();
	}
}