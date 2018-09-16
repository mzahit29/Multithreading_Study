// MultiThreadingStudy.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using namespace std;

mutex cout_mutex;


void f(const int countdown) {
	for (int i = countdown; i > 0; --i)
	{
		cout << "Counting down: " << i << endl;
		this_thread::sleep_for(chrono::milliseconds(200));
	}
}

int main()
{
	std::cout << "Hello World!\n";
	vector<thread> th_vector(2);

	thread t1 = thread(f, 2);
	th_vector[0] = thread{ f, 5 };


	for (auto& t : th_vector) {
		if (t.joinable()) t.join();
	}

	t1.join();
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
