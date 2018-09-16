#include "pch.h"
#include "Examples.h"
#include <queue>
#include <mutex>
#include <iostream>

using namespace std;

queue<int> q;

mutex mq;

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

