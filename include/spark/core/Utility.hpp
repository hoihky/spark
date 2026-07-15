#pragma once

namespace Spark {

template<typename T>
struct RemoveReference {
    using Type = T;
};

template<typename T>
struct RemoveReference<T&> {
    using Type = T;
};

template<typename T>
struct RemoveReference<T&&> {
    using Type = T;
};

template<typename T>
constexpr typename RemoveReference<T>::Type&& MoveTemp(T&& value) noexcept {
    return static_cast<typename RemoveReference<T>::Type&&>(value);
}

template<typename T>
constexpr T&& Forward(typename RemoveReference<T>::Type& value) noexcept {
    return static_cast<T&&>(value);
}

template<typename T>
constexpr T&& Forward(typename RemoveReference<T>::Type&& value) noexcept {
    return static_cast<T&&>(value);
}

template<typename T>
constexpr void Swap(T& a, T& b) noexcept {
    T tmp = MoveTemp(a);
    a = MoveTemp(b);
    b = MoveTemp(tmp);
}

}  // namespace Spark
