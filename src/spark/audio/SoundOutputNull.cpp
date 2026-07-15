#include "spark/audio/ISoundOutput.hpp"

#include "spark/memory/UniquePtr.hpp"

namespace Spark {

namespace {

class NullSoundOutput final : public ISoundOutput {
public:
    bool Start(std::uint32_t /*sampleRate*/, std::uint32_t /*channels*/) override { return true; }

    void Stop() noexcept override {}

    void SubmitInterleavedFloat(const float* /*samples*/, std::size_t /*frameCount*/) noexcept override {}
};

}  // namespace

UniquePtr<ISoundOutput> CreatePlatformSoundOutput() {
    return UniquePtr<ISoundOutput>(MakeUnique<NullSoundOutput>().Release());
}

}  // namespace Spark
