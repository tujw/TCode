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
	// ��ʼ���¼�
	// bEvent -��ʼ���ź�
	// bReset	-true -�Զ������ź� false-�ֶ�����
	*/
	bool Initialize(bool bEvent, bool bReset);

	/*
	�����ź�
	*/
	bool SetEvent();

	/*
	�ȴ��ź�
	millisecond - ����-1 ���õȴ�
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
	* ����
	*/
	void Lock();

	/*
	* ����
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
	* ����
	*/
	void Lock();
	/*
	* ��������ԭ����ͬ������������ٶ��µ�ͬ���������
	*/
	void Lock(CSyncMutex* ptr);
	/*
	* ����
	*/
	void Unlock();
private:
	CSyncSmart(const CSyncSmart&);
	CSyncSmart& operator=(const CSyncSmart&);

private:
	SyncSmartPtr m_impl;
};
#endif