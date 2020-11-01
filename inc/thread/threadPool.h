#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_
#include <memory>

#ifdef _MSC_VER
#define THREAD_POOL_EXPORTS
#endif

#ifdef THREAD_POOL_EXPORTS
#define THREAD_POOL_CLASS __declspec(dllexport)
#elif (defined LINUX || defined ANDROID)
#define THREAD_POOL_CLASS __attribute__((visibility("default")))
#else
#define THREAD_POOL_CLASS 
#endif

class THREAD_POOL_CLASS ThreadPool {
public:
	typedef void(*callback)(void* arg);
	typedef enum {Level0, Level1, Level2}TaskLevel;
	typedef std::pair<callback, void*> CallbackPair;
	typedef std::pair<TaskLevel, CallbackPair> TaskPair;

	explicit ThreadPool(uint16_t num = 3);
	~ThreadPool();

	// 开启线程池
	bool start();

	// 关闭线程池
	bool stop();

	// 添加任务
	void addTask(const TaskPair& task);

	void addTask(callback cb, void* arg);

protected:
	typedef struct {
		void* arg;
		callback cb;
	}threadPoolPair;
	ThreadPool(const ThreadPool&);
	ThreadPool& operator=(const ThreadPool&);

	// 线程函数
	void onThreadFunc();
	// 获取任务
	CallbackPair pop();
private:
	struct threadPoolImpl;
	threadPoolImpl* m_impl;
};

#endif