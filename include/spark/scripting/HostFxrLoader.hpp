#pragma once

#include <hostfxr.h>
#include <coreclr_delegates.h>

#include <cstddef>

namespace Spark {

struct HostFxrApi {
    hostfxr_initialize_for_runtime_config_fn initConfig = nullptr;
    hostfxr_get_runtime_delegate_fn getDelegate = nullptr;
    hostfxr_close_fn close = nullptr;
};

/** Dynamically loads hostfxr from the path returned by nethost. Returns false on failure. */
[[nodiscard]] bool LoadHostFxr(const char* hostfxrPath, HostFxrApi& outApi, char* errorBuffer, std::size_t errorBufferSize);

}  // namespace Spark
