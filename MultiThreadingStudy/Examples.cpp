#include "pch.h"
#include "Examples.h"
#include <queue>
#include <mutex>
#include <future>
#include <iostream>

using namespace std;

queue<int> q;

mutex mq;

condition_variable cond;


void Examples::simple_synchronization_over_mutex()
{
	// Mutex for cout's synchronization in threads
	mutex m;

	thread t1{ [&m]()
	{
		lock_guard<mutex> lg(m);
		cout << "[" << this_thread::get_id() << "] Inside thread" << endl;
		this_thread::sleep_for(chrono::seconds(5));
	} };
	thread t2{ [&m]()
	{
		lock_guard<mutex> lg(m);
		cout << "[" << this_thread::get_id() << "] Inside thread" << endl;
	} };

	t1.join();
	t2.join();
}

void Examples::simple_synchronization_over_mutex_unique_lock()
{
	// Mutex for cout's synchronization in threads
	mutex m;

	thread t1{ [&m]()
	{
		unique_lock<mutex> ul(m, defer_lock);
		ul.lock();
		cout << "[" << this_thread::get_id() << "] Inside thread" << endl;
		ul.unlock(); // Release lock not to make other threads wait on cout's mutex

		this_thread::sleep_for(chrono::seconds(5));
	}
	};

	thread t2{ [&m]()
	{
		unique_lock<mutex> ul(m, defer_lock);
		ul.lock();
		cout << "[" << this_thread::get_id() << "] Inside thread" << endl;
		ul.unlock();

		// Do some other stuff which doesn't require shared resource (cout) usage

		ul.lock();
		cout << "[" << this_thread::get_id() << "] Locking mutex to use cout after some processing in thread again." << endl;
		ul.unlock();
	}
	};


	t1.join();
	t2.join();
}

void produce(queue<int>& q)
{
	// Produce 100 numbers and push to queue.
	for (int i = 0; i < 100; ++i)
	{
		this_thread::sleep_for(chrono::milliseconds(10));
		lock_guard<mutex> lg(mq);
		q.push(i);
		cout << "[" << this_thread::get_id() << "] " << "Pushed " << i << " to queue" << endl;
	}
}

void consume(queue<int>& q)
{
	int data{};

	// Consume when a number is placed in queue
	while (data != 99)
	{
		this_thread::sleep_for(chrono::milliseconds(50));
		lock_guard<mutex> lg(mq);
		if (!q.empty())
		{
			data = q.front();
			q.pop();
			cout << "[" << this_thread::get_id() << "] " << "Pulled " << data << " from queue" << endl;
		}
	}
}


void produce_with_condition_var(queue<int>& q)
{
	// Produce 100 numbers and push to queue.
	for (int i = 0; i < 100; ++i)
	{
		this_thread::sleep_for(chrono::milliseconds(10));
		unique_lock<mutex> ul(mq);  // defer_lock is not used therefore mq is locked on construction
		q.push(i);
		ul.unlock();
		cond.notify_one();  // Notify one thread waiting on mq, that queue is free to be used.

		cout << "[" << this_thread::get_id() << "] " << "Pushed " << i << " to queue" << endl;
	}
}

void consume_with_condition_var(queue<int>& q)
{
	int data{};

	// Consume when a number is placed in queue
	while (data != 99)
	{
		this_thread::sleep_for(chrono::milliseconds(50));
		unique_lock<mutex> ul(mq);
		// If notified wake up, check predicate and lock ul. Then continue process.
		// If wakes up without being notified (superious wake up), check the predicate lambda func
		// if it is true, lock and continue process
		// else sleep again until notified.
		cond.wait(ul, [&q]() { return !q.empty(); });
		data = q.front();
		q.pop();
		ul.unlock();

		cout << "[" << this_thread::get_id() << "] " << "Pulled " << data << " from queue" << endl;
	}
}

