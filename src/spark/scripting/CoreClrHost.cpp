#include "spark/scripting/CoreClrHost.hpp"
#include "spark/scripting/HostFxrLoader.hpp"

#include <nethost.h>
#include <coreclr_delegates.h>
#include <hostfxr.h>

#include <array>
#include <string>

namespace Spark {

CoreClrHost::CoreClrHost(Options options) : options(std::move(options)) {}

CoreClrHost::~CoreClrHost() = default;

bool CoreClrHost::LoadAndInitialize(const SparkHostApi& hostApi, std::string& outError) {
    outError.clear();
    lastError.clear();

    std::array<char, 1024> hostfxrPath{};
    std::size_t hostfxrPathSize = hostfxrPath.size();
    const int getPathRc = get_hostfxr_path(hostfxrPath.data(), &hostfxrPathSize, nullptr);
    if (getPathRc != 0) {
        lastError = "get_hostfxr_path failed (install .NET 8 SDK / runtime)";
        outError = lastError;
        return false;
    }

    std::array<char, 512> loaderError{};
    HostFxrApi hostfxr{};
    if (!LoadHostFxr(hostfxrPath.data(), hostfxr, loaderError.data(), loaderError.size())) {
        lastError = loaderError.data();
        outError = lastError;
        return false;
    }

    hostfxr_handle cxt = nullptr;
    const int initRc = hostfxr.initConfig(options.runtimeConfigPath.c_str(), nullptr, &cxt);
    if (initRc != 0 || cxt == nullptr) {
        lastError = "hostfxr_initialize_for_runtime_config failed: " + std::to_string(initRc);
        outError = lastError;
        return false;
    }

    void* loadAssemblyFn = nullptr;
    if (hostfxr.getDelegate(cxt, hdt_load_assembly_and_get_function_pointer, &loadAssemblyFn) != 0 ||
        loadAssemblyFn == nullptr) {
        lastError = "hdt_load_assembly_and_get_function_pointer unavailable";
        outError = lastError;
        hostfxr.close(cxt);
        return false;
    }

    auto loadAssemblyAndGetFn = reinterpret_cast<load_assembly_and_get_function_pointer_fn>(loadAssemblyFn);

    void* entryFn = nullptr;
    const int loadRc = loadAssemblyAndGetFn(
            options.assemblyPath.c_str(),
            options.typeName.c_str(),
            options.methodName.c_str(),
            UNMANAGEDCALLERSONLY_METHOD,
            nullptr,
            &entryFn);

    hostfxr.close(cxt);

    if (loadRc != 0 || entryFn == nullptr) {
        lastError = "load_assembly_and_get_function_pointer failed: " + std::to_string(loadRc);
        outError = lastError;
        return false;
    }

    auto initialize = reinterpret_cast<SparkScriptEntryInitializeFn>(entryFn);
    const int managedRc = initialize(&hostApi);
    if (managedRc != 0) {
        lastError = "ScriptHostEntry.Initialize returned " + std::to_string(managedRc);
        outError = lastError;
        return false;
    }

    return true;
}

}  // namespace Spark
