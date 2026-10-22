# Spark build toggles (included from root CMakeLists.txt).

option(SPARK_BUILD_TESTS "Build Spark unit tests (GoogleTest)" OFF)
option(SPARK_BUILD_ENGINE_SHARED "Build SparkEngine as a shared library (OFF = static)" OFF)
option(SPARK_BUILD_ENGINE_DEMOS "Compile shell/demo sources into SparkEngine (OFF for slim editor/game DLL)" ON)
option(SPARK_BUILD_EDITOR "Compile spark/editor authoring module into SparkEngine" OFF)
option(SPARK_BUILD_SPARK_EDITOR "Build SparkEditor standalone executable (requires SPARK_BUILD_EDITOR)" OFF)
option(SPARK_BUILD_DEMO "Build SparkDemo sample executable (shell + demos)" ON)
option(SPARK_BUILD_GAMEPLAY "Build SparkGameplay kit (combat, pickups, game flow)" ON)
