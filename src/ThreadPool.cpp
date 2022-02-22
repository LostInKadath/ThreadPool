#include "..\include\ThreadPool.hpp"

ThreadPool::ThreadPool(size_t threads)
	: m_poolSize(threads)
{
	m_workers.reserve(m_poolSize);
	for (size_t i = 0; i < m_poolSize; ++i)
		m_workers.emplace_back([this] { WorkerThread(); });
}

ThreadPool::~ThreadPool()
{
	m_stopping = true;
	for (auto& worker : m_workers)
		if (worker.joinable())
			worker.join();
}

void ThreadPool::WorkerThread()
{
	std::function<void(void)> foo;
	while (!m_stopping)
	{
		if (!m_queue.pop(foo))
		{
			// TODO: сейчас здесь активное ожидание -- можно переделать на cv'шку.
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}

		foo();
		// После отработки foo можно спокойно удалить: `foo = {};`
		// Ее задача лишь в том, чтобы выполнить тело std::packaged_task, которая положит результат в std::future.
	}
}
