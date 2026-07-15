#pragma once

#include "spark/core/Utility.hpp"

#include <cstddef>
#include <new>

namespace Spark {

template<typename T>
class WeakPtr;

namespace Detail {

template<typename T>
struct SharedControlBlock {
    std::size_t strongCount = 0;
    std::size_t weakCount = 0;
    T* object = nullptr;

    static SharedControlBlock* Allocate(T* ptr) {
        auto* block = static_cast<SharedControlBlock*>(::operator new(sizeof(SharedControlBlock)));
        block->strongCount = 1;
        block->weakCount = 0;
        block->object = ptr;
        return block;
    }

    void AddStrongRef() noexcept { ++strongCount; }

    void ReleaseStrong() noexcept {
        if (strongCount > 0) {
            --strongCount;
        }
        if (strongCount == 0 && object != nullptr) {
            delete object;
            object = nullptr;
        }
        if (strongCount == 0 && weakCount == 0) {
            ::operator delete(this);
        }
    }

    void AddWeakRef() noexcept { ++weakCount; }

    void ReleaseWeak() noexcept {
        if (weakCount > 0) {
            --weakCount;
        }
        if (strongCount == 0 && weakCount == 0) {
            ::operator delete(this);
        }
    }

    [[nodiscard]] bool IsExpired() const noexcept { return object == nullptr; }
};

enum class SharedPtrAcquire { kExistingStrongRef };

}  // namespace Detail

/**
 * Shared ownership with reference counting (single-threaded counts).
 */
template<typename T>
class SharedPtr {
public:
    constexpr SharedPtr() noexcept = default;

    explicit SharedPtr(T* rawPointer) {
        if (rawPointer != nullptr) {
            control = Detail::SharedControlBlock<T>::Allocate(rawPointer);
        }
    }

    ~SharedPtr() { Reset(); }

    SharedPtr(const SharedPtr& other) noexcept : control(other.control) {
        if (control != nullptr) {
            control->AddStrongRef();
        }
    }

    SharedPtr& operator=(const SharedPtr& other) noexcept {
        if (this != &other) {
            SharedPtr tmp(other);
            Swap(control, tmp.control);
        }
        return *this;
    }

    SharedPtr(SharedPtr&& other) noexcept : control(other.control) { other.control = nullptr; }

    SharedPtr& operator=(SharedPtr&& other) noexcept {
        if (this != &other) {
            Reset();
            control = other.control;
            other.control = nullptr;
        }
        return *this;
    }

    void Reset() noexcept {
        if (control != nullptr) {
            control->ReleaseStrong();
            control = nullptr;
        }
    }

    [[nodiscard]] T* Get() const noexcept { return control != nullptr ? control->object : nullptr; }
    [[nodiscard]] T& operator*() const { return *Get(); }
    [[nodiscard]] T* operator->() const noexcept { return Get(); }

    explicit constexpr operator bool() const noexcept { return Get() != nullptr; }

    [[nodiscard]] std::size_t GetStrongCount() const noexcept {
        return control != nullptr ? control->strongCount : 0;
    }

    friend class WeakPtr<T>;

private:
    Detail::SharedControlBlock<T>* control = nullptr;

    explicit SharedPtr(Detail::SharedControlBlock<T>* block, Detail::SharedPtrAcquire) noexcept
        : control(block) {}

    static SharedPtr CreateFromExistingControlBlock(Detail::SharedControlBlock<T>* block) noexcept {
        if (block == nullptr || block->object == nullptr) {
            return SharedPtr();
        }
        block->AddStrongRef();
        return SharedPtr(block, Detail::SharedPtrAcquire::kExistingStrongRef);
    }
};

/**
 * Non-owning weak reference; Pin() promotes to SharedPtr if object is alive.
 */
template<typename T>
class WeakPtr {
public:
    constexpr WeakPtr() noexcept = default;

    WeakPtr(const SharedPtr<T>& shared) noexcept : control(shared.control) {
        if (control != nullptr) {
            control->AddWeakRef();
        }
    }

    WeakPtr(const WeakPtr& other) noexcept : control(other.control) {
        if (control != nullptr) {
            control->AddWeakRef();
        }
    }

    WeakPtr& operator=(const WeakPtr& other) noexcept {
        if (this != &other) {
            ReleaseWeak();
            control = other.control;
            if (control != nullptr) {
                control->AddWeakRef();
            }
        }
        return *this;
    }

    WeakPtr& operator=(const SharedPtr<T>& shared) noexcept {
        ReleaseWeak();
        control = shared.control;
        if (control != nullptr) {
            control->AddWeakRef();
        }
        return *this;
    }

    ~WeakPtr() { ReleaseWeak(); }

    WeakPtr(WeakPtr&& other) noexcept : control(other.control) { other.control = nullptr; }

    WeakPtr& operator=(WeakPtr&& other) noexcept {
        if (this != &other) {
            ReleaseWeak();
            control = other.control;
            other.control = nullptr;
        }
        return *this;
    }

    void Reset() noexcept { ReleaseWeak(); }

    [[nodiscard]] bool Expired() const noexcept {
        return control == nullptr || control->IsExpired();
    }

    [[nodiscard]] SharedPtr<T> Pin() const noexcept {
        if (control == nullptr || control->object == nullptr) {
            return SharedPtr<T>();
        }
        return SharedPtr<T>::CreateFromExistingControlBlock(control);
    }

private:
    Detail::SharedControlBlock<T>* control = nullptr;

    void ReleaseWeak() noexcept {
        if (control != nullptr) {
            control->ReleaseWeak();
            control = nullptr;
        }
    }
};

template<typename T, typename... Args>
[[nodiscard]] SharedPtr<T> MakeShared(Args&&... args) {
    return SharedPtr<T>(new T(Forward<Args>(args)...));
}

}  // namespace Spark
