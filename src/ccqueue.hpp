// Source: https://gist.github.com/Roout/c3be2d97809758c3f6936c6b238c3b3a
#pragma once
 
#include <mutex>
#include <condition_variable>
#include <array>
#include <cassert>
#include <optional>
#include <atomic>
#include <type_traits>
 
template<typename T, std::size_t Capacity>
class CcQueue {
public:
    static_assert(std::is_move_constructible_v<T>);
    static_assert(std::is_move_assignable_v<T>);
    static_assert(std::is_default_constructible_v<T>);
 
    static constexpr std::size_t kCapacity { Capacity };
 
    using element = T;
    using container = std::array<element, kCapacity>;
 
    CcQueue(bool sentinel = true) 
        : front_ { 0 }
        , back_ { 0 }
        , size_ { 0 }
        , sentinel_ { sentinel }
    {
        assert(front_ == back_);
    }
 
    // return true if value was pushed successfully (queue is not full)
    // otherwise return false on failure and doesn't block
    [[nodiscard]] bool TryPush(element cmd) {
        bool is_pushed = false;
        if (std::unique_lock<std::mutex> lock { mutex_ };
            !IsFull()
        ) {
            PushBack(std::move(cmd));
            lock.unlock();
            is_pushed = true;
            // notify consumers
            notifier_.notify_one();
        }
        return is_pushed;
    }
 
    // return front element if queue isn't empty
    // otherwise blocks
    // Note: it ignores sentinel so you can't stop consumer thread
    [[nodiscard]] element Pop() {
        std::unique_lock<std::mutex> lock { mutex_ };
        notifier_.wait(lock, [this]() {
            return !IsEmpty();
        });
        return PopFront();
    }
 
    // return front element if queue is not empty
    // return nullopt if queue is empty and doesn't have sentinel (== false)
    // otherwise (queue is empty and has sentinel) block
    [[nodiscard]] std::optional<element> TryPop() {
        std::optional<element> result{};
 
        std::unique_lock<std::mutex> lock { mutex_ };
        notifier_.wait(lock, [this]() {
            // wait (block) while the <empty> queue has <sentinel>
            return !IsEmpty() || !sentinel_.load(std::memory_order_relaxed);
        });
        if (!IsEmpty()) {
            result.emplace(PopFront());
        }
        lock.unlock();
        return result;
    }
 
    void DisableSentinel() noexcept {
        sentinel_.store(false, std::memory_order_relaxed);
        notifier_.notify_all();
    }

    void EnableSentinel() noexcept {
        sentinel_.store(true, std::memory_order_relaxed);
       // no need to notify as noone wait on it: notifier_.notify_all();
    }
 
private:
 
    void PushBack(element&& value) noexcept {
        assert(!IsFull());
        container_[back_] = std::move(value);
        back_ = (back_ + 1) % kCapacity;
        size_++;
    }
 
    [[nodiscard]] element PopFront() noexcept {
        assert(!IsEmpty());
        element value = std::move(container_[front_]);
        front_ = (front_ + 1) % kCapacity;
        size_--;
        return value;
    }
 
    [[nodiscard]] bool IsEmpty() const noexcept {
        return size_ == 0;
    }
 
    [[nodiscard]] bool IsFull() const noexcept {
        return size_ == kCapacity;
    }
 
    std::mutex mutex_;
    std::condition_variable notifier_;
    container container_;
    std::size_t front_ { 0 };
    std::size_t back_ { 0 };
    std::size_t size_ { 0 };
    std::atomic<bool> sentinel_;
}; 
