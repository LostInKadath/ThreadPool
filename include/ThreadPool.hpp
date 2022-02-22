/* ThreadPool.
* 
* Класс, представляющий пул потоков, способных выполнять различные задачи.
* 
* Концептуально представляет собой одну очередь, куда складываются клиентские задачи,
* и несколько потоков, которые эту очередь разгребают, выполняя задачи.
* 
* Когда клиент кладет задачу в очередь, он получает объект std::future,
* с помощью которого он сможет потом достать результат выполнения задачи.
* 
* В качестве задач можно скармливать функторы, лямбды, указатели на функции, объекты std::function и пр.
* 
* Задачи могут возвращать значения или возвращать void -- в этом случае из std::future невозможно достать результат.
* 
* Пример использования:
*	ThreadPool pool(5);		// Создаем пул из пяти рабочих потоков
* 
*	auto future1 = pool.AddTask([](int x) { return x + 10; }, 3);	// Отдали задачу -- получили std::future
*	auto answer1 = future1.get();									// Получили ответ из синхронного std::future::get()
* 
*	auto future2 = pool.AddTask([] { std::cout << "hello"; });		// Отдали задачу -- получили std::future
*	future2.get();													// Убедились, что задача выполнена
* 
* Часто задаваемые вопросы по AddTask() и WorkerThread():
* 
* 1. А если foo() выполнится раньше, чем мы вернем std::future клиенту?
* 	Когда foo() выполнится, она проставит ответ в std::packaged_task.
* 	Будучи в методе AddTask(), мы просто возьмем std::future от std::packaged_task с уже проставленным ответом и отдадим клиенту.
* 
* 2. А если foo() выполнится, когда мы выйдем из AddTask() -- тогда уничтожится std::packaged_task. Откуда возьмется результат?
* 	Так как std::packaged_task упакован в std::shared_ptr, то он будет жить, пока на него есть ссылки.
* 	Когда мы формируем foo(), она добавляет новую ссылку на std::packaged_task.
* 	Поэтому даже когда мы выйдем из AddTask(), std::packaged_task останется жить на куче, пока не выполнится foo().
* 	Когда foo() выполнится, у нас к тому моменту уже будет std::future, в который std::packaged_task перед смертью проставит ответ.
* 
* 3. А вот если foo() выполнится и сразу же перезатрется следующей foo() из очереди -- не страшно?
* 	Не foo() возвращает ответ клиенту, а std::packaged_task через std::future.
* 	foo() нужна только для того, чтобы выполнить тело std::packaged_task, чтобы она отдала ответ std::future.
* 	Каждая foo() в очереди "настроена" на свою std::packaged_task.
* 	Поэтому как только foo() выполнилась, она больше не нужна и смело может быть перезатерта.
* 
*/

#pragma once
#include <atomic>
#include <deque>
#include <future>
#include <vector>

class ThreadPool
{
	template <typename Type>
	class Queue
	{
	public:
		void push(Type&& value)
		{
			std::lock_guard<std::mutex> lock(m_guard);
			m_queue.push_back(std::move(value));
		}

		bool pop(Type& value)
		{
			std::lock_guard<std::mutex> lock(m_guard);
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
	ThreadPool(size_t threads = std::thread::hardware_concurrency());

	virtual ~ThreadPool();

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

	ThreadPool(ThreadPool&&) = default;
	ThreadPool& operator=(ThreadPool&&) = default;

public:
	template<typename Func, typename ...Args>
	auto AddTask(Func&& f, Args&&...args) -> std::future<decltype(f(args...))>
	{
		// Мы получили функцию и ее аргументы. Тип функции -- decltype(f(args...))(Args...).
		
		// Сначала преобразуем ее в функцию единого типа -- decltype(f(args...))(void).
		// Затем упакуем ее в std::packaged_task, чтобы вернуть std::future.
		auto taskPtr = std::make_shared<std::packaged_task<decltype(f(args...))(void)>>(
			std::bind(std::forward<Func>(f), std::forward<Args...>(args...))
		);

		// Создаем функцию, где выполнится тело std::packaged_task, и кладем ее в очередь на выполнение.
		// Функция имеет прототип void(void), потому что std::packaged_task::operator()() возвращает void.
		auto foo = std::function<void(void)>([taskPtr]{ (*taskPtr)(); });

		// Положим задачу в рабочий поток
		m_queue.push(std::move(foo));

		// Наружу возвращаем std::future, из которой позже можно будет вытянуть результат.
		return taskPtr->get_future();
	}

	template<typename Func>
	auto AddTask(Func&& f) -> std::future<decltype(f())>
	{
		// Мы получили функцию без аргументов. Тип функции -- decltype(f())(void).

		// Упакуем ее в std::packaged_task, чтобы вернуть std::future.
		auto taskPtr = std::make_shared<std::packaged_task<decltype(f())(void)>>(
			std::forward<Func>(f)
		);

		// Создаем функцию, где выполнится тело std::packaged_task, и кладем ее в очередь на выполнение.
		// Функция имеет прототип void(void), потому что std::packaged_task::operator()() возвращает void.
		auto foo = std::function<void(void)>([taskPtr] { (*taskPtr)(); });

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

	Queue<std::function<void(void)>> m_queue;
};
