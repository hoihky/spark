#include "spark/scripting/HostFxrLoader.hpp"

#include <cstring>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace Spark {
namespace {

void SetError(char* buffer, std::size_t size, const char* message) {
    if (buffer == nullptr || size == 0) {
        return;
    }
#if defined(_WIN32)
    strncpy_s(buffer, size, message, _TRUNCATE);
#else
    std::strncpy(buffer, message, size - 1);
    buffer[size - 1] = '\0';
#endif
}

#if !defined(_WIN32)
void* g_hostfxrModule = nullptr;
#endif
#if defined(_WIN32)
HMODULE g_hostfxrModule = nullptr;
#endif

}  // namespace

bool LoadHostFxr(const char* hostfxrPath, HostFxrApi& outApi, char* errorBuffer, std::size_t errorBufferSize) {
    if (hostfxrPath == nullptr || hostfxrPath[0] == '\0') {
        SetError(errorBuffer, errorBufferSize, "empty hostfxr path");
        return false;
    }

#if defined(_WIN32)
    g_hostfxrModule = LoadLibraryA(hostfxrPath);
    if (g_hostfxrModule == nullptr) {
        SetError(errorBuffer, errorBufferSize, "LoadLibraryA(hostfxr) failed");
        return false;
    }
    outApi.initConfig = reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>(
            GetProcAddress(g_hostfxrModule, "hostfxr_initialize_for_runtime_config"));
    outApi.getDelegate = reinterpret_cast<hostfxr_get_runtime_delegate_fn>(
            GetProcAddress(g_hostfxrModule, "hostfxr_get_runtime_delegate"));
    outApi.close = reinterpret_cast<hostfxr_close_fn>(GetProcAddress(g_hostfxrModule, "hostfxr_close"));
#else
    g_hostfxrModule = dlopen(hostfxrPath, RTLD_LAZY | RTLD_LOCAL);
    if (g_hostfxrModule == nullptr) {
        const char* dlErr = dlerror();
        SetError(errorBuffer, errorBufferSize, dlErr != nullptr ? dlErr : "dlopen(hostfxr) failed");
        return false;
    }
    outApi.initConfig = reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>(
            dlsym(g_hostfxrModule, "hostfxr_initialize_for_runtime_config"));
    outApi.getDelegate = reinterpret_cast<hostfxr_get_runtime_delegate_fn>(
            dlsym(g_hostfxrModule, "hostfxr_get_runtime_delegate"));
    outApi.close = reinterpret_cast<hostfxr_close_fn>(dlsym(g_hostfxrModule, "hostfxr_close"));
#endif

    if (outApi.initConfig == nullptr || outApi.getDelegate == nullptr || outApi.close == nullptr) {
        SetError(errorBuffer, errorBufferSize, "hostfxr exports missing");
        return false;
    }

    return true;
}

}  // namespace Spark
