#include <gtest/gtest.h>

#include "spark/core/Function.hpp"
#include "spark/core/Optional.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Vector3.hpp"

namespace {

TEST(OptionalTest, DefaultIsEmpty) {
    Spark::Optional<int> value;
    EXPECT_FALSE(value.HasValue());
    EXPECT_FALSE(static_cast<bool>(value));
}

TEST(OptionalTest, HoldsAndClearsValue) {
    Spark::Optional<int> value = 7;
    EXPECT_TRUE(value.HasValue());
    EXPECT_EQ(*value, 7);
    value.Reset();
    EXPECT_FALSE(value.HasValue());
}

TEST(OptionalTest, ValueOrFallback) {
    Spark::Optional<int> empty;
    Spark::Optional<int> filled = 3;
    EXPECT_EQ(empty.ValueOr(42), 42);
    EXPECT_EQ(filled.ValueOr(42), 3);
}

TEST(OptionalTest, CopyAndMovePreserveState) {
    Spark::Optional<Spark::Vector3> source = Spark::Vector3{1.0F, 2.0F, 3.0F};
    Spark::Optional<Spark::Vector3> copied = source;
    EXPECT_TRUE(copied.HasValue());
    EXPECT_FLOAT_EQ(copied->x, 1.0F);

    Spark::Optional<Spark::Vector3> moved = MoveTemp(source);
    EXPECT_TRUE(moved.HasValue());
    EXPECT_FALSE(source.HasValue());
}

TEST(OptionalTest, FunctionInvokesBoundLambda) {
    int counter = 0;
    Spark::Function<void()> callback = [&counter]() { ++counter; };
    EXPECT_TRUE(callback.IsBound());
    callback();
    EXPECT_EQ(counter, 1);
}

TEST(OptionalTest, NullFunctionPointerIsUnbound) {
    Spark::Function<void(int)> callback = static_cast<void (*)(int)>(nullptr);
    EXPECT_FALSE(callback.IsBound());
    EXPECT_FALSE(static_cast<bool>(callback));
}

TEST(OptionalTest, NullptrConstructionIsUnbound) {
    Spark::Function<void(Spark::GameObject&)> callback = nullptr;
    EXPECT_FALSE(callback.IsBound());
    callback = nullptr;
    EXPECT_FALSE(callback.IsBound());
}

TEST(OptionalTest, NullFunctionPointerCallbackIsSafeToCheck) {
    Spark::Function<void(Spark::GameObject&)> onEnter = nullptr;
    if (onEnter) {
        FAIL() << "null callback must not appear bound";
    }
    SUCCEED();
}

TEST(OptionalTest, FunctionStoredByValueSurvivesTemporaryLambda) {
    int counter = 0;
    struct Holder {
        Spark::Function<int(int)> callback;
        explicit Holder(const Spark::Function<int(int)>& fn) : callback(fn) {}
        int Run(int value) const { return callback(value); }
    };
    const Holder holder([&counter](int value) {
        counter += value;
        return counter;
    });
    EXPECT_EQ(holder.Run(3), 3);
    EXPECT_EQ(holder.Run(5), 8);
}

}  // namespace
