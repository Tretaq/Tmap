#include <thread>
#include <atomic>
#include <vector>
#include <iostream>

using namespace std;
int main()
{
    atomic<int> counter{0};
    // int counter{0}; //if normal int 2 threads will see the old value  
    vector<thread> threads;
    for (int t = 0; t < 4 ; ++t)
    {
        threads.emplace_back([&counter] // its a lambda
        {
            for (int i = 0 ; i < 100000 ; ++i) counter ++;
        });
    }
    for (auto& th : threads)th.join(); // join makes main thread wait for that thread to finish
    {
        cout << counter << "\n";
    }
}