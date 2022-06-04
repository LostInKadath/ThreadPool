#pragma once
#include <atomic>
#include <deque>
#include <future>
#include <vector>

class ThreadPool final
{
	template <typename Type>
	class TasksQueue
	{
	public:
		void push(Type&& value)
		{
			std::lock_guard lock{ m_guard };
			m_queue.push_back(std::move(value));
		}

		bool pop(Type& value)
		{
			std::lock_guard lock{ m_guard };
			if (m_queue.empty())
				return false;

			value = m_queue.front();
			m_queue.pop_front();
			return true;
		}

	private:
		std::mutex m_guard;
		std::deque<Type> m_queue;
	};

public:
	explicit ThreadPool(size_t threads);
	~ThreadPool();

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

	ThreadPool(ThreadPool&&) = default;
	ThreadPool& operator=(ThreadPool&&) = default;

public:
	template<typename Func, typename ...Args>
	auto AddTask(Func&& f, Args&&...args)
	{
		using ReturnType = std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>;
		
		// Сначала преобразуем исходную функцию в функцию единого типа -- ReturnType(void).
		// Затем упакуем ее в std::packaged_task, чтобы вернуть std::future.
		// shared_ptr нужен для упаковки movable-only объектов, типа unique_ptr.
		auto taskPtr = std::make_shared<std::packaged_task<ReturnType(void)>>(
			std::bind(std::forward<Func>(f), std::forward<Args>(args)...)
		);

		// Создаем функцию, где выполнится тело std::packaged_task, и кладем ее в очередь на выполнение.
		// Функция имеет прототип void(void), потому что std::packaged_task::operator()() возвращает void.
		auto foo = std::function<void(void)>([taskPtr]{ (*taskPtr)(); });

		// Положим задачу в рабочий поток
		m_queue.push(std::move(foo));

		// Наружу возвращаем std::future, из которой позже можно будет вытянуть результат.
		return taskPtr->get_future();
	}

private:
	void WorkerThread();

private:
	size_t m_poolSize;

	std::atomic_bool m_stopping{ false };
	std::vector<std::thread> m_workers;

	TasksQueue<std::function<void(void)>> m_queue;
};
