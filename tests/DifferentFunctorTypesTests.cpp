#include <gmock/gmock.h>
#include <ThreadPool.hpp>

class Handlers
{
public:
	MOCK_METHOD(void, VoidHandler, ());
	MOCK_METHOD(int, IntHandler, ());
};

class ThreadPoolFixture : public ::testing::Test
{
public:
	ThreadPoolFixture()
		: pool(std::thread::hardware_concurrency())
	{
	}
protected:
	ThreadPool pool;
	::testing::NiceMock<Handlers> mocker;
};


TEST_F(ThreadPoolFixture, Lambda_ReturnsVoid)
{
	EXPECT_CALL(mocker, VoidHandler()).Times(1);

	auto future = pool.AddTask([this]
	{
		mocker.VoidHandler();
	});
	ASSERT_NO_THROW(future.get());
}

TEST_F(ThreadPoolFixture, Lambda_ReturnsInt)
{
	ON_CALL(mocker, IntHandler()).WillByDefault(::testing::Return(5));
	EXPECT_CALL(mocker, IntHandler()).Times(1);

	auto future = pool.AddTask([this]
	{
		return mocker.IntHandler();
	});
	ASSERT_EQ(5, future.get());
}

TEST_F(ThreadPoolFixture, Functor_ReturnsVoid)
{
	struct Functor
	{
		Functor(std::atomic_bool& flag)
			: flag(flag)
		{}
		void operator()()
		{
			flag.store(true);
		}
		std::atomic_bool& flag;
	};

	std::atomic_bool flag{ false };

	auto future = pool.AddTask(Functor{ flag });

	ASSERT_NO_THROW(future.get());
	ASSERT_TRUE(flag.load());
}

TEST_F(ThreadPoolFixture, Functor_ReturnsInt)
{
	struct Functor
	{
		int operator()()
		{
			return 7;
		}
	};

	auto future = pool.AddTask(Functor{});

	ASSERT_EQ(7, future.get());
}
