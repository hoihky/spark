#include "spark/gameplay/GameplayDataTable.hpp"

#include <gtest/gtest.h>

#include <cstdio>

TEST(GameplayDataTableTest, LoadsTypedRows) {
    const char* path = "/tmp/spark_gameplay_test_table.txt";
    FILE* file = std::fopen(path, "w");
    ASSERT_NE(file, nullptr);
    std::fprintf(file, "spark_gameplay_v1\n");
    std::fprintf(file, "float enemy.slime.max_hp 12.5\n");
    std::fprintf(file, "int gem.value 3\n");
    std::fprintf(file, "bool pickup.respawn 1\n");
    std::fclose(file);

    Spark::GameplayDataTable table{};
    EXPECT_TRUE(table.TryLoad(path));
    EXPECT_FLOAT_EQ(table.GetFloat("enemy.slime.max_hp", 0.0F), 12.5F);
    EXPECT_EQ(table.GetInt("gem.value", 0), 3);
    EXPECT_TRUE(table.GetBool("pickup.respawn", false));
}
