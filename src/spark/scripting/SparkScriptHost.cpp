#include "spark/engine/Engine.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/scripting/CoreClrHost.hpp"
#include "spark/scripting/ManagedGameBridge.hpp"
#include "spark/scripting/SparkInterop.h"

#include <filesystem>
#include <iostream>
#include <string>

namespace {

namespace fs = std::filesystem;

SparkManagedGameCallbacks g_callbacks{};
bool g_gameRegistered = false;

constexpr const char* kDefaultRuntimeConfig =
        "scripting/samples/HelloCsGame/bin/Release/net8.0/HelloCsGame.runtimeconfig.json";
constexpr const char* kDefaultAssembly =
        "scripting/samples/HelloCsGame/bin/Release/net8.0/HelloCsGame.dll";

[[nodiscard]] fs::path FindRepoRoot() {
    fs::path dir = fs::current_path();
    for (int depth = 0; depth < 12; ++depth) {
        if (fs::exists(dir / "CMakeLists.txt") && fs::exists(dir / "scripting" / "samples" / "HelloCsGame")) {
            return dir;
        }
        if (!dir.has_parent_path() || dir == dir.parent_path()) {
            break;
        }
        dir = dir.parent_path();
    }
    return fs::current_path();
}

[[nodiscard]] std::string ResolveExistingPath(const char* path) {
    if (path == nullptr || path[0] == '\0') {
        return {};
    }
    const fs::path input(path);
    if (fs::exists(input)) {
        return fs::absolute(input).string();
    }
    const fs::path fromCwd = fs::current_path() / input;
    if (fs::exists(fromCwd)) {
        return fs::absolute(fromCwd).string();
    }
    const fs::path fromRepo = FindRepoRoot() / input;
    if (fs::exists(fromRepo)) {
        return fs::absolute(fromRepo).string();
    }
    return {};
}

int RegisterManagedGame(const SparkManagedGameCallbacks* callbacks) {
    if (callbacks == nullptr || callbacks->onAttach == nullptr || callbacks->onUpdate == nullptr) {
        return -1;
    }
    g_callbacks = *callbacks;
    g_gameRegistered = true;
    return 0;
}

}  // namespace

extern "C" SPARK_SCRIPT_API int spark_script_host_run(int argc, const char* const* argv) {
    const char* runtimeConfigArg = (argc >= 2) ? argv[1] : kDefaultRuntimeConfig;
    const char* assemblyArg = (argc >= 3) ? argv[2] : kDefaultAssembly;

    const std::string runtimeConfigPath = ResolveExistingPath(runtimeConfigArg);
    const std::string assemblyPath = ResolveExistingPath(assemblyArg);
    if (runtimeConfigPath.empty() || assemblyPath.empty()) {
        std::cerr << "Could not find managed game files.\n"
                  << "  runtimeconfig: " << runtimeConfigArg << '\n'
                  << "  assembly:      " << assemblyArg << '\n'
                  << "Build HelloCsGame first:\n"
                  << "  dotnet build scripting/samples/HelloCsGame/HelloCsGame.csproj -c Release\n"
                  << "Or build CMake target SparkScriptingBuild.\n"
                  << "Working directory should be the Spark repo root (for assets/).\n";
        return 2;
    }

    SparkHostApi hostApi{};
    hostApi.structSize = sizeof(SparkHostApi);
    hostApi.registerManagedGame = RegisterManagedGame;

    Spark::CoreClrHost::Options options;
    options.runtimeConfigPath = runtimeConfigPath;
    options.assemblyPath = assemblyPath;
    if (argc >= 5) {
        options.typeName = argv[3];
        options.methodName = argv[4];
    }

    Spark::CoreClrHost host(std::move(options));
    std::string error;
    if (!host.LoadAndInitialize(hostApi, error)) {
        std::cerr << "CoreCLR host failed: " << error << '\n';
        return 1;
    }

    if (!g_gameRegistered) {
        std::cerr << "Managed game did not register callbacks.\n";
        return 1;
    }

    auto game = Spark::MakeUnique<Spark::ManagedGameBridge>();
    game->SetCallbacks(g_callbacks);
    Spark::Engine engine(Spark::UniquePtr<Spark::IGame>(game.Release()));
    engine.Run();
    return 0;
}

int main(int argc, char** argv) {
    return spark_script_host_run(argc, const_cast<const char* const*>(argv));
}
