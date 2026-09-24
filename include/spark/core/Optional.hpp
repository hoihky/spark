#pragma once

#include "spark/core/TypeTraits.hpp"
#include "spark/core/Utility.hpp"

#include <cstddef>
#include <new>

namespace Spark {

struct NulloptT {
    struct SecretTag {};
    explicit constexpr NulloptT(SecretTag) noexcept {}
};

inline constexpr NulloptT Nullopt{NulloptT::SecretTag{}};

/**
 * A value that may or may not be present. Drop-in replacement for <c>std::optional</c>
 * without pulling in the C++ standard library containers layer.
 */
template<typename T>
class Optional {
public:
    constexpr Optional() noexcept = default;

    constexpr Optional(NulloptT) noexcept {}

    constexpr Optional(const T& value) { Construct(value); }

    constexpr Optional(T&& value) { Construct(MoveTemp(value)); }

    Optional(const Optional& other) {
        if (other.HasValue()) {
            Construct(*other.Ptr());
        }
    }

    Optional(Optional&& other) noexcept {
        if (other.HasValue()) {
            Construct(MoveTemp(*other.Ptr()));
            other.DestroyValue();
        }
    }

    ~Optional() { Reset(); }

    Optional& operator=(NulloptT) noexcept {
        Reset();
        return *this;
    }

    Optional& operator=(const Optional& other) {
        if (this == &other) {
            return *this;
        }
        if (other.HasValue()) {
            if (HasValue()) {
                *Ptr() = *other.Ptr();
            } else {
                Construct(*other.Ptr());
            }
        } else {
            Reset();
        }
        return *this;
    }

    Optional& operator=(Optional&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        if (other.HasValue()) {
            if (HasValue()) {
                *Ptr() = MoveTemp(*other.Ptr());
            } else {
                Construct(MoveTemp(*other.Ptr()));
            }
            other.DestroyValue();
        } else {
            Reset();
        }
        return *this;
    }

    Optional& operator=(const T& value) {
        if (HasValue()) {
            *Ptr() = value;
        } else {
            Construct(value);
        }
        return *this;
    }

    Optional& operator=(T&& value) {
        if (HasValue()) {
            *Ptr() = MoveTemp(value);
        } else {
            Construct(MoveTemp(value));
        }
        return *this;
    }

    template<typename... Args>
    T& Emplace(Args&&... args) {
        Reset();
        ConstructFromArgs(Forward<Args>(args)...);
        return *Ptr();
    }

    void Reset() noexcept {
        if (HasValue()) {
            DestroyValue();
        }
    }

    [[nodiscard]] bool HasValue() const noexcept { return engaged_; }

    [[nodiscard]] T& Value() & {
        return *Ptr();
    }

    [[nodiscard]] const T& Value() const& {
        return *Ptr();
    }

    [[nodiscard]] T&& Value() && {
        return MoveTemp(*Ptr());
    }

    [[nodiscard]] T ValueOr(const T& fallback) const& {
        return HasValue() ? *Ptr() : fallback;
    }

    [[nodiscard]] T ValueOr(T&& fallback) const& {
        return HasValue() ? *Ptr() : MoveTemp(fallback);
    }

    [[nodiscard]] explicit operator bool() const noexcept { return HasValue(); }

    [[nodiscard]] T* operator->() noexcept { return Ptr(); }
    [[nodiscard]] const T* operator->() const noexcept { return Ptr(); }

    [[nodiscard]] T& operator*() & noexcept { return *Ptr(); }
    [[nodiscard]] const T& operator*() const& noexcept { return *Ptr(); }
    [[nodiscard]] T&& operator*() && noexcept { return MoveTemp(*Ptr()); }

private:
    alignas(T) unsigned char storage_[sizeof(T)]{};
    bool engaged_ = false;

    [[nodiscard]] T* Ptr() noexcept { return reinterpret_cast<T*>(storage_); }
    [[nodiscard]] const T* Ptr() const noexcept { return reinterpret_cast<const T*>(storage_); }

    template<typename... Args>
    void ConstructFromArgs(Args&&... args) {
        new (Ptr()) T(Forward<Args>(args)...);
        engaged_ = true;
    }

    void Construct(const T& value) { ConstructFromArgs(value); }

    void Construct(T&& value) { ConstructFromArgs(MoveTemp(value)); }

    void DestroyValue() noexcept {
        Ptr()->~T();
        engaged_ = false;
    }
};

template<typename T>
[[nodiscard]] constexpr bool operator==(const Optional<T>& lhs, NulloptT) noexcept {
    return !lhs.HasValue();
}

template<typename T>
[[nodiscard]] constexpr bool operator==(NulloptT, const Optional<T>& rhs) noexcept {
    return !rhs.HasValue();
}

template<typename T>
[[nodiscard]] constexpr bool operator!=(const Optional<T>& lhs, NulloptT) noexcept {
    return lhs.HasValue();
}

template<typename T>
[[nodiscard]] constexpr bool operator!=(NulloptT, const Optional<T>& rhs) noexcept {
    return rhs.HasValue();
}

template<typename T>
[[nodiscard]] Optional<DecayT<T>> MakeOptional(T&& value) {
    return Optional<DecayT<T>>(Forward<T>(value));
}

template<typename T>
[[nodiscard]] bool operator==(const Optional<T>& lhs, const T& rhs) {
    return lhs.HasValue() && lhs.Value() == rhs;
}

template<typename T>
[[nodiscard]] bool operator==(const T& lhs, const Optional<T>& rhs) {
    return rhs.HasValue() && lhs == rhs.Value();
}

template<typename T>
[[nodiscard]] bool operator!=(const Optional<T>& lhs, const T& rhs) {
    return !(lhs == rhs);
}

template<typename T>
[[nodiscard]] bool operator!=(const T& lhs, const Optional<T>& rhs) {
    return !(lhs == rhs);
}

}  // namespace Spark
