#pragma once

namespace Spark {

template<typename...>
struct VoidImpl {
    using Type = void;
};

template<typename... Ts>
using VoidT = typename VoidImpl<Ts...>::Type;

template<typename T>
struct RemoveCv {
    using Type = T;
};

template<typename T>
struct RemoveCv<const T> {
    using Type = typename RemoveCv<T>::Type;
};

template<typename T>
struct RemoveCv<volatile T> {
    using Type = typename RemoveCv<T>::Type;
};

template<typename T>
struct RemoveCv<const volatile T> {
    using Type = typename RemoveCv<T>::Type;
};

template<typename T>
using RemoveCvT = typename RemoveCv<T>::Type;

namespace Detail {

/** True iff `const volatile D*` can be explicitly upcast to `const volatile B*` via built-in rules (public base or same type). */
template<typename D, typename B, typename Enable = void>
inline constexpr bool IsPublicSubobjectPointerUpcastV = false;

template<typename D, typename B>
inline constexpr bool IsPublicSubobjectPointerUpcastV<D, B,
        VoidT<decltype(static_cast<const volatile B*>(static_cast<const volatile D*>(nullptr)))>> = true;

}  // namespace Detail

/**
 * Same intent as std::derived_from: after stripping cv from both types, Derived must be the same
 * class as Base or a publicly derived class. Private / protected bases make the pointer upcast
 * ill-formed here, so this yields false without the C++ standard library.
 */
template<typename Derived, typename Base>
inline constexpr bool IsDerivedFromV =
        Detail::IsPublicSubobjectPointerUpcastV<RemoveCvT<Derived>, RemoveCvT<Base>>;

template<typename Derived, typename Base>
concept DerivedFrom = IsDerivedFromV<Derived, Base>;

}  // namespace Spark
