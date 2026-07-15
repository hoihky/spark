#pragma once

#include "spark/scripting/SparkInterop.h"

#include <string>
#include <string_view>

namespace Spark {

/** Loads CoreCLR via nethost + hostfxr and invokes managed Spark.Scripting bootstrap. */
class CoreClrHost {
public:
    struct Options {
        std::string runtimeConfigPath;
        std::string assemblyPath;
        std::string typeName = "HelloCsGame.GameEntry, HelloCsGame";
        std::string methodName = "Initialize";
    };

    explicit CoreClrHost(Options options);
    ~CoreClrHost();

    CoreClrHost(const CoreClrHost&) = delete;
    CoreClrHost& operator=(const CoreClrHost&) = delete;

    /** Loads runtime + assembly; calls managed Initialize(hostApi). Returns false on failure. */
    [[nodiscard]] bool LoadAndInitialize(const SparkHostApi& hostApi, std::string& outError);

    [[nodiscard]] std::string_view GetLastError() const noexcept { return lastError_; }

private:
    Options options_;
    std::string lastError_;
    void* hostfxrHandle_ = nullptr;
    void* loadAssemblyFn_ = nullptr;
};

}  // namespace Spark
