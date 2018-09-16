#include "pch.h"
#include "Examples.h"
#include <queue>
#include <mutex>
#include <iostream>

using namespace std;

queue<int> q;

mutex mq;

condition_variable cond;

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

	thread t1 { [&m]()
	{
		unique_lock<mutex> ul(m, defer_lock);
		ul.lock();
		cout << "[" << this_thread::get_id() << "] Inside thread" << endl;
		ul.unlock(); // Release lock not to make other threads wait on cout's mutex

		this_thread::sleep_for(chrono::seconds(5));
	}
	};

	thread t2 { [&m]()
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

