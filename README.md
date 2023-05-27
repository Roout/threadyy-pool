# Thread pool

Make thread pool

## Requirements

- c++23: it uses `std::move_only_function` which you can implement yourself for old standards and compilers
- cmake
- gtest: added as external project downloaded via cmake

## Build

```
cd build
cmake  .. -DDOWNLOAD_GTEST=ON -DBUILD_STATIC_LIB=ON -DBUILD_TESTS=OFF -DBUILD_EXE=OFF
cmake --build . --target install --config Debug
```

## Notes

1. To address the issue of executing `Halt()` between evaluating `halt_` variable in this cv's predicate and cv going to sleep ([see 2](#refs)) which leads to undesired block on `cv.wait` despite halting I use `wait_for` with timeout around `100ms`. **UPDATE**: switched back to mutex for queue because we're locking mutex when pushing callback anyway so using atomic variable won't give any speedup: I feel like approach with `wait_for` which will have to spin in the loop to check whether it's timeout occured or new work appeared cost more CPU cycles then locking mutex!
2. Use `notify_one` under the lock in `scheduler_test.cpp` to avoid data race against CV: it could be destroyed right after `exec_time` assignment when `notify_one` is being called, e.g. `exec_time.has_value()` already true. Mutex is unlocked (if `finished` is not under the lock). At the same time CV wakes up, checks stop predicate and doesn't wait anymore! Thread with CV can THERIOTICALLY be destroyed before/when `notify_one` in another thread being called. 
To resolve this you can either increase lifetime of CV (shared_ptr, static, etc) or notify under locked `mutex`. See [pthread_cond_signal](#refs)
3. For passing references I decide to pass args using `std::ref/std::cref` wrappers. Instead of using `std::tuple` I prefer `std::ref`. See [Perfect forwaring and capture](#refs)
4. For MSVC compiler: you cannot `Post` move-only callable due to bug: they afrad to break ABI. I use `std::promise - std::future` pair to emultate `std::packaged_task` behaviour: get future and set exception on need. I probably need replace `std::packaged_task` too and leave only workaround for MSVC but I don't want for now... Just leave it to the future ME. For bug issue see [MSVC (5)](#refs).

```C++
// Notes#2 ...
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

## Todo

- [ ] decide whether to get rid of `_MSC_VER` and use `std::promise - std::future` pair instead of `std::packaged_task`. See Notes (#4)

# <a name="refs"></a>References

1. [C++ compiler support](https://runebook.dev/en/docs/cpp/compiler_support#C.2B.2B23_library_features)
2. [Multithreading by example: C++ dev blog](https://dev.to/glpuga/multithreading-by-example-the-stuff-they-didn-t-tell-you-4ed8)
3. [`pthread_cond_signal` under lock](https://stackoverflow.com/questions/47308494/is-there-a-data-race-if-pthread-cond-signal-is-the-last-call-in-a-thread)
4. [Perfect forwaring and capture](https://stackoverflow.com/questions/73066229/perfectly-forwarding-lambda-capture-in-c20-or-newer)
5. [MSVC bug: `std::package_task` from movable](https://github.com/microsoft/STL/issues/321)
