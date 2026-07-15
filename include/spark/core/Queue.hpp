#pragma once

#include "spark/core/Utility.hpp"

#include <cstddef>
#include <new>

namespace Spark {

/**
 * FIFO queue backed by a contiguous ring buffer.
 */
template<typename T>
class Queue {
public:
    Queue() = default;

    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    Queue(Queue&& other) noexcept
        : data(other.data), cap(other.cap), head(other.head), count(other.count) {
        other.data = nullptr;
        other.cap = 0;
        other.head = 0;
        other.count = 0;
    }

    Queue& operator=(Queue&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        Clear();
        ::operator delete(data);
        data = other.data;
        cap = other.cap;
        head = other.head;
        count = other.count;
        other.data = nullptr;
        other.cap = 0;
        other.head = 0;
        other.count = 0;
        return *this;
    }

    ~Queue() {
        Clear();
        ::operator delete(data);
    }

    [[nodiscard]] std::size_t GetSize() const noexcept { return count; }
    [[nodiscard]] bool IsEmpty() const noexcept { return count == 0; }

    void Clear() noexcept {
        while (count > 0) {
            data[head].~T();
            head = cap > 0 ? (head + 1) % cap : 0;
            --count;
        }
        head = 0;
    }

    void Enqueue(const T& value) {
        EnsureCapacity();
        const std::size_t tail = (head + count) % cap;
        new (data + tail) T(value);
        ++count;
    }

    void Enqueue(T&& value) {
        EnsureCapacity();
        const std::size_t tail = (head + count) % cap;
        new (data + tail) T(MoveTemp(value));
        ++count;
    }

    bool Dequeue(T& outValue) {
        if (count == 0) {
            return false;
        }
        T& front = data[head];
        outValue = MoveTemp(front);
        front.~T();
        head = (head + 1) % cap;
        --count;
        return true;
    }

    [[nodiscard]] T* Peek() noexcept {
        if (count == 0) {
            return nullptr;
        }
        return data + head;
    }

    [[nodiscard]] const T* Peek() const noexcept {
        if (count == 0) {
            return nullptr;
        }
        return data + head;
    }

private:
    T* data = nullptr;
    std::size_t cap = 0;
    std::size_t head = 0;
    std::size_t count = 0;

    void EnsureCapacity() {
        if (count < cap) {
            return;
        }
        const std::size_t newCap = cap == 0 ? 8 : cap * 2;
        T* newData = static_cast<T*>(::operator new(sizeof(T) * newCap));
        for (std::size_t i = 0; i < count; ++i) {
            const std::size_t src = (head + i) % cap;
            new (newData + i) T(MoveTemp(data[src]));
            data[src].~T();
        }
        ::operator delete(data);
        data = newData;
        cap = newCap;
        head = 0;
    }
};

}  // namespace Spark
