#include "threadpool.h"
#include <iostream>
#include <atomic>

std::atomic<int> all = 0;
int task_test()
{
    for (int i = 0; i < 10000; i++)
    {
        all.fetch_add(1);
    }

    return rand() % 1234;
}

int main()
{

    std::vector<std::future<int>> results;
    for (int i = 0; i < 10; i++)
    {
        results.emplace_back(TLThreadpool::getInstance().postTask(task_test));
    }

    TLThreadpool::getInstance().waitAll();

    std::cout << all << std::endl;

    return 0;
}