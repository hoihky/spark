#pragma once

#include "spark/core/Optional.hpp"
#include "spark/core/TypeTraits.hpp"
#include "spark/core/Utility.hpp"

#include <cstddef>

namespace Spark {

namespace Detail {

template<typename Stored, typename R, typename... Args>
struct FunctionVtable {
    static R Invoke(void* callable, Args... args) {
        return (*static_cast<Stored*>(callable))(Forward<Args>(args)...);
    }

    static void Destroy(void* callable) noexcept {
        delete static_cast<Stored*>(callable);
    }

    static void Clone(void* src, void*& dst) {
        dst = new Stored(*static_cast<const Stored*>(src));
    }
};

template<typename Stored, typename... Args>
struct FunctionVtable<Stored, void, Args...> {
    static void Invoke(void* callable, Args... args) {
        (*static_cast<Stored*>(callable))(Forward<Args>(args)...);
    }

    static void Destroy(void* callable) noexcept {
        delete static_cast<Stored*>(callable);
    }

    static void Clone(void* src, void*& dst) {
        dst = new Stored(*static_cast<const Stored*>(src));
    }
};

template<typename R, typename... Args>
struct FreeFunctionHolder {
    R (*ptr)(Args...) = nullptr;

    R operator()(Args... args) const {
        return ptr(Forward<Args>(args)...);
    }
};

}  // namespace Detail

/**
 * Type-erased callable wrapper (heap-backed). Replacement for <c>std::function</c>
 * in engine APIs that need lambdas with capture.
 *
 * Null function pointers and <c>nullptr</c> produce an empty (unbound) function,
 * matching <c>std::function</c> behavior.
 */
template<typename Sig>
class Function;

template<typename R, typename... Args>
class Function<R(Args...)> {
public:
    using Result = R;

    Function() noexcept = default;

    Function(NulloptT) noexcept {}

    Function(std::nullptr_t) noexcept {}

    template<typename R2, typename... Args2>
    Function(R2 (*fn)(Args2...)) {
        if (fn != nullptr) {
            BindCallable(Detail::FreeFunctionHolder<R2, Args2...>{fn});
        }
    }

    template<typename R2, typename... Args2>
    Function(R2 (*fn)(Args2...) noexcept) {
        if (fn != nullptr) {
            BindCallable(Detail::FreeFunctionHolder<R2, Args2...>{fn});
        }
    }

    template<typename F>
    Function(F&& callable) {
        BindCallable(Forward<F>(callable));
    }

    Function(const Function& other) { CopyFrom(other); }

    Function(Function&& other) noexcept { MoveFrom(MoveTemp(other)); }

    ~Function() { Reset(); }

    Function& operator=(NulloptT) noexcept {
        Reset();
        return *this;
    }

    Function& operator=(std::nullptr_t) noexcept {
        Reset();
        return *this;
    }

    Function& operator=(const Function& other) {
        if (this != &other) {
            Reset();
            CopyFrom(other);
        }
        return *this;
    }

    Function& operator=(Function&& other) noexcept {
        if (this != &other) {
            Reset();
            MoveFrom(MoveTemp(other));
        }
        return *this;
    }

    template<typename F>
    Function& operator=(F&& callable) {
        Reset();
        BindCallable(Forward<F>(callable));
        return *this;
    }

    [[nodiscard]] explicit operator bool() const noexcept { return IsBound(); }
    [[nodiscard]] bool IsBound() const noexcept { return invoke_ != nullptr; }

    void Reset() noexcept {
        if (callable_ != nullptr && destroy_ != nullptr) {
            destroy_(callable_);
        }
        callable_ = nullptr;
        invoke_ = nullptr;
        destroy_ = nullptr;
        clone_ = nullptr;
    }

    R operator()(Args... args) const {
        if (invoke_ == nullptr) {
            if constexpr (IsSameV<R, void>) {
                return;
            } else {
                return R{};
            }
        }
        return invoke_(callable_, Forward<Args>(args)...);
    }

private:
    using InvokeFn = R (*)(void*, Args...);
    using DestroyFn = void (*)(void*) noexcept;
    using CloneFn = void (*)(void*, void*&);

    void* callable_ = nullptr;
    InvokeFn invoke_ = nullptr;
    DestroyFn destroy_ = nullptr;
    CloneFn clone_ = nullptr;

    template<typename Stored>
    void BindCallable(Stored&& callable) {
        using HeapStored = DecayT<Stored>;
        callable_ = new HeapStored(Forward<Stored>(callable));
        invoke_ = &Detail::FunctionVtable<HeapStored, R, Args...>::Invoke;
        destroy_ = &Detail::FunctionVtable<HeapStored, R, Args...>::Destroy;
        clone_ = &Detail::FunctionVtable<HeapStored, R, Args...>::Clone;
    }

    void CopyFrom(const Function& other) {
        if (!other.callable_) {
            return;
        }
        other.clone_(other.callable_, callable_);
        invoke_ = other.invoke_;
        destroy_ = other.destroy_;
        clone_ = other.clone_;
    }

    void MoveFrom(Function&& other) noexcept {
        callable_ = other.callable_;
        invoke_ = other.invoke_;
        destroy_ = other.destroy_;
        clone_ = other.clone_;
        other.callable_ = nullptr;
        other.invoke_ = nullptr;
        other.destroy_ = nullptr;
        other.clone_ = nullptr;
    }
};

}  // namespace Spark
