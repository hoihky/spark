#include "spark/demo/NewShellDemoGame.hpp"
#include "spark/config.hpp"
#include "spark/engine/Engine.hpp"
#include "spark/engine/EngineRunOptions.hpp"
#include "spark/media/VideoRecordingSettings.hpp"

#include <cstring>
#include <ctime>
#include <exception>
#include <iostream>
#include <print>

#include <chrono>
#include <cstdio>

namespace {

bool ParseRecordPreset(const char* text, Spark::VideoRecordingPreset& out) {
    if (text == nullptr) {
        return false;
    }
    if (std::strcmp(text, "native") == 0) {
        out = Spark::VideoRecordingPreset::Native;
        return true;
    }
    if (std::strcmp(text, "720") == 0 || std::strcmp(text, "hd720") == 0) {
        out = Spark::VideoRecordingPreset::Hd720;
        return true;
    }
    if (std::strcmp(text, "1080") == 0 || std::strcmp(text, "hd1080") == 0) {
        out = Spark::VideoRecordingPreset::Hd1080;
        return true;
    }
    return false;
}

void PrintUsage(const char* argv0) {
    std::println(
            std::cerr,
            "Usage: {} [--record [output.mp4]] [--record-fps N] [--record-preset native|720|1080] "
            "[--record-video-bitrate BPS] [--record-audio-bitrate BPS] [--no-record-watermark]",
            argv0 != nullptr ? argv0 : "SparkDemo");
    std::println(std::cerr, "  F3 or F4 toggles FPS overlay.");
    std::println(std::cerr, "  F9 toggles MP4 recording (H.264 + AAC) while the demo runs.");
    std::println(std::cerr, "  F12 saves a PNG screenshot.");
}

}  // namespace

int main(int argc, char** argv) {
    try {
        Spark::EngineRunOptions options{};
        for (int i = 1; i < argc; ++i) {
            const char* arg = argv[i];
            if (std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-h") == 0) {
                PrintUsage(argv[0]);
                return 0;
            }
            if (std::strcmp(arg, "--record") == 0) {
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    options.autoRecordPath = Spark::Utf8String(argv[++i]);
                } else {
                    options.autoRecordPath = Spark::Utf8String("auto");
                }
                continue;
            }
            if (std::strcmp(arg, "--record-fps") == 0 && i + 1 < argc) {
                options.recordFps = static_cast<std::uint32_t>(std::atoi(argv[++i]));
                continue;
            }
            if (std::strcmp(arg, "--record-preset") == 0 && i + 1 < argc) {
                Spark::VideoRecordingPreset preset = Spark::VideoRecordingPreset::Native;
                if (!ParseRecordPreset(argv[++i], preset)) {
                    std::println(std::cerr, "Spark: unknown --record-preset value");
                    return 1;
                }
                options.recordPreset = preset;
                continue;
            }
            if (std::strcmp(arg, "--record-video-bitrate") == 0 && i + 1 < argc) {
                options.videoBitrate = static_cast<std::uint32_t>(std::atoi(argv[++i]));
                continue;
            }
            if (std::strcmp(arg, "--record-audio-bitrate") == 0 && i + 1 < argc) {
                options.audioBitrate = static_cast<std::uint32_t>(std::atoi(argv[++i]));
                continue;
            }
            if (std::strcmp(arg, "--no-record-watermark") == 0) {
                options.recordWatermark = false;
                continue;
            }
            std::println(std::cerr, "Spark: unknown argument: {}", arg);
            PrintUsage(argv[0]);
            return 1;
        }

        if (options.autoRecordPath == Spark::Utf8String("auto")) {
            char path[512]{};
            const auto now = std::chrono::system_clock::now();
            const std::time_t nowSec = std::chrono::system_clock::to_time_t(now);
            std::tm localTm{};
            localtime_r(&nowSec, &localTm);
            std::snprintf(
                    path,
                    sizeof(path),
                    "%s/recordings/spark_%04d%02d%02d_%02d%02d%02d.mp4",
                    SPARK_BUILD_ASSETS_DIR,
                    localTm.tm_year + 1900,
                    localTm.tm_mon + 1,
                    localTm.tm_mday,
                    localTm.tm_hour,
                    localTm.tm_min,
                    localTm.tm_sec);
            options.autoRecordPath = Spark::Utf8String(path);
        }

        Spark::Engine engine(Spark::NewShellDemoGame());
        engine.Run(options);
    } catch (const std::exception& e) {
        std::println(std::cerr, "Spark: {}", e.what());
        return 1;
    }
    return 0;
}
