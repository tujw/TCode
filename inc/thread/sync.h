#ifndef SYNC_H_
#define SYNC_H_
#include <memory>

#ifdef _MSC_VER
#define SYNC_EXPORTS
#endif

#ifdef SYNC_EXPORTS
#define SYNC_CLASS __declspec(dllexport)
#elif (defined LINUX || defined ANDROID)
#define SYNC_CLASS __attribute__((visibility("default")))
#else
#define SYNC_CLASS 
#endif


class SYNC_CLASS CSyncEvent
{
		struct SyncEventImpl;
		typedef std::unique_ptr<SyncEventImpl> SyncEvImplPtr;
public:
	CSyncEvent();
	~CSyncEvent();

	/*
	// 初始化事件
	// bEvent -初始有信号
	// bReset	-true -自动设置信号 false-手动设置
	*/
	bool Initialize(bool bEvent, bool bReset);

	/*
	设置信号
	*/
	bool SetEvent();

	/*
	等待信号
	millisecond - 等于-1 永久等待
	*/
	int WaitEvent(unsigned long millisecond = -1);
private:
	CSyncEvent(const CSyncEvent&);
	CSyncEvent&operator=(const CSyncEvent&);
private:
	SyncEvImplPtr m_impl;
};


class SYNC_CLASS CSyncMutex 
{
		struct SyncMutexImpl;
		typedef std::unique_ptr<SyncMutexImpl> SyncMutexPtr;
public:
	CSyncMutex();
	~CSyncMutex();

	/*
	* 加锁
	*/
	void Lock();

	/*
	* 解锁
	*/
	void Unlock();
private:
	CSyncMutex(const CSyncMutex&);
	CSyncMutex& operator=(const CSyncMutex&);

private:
	SyncMutexPtr m_impl;
};


class SYNC_CLASS CSyncSmart 
{
	class SyncSmartImpl;
	typedef std::unique_ptr<SyncSmartImpl> SyncSmartPtr;
public:
	explicit CSyncSmart(CSyncMutex* ptr);
	~CSyncSmart();
	/*
	* 加锁
	*/
	void Lock();
	/*
	* 加锁，将原来的同步对象解锁，再对新的同步对象加锁
	*/
	void Lock(CSyncMutex* ptr);
	/*
	* 解锁
	*/
	void Unlock();
private:
	CSyncSmart(const CSyncSmart&);
	CSyncSmart& operator=(const CSyncSmart&);

private:
	SyncSmartPtr m_impl;
};
#endif