// One thread will push values to queue and the other will read from the queue.
// Consumer thread needs to be notified by the producer thread, o.w. consumer
// will have to keep polling whether the queue has something. This is called busy
// waiting and not preferred.
void Examples::producer_consumer_problem_with_busy_waiting()
{
	thread t1{ produce, std::ref(q) };
	thread t2{ consume, std::ref(q) };

	t1.join();
	t2.join();
}

void Examples::producer_consumer_problem_with_conditional_variable()
{
	thread t1{ produce_with_condition_var, std::ref(q) };
	thread t2{ consume_with_condition_var, std::ref(q) };

	t1.join();
	t2.join();
}

mutex mutex_fact;
condition_variable cond_fact;
// Lock the mutex_fact to synchronize result parameter with parent
void factorial_with_ref_result_parameter(int x, int& result)
{
	cout << "[" << this_thread::get_id() << "] Calculating factorial of " << x << endl;

	unique_lock<mutex> ul(mutex_fact);
	result = 1;
	for (int i = 2; i <= x; ++i)
	{
		result *= i;
	}
	ul.unlock();
}

// Instead of using async(..) and using the returned future to get the result from the child thread,
// we use the std::ref(result) being passed to the child thread which fills the calculated result
// and main thread can see the result.
void Examples::factorial_with_conditional_wait()
{
	int result{};
	thread t1{ factorial_with_ref_result_parameter, 4, std::ref(result)};
	unique_lock<mutex> ul(mutex_fact);
	//cond.wait(ul, [&result]()  // Waits until notify_one(), spurious wake up may happen not guaranteed, this is why predicate is req.
	cond.wait_for(ul, chrono::seconds(2), [&result]()  // Waits for 2 seconds, then if not notified during this time, wakes up by itself.
	{
		cout << "Inside condition predicate" << endl;
		cout << "Result in parent: " << result << endl;
		return result != 0;
	});
	cout << "Factorial result after factorial_with_ref_result_parameter thread joined parent: " << result << endl;
	ul.unlock();

	t1.join();
}

// This function is used with async(..) to return a value of future<int>
int factorial_to_return_a_future(int x)
{
	cout << "[" << this_thread::get_id() << "] Calculating factorial of " << x << endl;

	int result{ 1 };
	for (int i = 1; i <= x; ++i)
	{
		result *= i;
	}
	return result;
}

// The thread that executes this function will be waiting for a future value.
// This future value will be provided by the parent thread with promise.set_value(..)
// This accomplishes the parent to child thread communication
int factorial_waiting_for_promise(future<int>& fut)
{
	int x = fut.get();
	cout << "[" << this_thread::get_id() << "] Calculating factorial of " << x << endl;

	int result{ 1 };
	for (int i = 1; i <= x; ++i)
	{
		result *= i;
	}
	return result;
}

void Examples::factorial_async_future_promise()
{
	// Child thread sending data to parent thread with async(..)
	// Makes factorial_to_return_a_future run on the parent thread when fut.get() is called
	//future<int> fut = async(launch::deferred, factorial_to_return_a_future, 4);  
	// Makes factorial_to_return_a_future run on the child thread as soon as this async func call is made
	future<int> fut = async(launch::async, factorial_to_return_a_future, 4);  

	this_thread::sleep_for(chrono::seconds(1));
	cout << "[" << this_thread::get_id() << "] Factorial result calculated from child thread: " << fut.get() << endl;


	// Parent thread sending data to child thread. In this case the value of factorial that is to be calculated
	promise<int> p;
	future<int> fut2 = p.get_future();

	future<int> fut3 = async(launch::async, factorial_waiting_for_promise, std::ref(fut2));
	this_thread::sleep_for(chrono::seconds(2));
	// Fullfill your promise, if you don't fullfil it then child thread will be waiting on fut.get() to get the x
	// to calculate the factorial for
	p.set_value(3);
	cout << "[" << this_thread::get_id() << "] Factorial result calculated from child thread" 
		<< " that was waiting for promise to be fullfilled from parent: " << fut3.get() << endl;



}
