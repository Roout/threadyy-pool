#include <iostream>
#include "thread_pool.hpp"


int main() {
    static constexpr std::size_t kWorkers{5};
    klyaksa::ThreadPool pool {kWorkers};
    std::atomic<int> counter;
    std::atomic<bool> stopped { false };
    auto console = pool.Post([&](){
        int iters = 0;
        while (!stopped.load(std::memory_order_acquire)) {
            std::cout << "* ";
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
            iters++;
        }
        return iters;
    }).value();
    for (size_t i = 0; i < 10; i++) {
        (void) pool.Post([&](){ counter++; });
    }
    pool.Start();
    std::this_thread::sleep_for(std::chrono::seconds{2});
    stopped.store(true, std::memory_order_release);
    pool.Stop();
    std::cout << "\nastrics: " << console.get() << '\n';
    std::cout << counter << '\n';

    for (std::size_t i = 0; i < 1000; i++) {
        pool.Start();
        pool.Stop();
    }
    return 0;
}