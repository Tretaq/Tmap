#include <thread>
#include <atomic>
#include <vector>
#include <iostream>

using namespace std;
int main()
{
    atomic<int> counter{0};
    vector<thread> threads;
    for (int t = 0; t < 4 ; ++t)
    {
        threads.emplace_back([&counter]()
        {
            for (int i = 0 ; i < 100000 ; ++i) counter ++;
        });
    }
    for (auto& th : threads)
    {
        cout << counter << "\n";
    }
}