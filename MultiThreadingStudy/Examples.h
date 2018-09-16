#pragma once
class Examples
{
public:
	Examples() = default;
	~Examples() = default;



	static void simple_synchronization_over_mutex();
	static void simple_synchronization_over_mutex_unique_lock();
	static void producer_consumer_problem_with_busy_waiting();
	static void producer_consumer_problem_with_conditional_variable();
	static void factorial_with_conditional_wait();
	static void factorial_async_future_promise();
};

