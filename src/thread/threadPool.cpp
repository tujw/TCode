#include "threadPool.h"
#include <utility>
#include <queue>
#include <thread>
#include <mutex>
#include <vector>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <condition_variable>

struct threadPoolCompare{
	bool operator()(const ThreadPool::TaskPair& lhs, const ThreadPool::TaskPair& rhs)const{
		return lhs.first > rhs.first;
	}
};

struct ThreadPool::threadPoolImpl{
	typedef std::priority_queue<TaskPair, std::vector<TaskPair>, threadPoolCompare> taskQueue;

	bool										m_isQuit;
	int										m_count;
	taskQueue							m_tasks;
	std::vector<std::thread>	m_threads;
	std::mutex							m_mutex;
	std::condition_variable		m_condition;

	threadPoolImpl()
		: m_isQuit(false)
		, m_count(0)
		, m_tasks()
		, m_threads()
		, m_mutex()
		, m_condition(){}
};


ThreadPool::ThreadPool(uint16_t num)
	: m_impl(new threadPoolImpl)
{
	if(num > 0){
		m_impl->m_count = num;
	}
}

ThreadPool::~ThreadPool()
{
	stop();

	if(m_impl){
		delete m_impl;
	}
	m_impl = nullptr;
}

bool ThreadPool::start(){
	m_impl->m_threads.reserve(m_impl->m_count);
	for(int i = 0; i < m_impl->m_count; ++i){
		m_impl->m_threads.emplace_back(std::thread(&ThreadPool::onThreadFunc, this));
	}
	return true;
}

bool ThreadPool::stop(){
	printf("threadPool::stop() %d\n", std::this_thread::get_id());
	{
		std::lock_guard<std::mutex> locker(m_impl->m_mutex);
		m_impl->m_isQuit = true;
		m_impl->m_condition.notify_all();
	}

	std::for_each(m_impl->m_threads.begin(), m_impl->m_threads.end(), [](std::thread& item){if(item.joinable()){item.join();}});
	m_impl->m_threads.clear();
	while(!m_impl->m_tasks.empty()){m_impl->m_tasks.pop();}
	printf("threadPool::stop() %d exit\n", std::this_thread::get_id());
	return true;
}

void ThreadPool::addTask(const TaskPair& task)
{
	std::lock_guard<std::mutex> locker(m_impl->m_mutex);
	m_impl->m_tasks.push(task);
	m_impl->m_condition.notify_one();
}

void ThreadPool::addTask(callback cb, void* arg){
	TaskPair task;
	task.second.first = cb;
	task.second.second = arg;
	task.first = TaskLevel::Level2;
	std::lock_guard<std::mutex> locker(m_impl->m_mutex);
	m_impl->m_tasks.push(task);
	m_impl->m_condition.notify_one();
}

void ThreadPool::onThreadFunc(){
	printf("threadPool::onThreadFunc() %d\n", std::this_thread::get_id());
	while(!m_impl->m_isQuit){
		CallbackPair pair = pop();
		if (pair.first){ pair.first(pair.second); }
	}
	printf("threadPool::onThreadFunc() %d exit\n", std::this_thread::get_id());
}

ThreadPool::CallbackPair ThreadPool::pop()
{
	std::unique_lock<std::mutex> locker(m_impl->m_mutex);
	while(m_impl->m_tasks.empty() && !m_impl->m_isQuit){
		printf("threadPool::pop() %d wait\n", std::this_thread::get_id());
		m_impl->m_condition.wait(locker);
	}

	CallbackPair pair;
	auto size = m_impl->m_tasks.size();
	if(!m_impl->m_tasks.empty() && !m_impl->m_isQuit){
		auto task = m_impl->m_tasks.top();
		m_impl->m_tasks.pop();
		pair.first = task.second.first;
		pair.second = task.second.second;
		assert(size - 1 == m_impl->m_tasks.size());
	}
	printf("threadPool::pop() %d exit\n", std::this_thread::get_id());
	return pair;
}