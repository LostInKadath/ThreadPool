#include <gmock/gmock.h>
#include <ThreadPool.hpp>

TEST(ThreadPool, NoThreads)
{
	ThreadPool pool(0);
}

TEST(ThreadPool, LongTask_ReturnsVoid_Destruction)
{
	std::future<void> future;
	{
		ThreadPool localPool(1);
		future = localPool.AddTask([]
		{
			std::this_thread::sleep_for(std::chrono::seconds(10));
		});
	}
	ASSERT_TRUE(future.valid());
	ASSERT_THROW(future.get(), std::future_error);
}

TEST(ThreadPool, LongTask_ReturnsInt_Destruction)
{
	std::future<int> future;
	{
		ThreadPool localPool(1);
		future = localPool.AddTask([]
		{
			std::this_thread::sleep_for(std::chrono::seconds(10));
			return 7;
		});
	}
	ASSERT_TRUE(future.valid());
	ASSERT_THROW(future.get(), std::future_error);
}
