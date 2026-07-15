#pragma once

#include "spark/core/Utility.hpp"

#include <cstddef>
#include <new>

namespace Spark {

/**
 * Dynamic array (contiguous storage). Grows geometrically. Move-only if T is move-only.
 */
template<typename T>
class Array {
public:
    Array() = default;

    Array(const Array& other) : count(other.count), capacity(other.count) {
        if (count > 0) {
            data = static_cast<T*>(::operator new(sizeof(T) * capacity));
            for (std::size_t i = 0; i < count; ++i) {
                new (data + i) T(other.data[i]);
            }
        }
    }

    Array& operator=(const Array& other) {
        if (this == &other) {
            return *this;
        }
        Clear();
        Reserve(other.count);
        for (std::size_t i = 0; i < other.count; ++i) {
            PushBack(other.data[i]);
        }
        return *this;
    }

    Array(Array&& other) noexcept : data(other.data), count(other.count), capacity(other.capacity) {
        other.data = nullptr;
        other.count = 0;
        other.capacity = 0;
    }

    Array& operator=(Array&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        DestroyRange(0, count);
        ::operator delete(data);
        data = other.data;
        count = other.count;
        capacity = other.capacity;
        other.data = nullptr;
        other.count = 0;
        other.capacity = 0;
        return *this;
    }

    ~Array() {
        DestroyRange(0, count);
        ::operator delete(data);
    }

    [[nodiscard]] std::size_t GetSize() const noexcept { return count; }
    [[nodiscard]] std::size_t GetCapacity() const noexcept { return capacity; }
    [[nodiscard]] bool IsEmpty() const noexcept { return count == 0; }

    [[nodiscard]] T* GetData() noexcept { return data; }
    [[nodiscard]] const T* GetData() const noexcept { return data; }

    [[nodiscard]] T& operator[](std::size_t index) noexcept { return data[index]; }
    [[nodiscard]] const T& operator[](std::size_t index) const noexcept { return data[index]; }

    [[nodiscard]] T& GetLast() noexcept { return data[count - 1]; }
    [[nodiscard]] const T& GetLast() const noexcept { return data[count - 1]; }

    void Clear() noexcept {
        DestroyRange(0, count);
        count = 0;
    }

    void Reserve(std::size_t newCapacity) {
        if (newCapacity <= capacity) {
            return;
        }
        Reallocate(newCapacity);
    }

    /** Sets size; new elements are default-constructed. Shrinking destroys trailing elements. */
    void Resize(std::size_t newSize) {
        if (newSize < count) {
            DestroyRange(newSize, count);
            count = newSize;
            return;
        }
        if (newSize > count) {
            Reserve(newSize);
            while (count < newSize) {
                new (data + count) T();
                ++count;
            }
        }
    }

    void PushBack(const T& value) {
        EnsureSpace(1);
        new (data + count) T(value);
        ++count;
    }

    void PushBack(T&& value) {
        EnsureSpace(1);
        new (data + count) T(MoveTemp(value));
        ++count;
    }

    void PopBack() {
        if (count == 0) {
            return;
        }
        --count;
        data[count].~T();
    }

    void RemoveAt(std::size_t index) {
        if (index >= count) {
            return;
        }
        data[index].~T();
        for (std::size_t i = index + 1; i < count; ++i) {
            new (data + i - 1) T(MoveTemp(data[i]));
            data[i].~T();
        }
        --count;
    }

private:
    T* data = nullptr;
    std::size_t count = 0;
    std::size_t capacity = 0;

    void DestroyRange(std::size_t from, std::size_t to) noexcept {
        for (std::size_t i = from; i < to; ++i) {
            data[i].~T();
        }
    }

    void EnsureSpace(std::size_t extra) {
        if (count + extra <= capacity) {
            return;
        }
        std::size_t newCap = capacity == 0 ? 8 : capacity;
        while (newCap < count + extra) {
            newCap *= 2;
        }
        Reallocate(newCap);
    }

    void Reallocate(std::size_t newCapacity) {
        T* newData = static_cast<T*>(::operator new(sizeof(T) * newCapacity));
        for (std::size_t i = 0; i < count; ++i) {
            new (newData + i) T(MoveTemp(data[i]));
            data[i].~T();
        }
        ::operator delete(data);
        data = newData;
        capacity = newCapacity;
    }
};

}  // namespace Spark
