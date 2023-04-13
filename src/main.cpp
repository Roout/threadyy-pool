#include <iostream>
#include "thread_pool.hpp"


int main() {
    klyaksa::ThreadPool pool {2};
    std::atomic<int> counter;
    for (size_t i = 0; i < 10; i++) {
        (void) pool.Post([&](){ counter++; });
    }
    pool.Start();
    std::this_thread::sleep_for(std::chrono::seconds{2});
    pool.Stop();
    std::cout << counter << '\n';
    return 0;
}