# Thread pool

Make thread pool

## Requirements

- c++23: it uses `std::move_only_function` which you can implement yourself for old standards and compilers
- cmake
- gtest: added as external project downloaded via cmake

## Build

```
cmake --build . --target install --config Debug
```

## Notes

1. To address the issue of executing `Halt()` between evaluating `halt_` variable in this cv's predicate and cv going to sleep ([see 2](#refs)) which leads to undesired block on `cv.wait` despite halting I use `wait_for` with timeout around `100ms`. **UPD**: switched back to mutex for queue cuz we're locking mutex when pushing callback anyway! and `wait_for` need to spin in loop to check whether it's timeout occured or new work appearedW!
2. Use `notify_one` under the lock in `scheduler_test.cpp` to avoid data race against CV: it could be destroyed right after `exec_time` assignment when `notify_one` is being called, e.g. `exec_time.has_value()` already true. Mutex is unlocked (if `finished` is not under the lock). At the same time CV wakes up, checks stop predicate and doesn't wait anymore! Thread with CV can THERIOTICALLY be destroyed before/when `notify_one` in another thread being called. 
To resolve this you can either increase lifetime of CV (shared_ptr, static, etc) or notify under locked `mutex`. See [pthread_cond_signal](#refs)

```C++
// ...
scheduler.ScheduleAt(expectedExecTime, [&]() {
    // { with this scope uncommented data race will occur
        std::lock_guard lock{ mutex };
        exec_time.emplace(std::chrono::steady_clock::now());
    // }
    finished.notify_one();
}); 
std::unique_lock lock{ mutex };
(void) finished.wait_for(lock, maxWaitTime * 2, [&]() {
    return exec_time.has_value();
});
```

# <a name="refs"></a>References

1. [C++ compiler support](https://runebook.dev/en/docs/cpp/compiler_support#C.2B.2B23_library_features)
2. [Multithreading by example: C++ dev blog](https://dev.to/glpuga/multithreading-by-example-the-stuff-they-didn-t-tell-you-4ed8)
3. [pthread_cond_signal under lock](https://stackoverflow.com/questions/47308494/is-there-a-data-race-if-pthread-cond-signal-is-the-last-call-in-a-thread)
