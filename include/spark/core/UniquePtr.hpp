#pragma once

#include "spark/core/Utility.hpp"

#include <cstddef>

namespace Spark {

/**
 * Unique ownership; move-only. Same role as std::unique_ptr.
 */
template<typename T>
class UniquePtr {
public:
    constexpr UniquePtr() noexcept = default;

    explicit constexpr UniquePtr(T* pointer) noexcept : ptr(pointer) {}

    ~UniquePtr() { Reset(); }

    UniquePtr(const UniquePtr&) = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;

    constexpr UniquePtr(UniquePtr&& other) noexcept : ptr(other.ptr) { other.ptr = nullptr; }

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this != &other) {
            Reset();
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    [[nodiscard]] T& operator*() const { return *ptr; }
    [[nodiscard]] T* operator->() const noexcept { return ptr; }
    [[nodiscard]] T* Get() const noexcept { return ptr; }

    explicit constexpr operator bool() const noexcept { return ptr != nullptr; }

    void Reset(T* pointer = nullptr) noexcept {
        if (ptr != nullptr) {
            delete ptr;
        }
        ptr = pointer;
    }

    [[nodiscard]] T* Release() noexcept {
        T* out = ptr;
        ptr = nullptr;
        return out;
    }

private:
    T* ptr = nullptr;
};

template<typename T>
[[nodiscard]] constexpr bool operator==(const UniquePtr<T>& lhs, std::nullptr_t) noexcept {
    return lhs.Get() == nullptr;
}

template<typename T>
[[nodiscard]] constexpr bool operator!=(const UniquePtr<T>& lhs, std::nullptr_t) noexcept {
    return lhs.Get() != nullptr;
}

template<typename T>
[[nodiscard]] constexpr bool operator==(std::nullptr_t, const UniquePtr<T>& rhs) noexcept {
    return rhs.Get() == nullptr;
}

template<typename T>
[[nodiscard]] constexpr bool operator!=(std::nullptr_t, const UniquePtr<T>& rhs) noexcept {
    return rhs.Get() != nullptr;
}

template<typename T>
class UniquePtr<T[]> {
public:
    constexpr UniquePtr() noexcept = default;

    explicit constexpr UniquePtr(T* pointer) noexcept : ptr(pointer) {}

    ~UniquePtr() { Reset(); }

    UniquePtr(const UniquePtr&) = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;

    constexpr UniquePtr(UniquePtr&& other) noexcept : ptr(other.ptr) { other.ptr = nullptr; }

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this != &other) {
            Reset();
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    [[nodiscard]] T& operator[](std::size_t index) const { return ptr[index]; }
    [[nodiscard]] T* Get() const noexcept { return ptr; }
    explicit constexpr operator bool() const noexcept { return ptr != nullptr; }

    void Reset(T* pointer = nullptr) noexcept {
        if (ptr != nullptr) {
            delete[] ptr;
        }
        ptr = pointer;
    }

    [[nodiscard]] T* Release() noexcept {
        T* out = ptr;
        ptr = nullptr;
        return out;
    }

private:
    T* ptr = nullptr;
};

template<typename T>
[[nodiscard]] constexpr bool operator==(const UniquePtr<T[]>& lhs, std::nullptr_t) noexcept {
    return lhs.Get() == nullptr;
}

template<typename T>
[[nodiscard]] constexpr bool operator!=(const UniquePtr<T[]>& lhs, std::nullptr_t) noexcept {
    return lhs.Get() != nullptr;
}

template<typename T>
[[nodiscard]] constexpr bool operator==(std::nullptr_t, const UniquePtr<T[]>& rhs) noexcept {
    return rhs.Get() == nullptr;
}

template<typename T>
[[nodiscard]] constexpr bool operator!=(std::nullptr_t, const UniquePtr<T[]>& rhs) noexcept {
    return rhs.Get() != nullptr;
}

template<typename T, typename... Args>
[[nodiscard]] UniquePtr<T> MakeUnique(Args&&... args) {
    return UniquePtr<T>(new T(Forward<Args>(args)...));
}

template<typename T>
[[nodiscard]] UniquePtr<T[]> MakeUnique(std::size_t count) {
    return UniquePtr<T[]>(new T[count]());
}

}  // namespace Spark
