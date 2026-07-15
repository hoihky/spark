#include "spark/audio/ISoundOutput.hpp"

#include "spark/core/Array.hpp"
#include "spark/core/Utility.hpp"
#include "spark/memory/UniquePtr.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <audioclient.h>
#include <combaseapi.h>
#include <ksmedia.h>
#include <mmdeviceapi.h>
#include <objbase.h>
#include <windows.h>

#include <cstring>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")

namespace Spark {

namespace {

constexpr std::size_t kRingCapFloats = 131072;

class Win32WasapiSoundOutput final : public ISoundOutput {
public:
    Win32WasapiSoundOutput() noexcept { InitializeCriticalSection(&cs); }

    ~Win32WasapiSoundOutput() override {
        Stop();
        DeleteCriticalSection(&cs);
    }

    bool Start(std::uint32_t sampleRate, std::uint32_t channels) override {
        Stop();
        if (channels != 2U) {
            return false;
        }
        (void)sampleRate;

        const HRESULT coinit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(coinit) && coinit != RPC_E_CHANGED_MODE) {
            return false;
        }
        comDidInit = (coinit == S_OK);

        IMMDeviceEnumerator* enumerator = nullptr;
        IMMDevice* device = nullptr;
        WAVEFORMATEX* mix = nullptr;

        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                reinterpret_cast<void**>(&enumerator));
        if (FAILED(hr) || enumerator == nullptr) {
            TeardownCom();
            return false;
        }

        hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
        enumerator->Release();
        enumerator = nullptr;
        if (FAILED(hr) || device == nullptr) {
            TeardownCom();
            return false;
        }

        hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&client));
        device->Release();
        device = nullptr;
        if (FAILED(hr) || client == nullptr) {
            TeardownCom();
            return false;
        }

        hr = client->GetMixFormat(&mix);
        if (FAILED(hr) || mix == nullptr) {
            AbortStartupNoDeviceStart();
            return false;
        }

        if (!IsAcceptableMixFormat(mix)) {
            CoTaskMemFree(mix);
            AbortStartupNoDeviceStart();
            return false;
        }

        /** ~20 ms @ 100-ns units; matches engine frame cadence reasonably. */
        constexpr REFERENCE_TIME kBufferDuration100Ns = 200000;
        hr = client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, kBufferDuration100Ns, 0, mix,
                nullptr);
        CoTaskMemFree(mix);
        mix = nullptr;
        if (FAILED(hr)) {
            AbortStartupNoDeviceStart();
            return false;
        }

        hr = client->GetBufferSize(&bufferFrames);
        if (FAILED(hr) || bufferFrames == 0U) {
            AbortStartupNoDeviceStart();
            return false;
        }

        event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (event == nullptr) {
            AbortStartupNoDeviceStart();
            return false;
        }

        hr = client->SetEventHandle(event);
        if (FAILED(hr)) {
            AbortStartupNoDeviceStart();
            return false;
        }

        hr = client->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void**>(&render));
        if (FAILED(hr) || render == nullptr) {
            AbortStartupNoDeviceStart();
            return false;
        }

        ring.Clear();
        ring.Resize(kRingCapFloats);
        for (std::size_t i = 0; i < kRingCapFloats; ++i) {
            ring[i] = 0.0F;
        }
        capMask = kRingCapFloats - 1U;
        head = 0;
        size = 0;

        {
            BYTE* prime = nullptr;
            const HRESULT pr = render->GetBuffer(bufferFrames, &prime);
            if (FAILED(pr) || prime == nullptr) {
                AbortStartupNoDeviceStart();
                return false;
            }
            std::memset(prime, 0, static_cast<std::size_t>(bufferFrames) * sizeof(float) * 2U);
            (void)render->ReleaseBuffer(bufferFrames, AUDCLNT_BUFFERFLAGS_SILENCE);
        }

        hr = client->Start();
        if (FAILED(hr)) {
            AbortStartupNoDeviceStart();
            return false;
        }

        stopThread = false;
        thread = CreateThread(nullptr, 0, RenderThreadThunk, this, 0, nullptr);
        if (thread == nullptr) {
            Stop();
            return false;
        }
        return true;
    }

    void Stop() noexcept override {
        if (thread != nullptr) {
            stopThread = true;
            if (event != nullptr) {
                SetEvent(event);
            }
            WaitForSingleObject(thread, INFINITE);
            CloseHandle(thread);
            thread = nullptr;
        }

        if (client != nullptr) {
            client->Stop();
        }

        if (render != nullptr) {
            render->Release();
            render = nullptr;
        }
        if (client != nullptr) {
            client->Release();
            client = nullptr;
        }
        if (event != nullptr) {
            CloseHandle(event);
            event = nullptr;
        }

        EnterCriticalSection(&cs);
        size = 0;
        head = 0;
        LeaveCriticalSection(&cs);

        TeardownCom();
    }

    void SubmitInterleavedFloat(const float* samples, const std::size_t frameCount) noexcept override {
        if (samples == nullptr || frameCount == 0) {
            return;
        }
        const std::size_t n = frameCount * 2U;
        EnterCriticalSection(&cs);
        while (size + n > kRingCapFloats) {
            head = (head + 2U) & capMask;
            size -= 2U;
        }
        for (std::size_t i = 0; i < n; ++i) {
            ring[(head + size + i) & capMask] = samples[i];
        }
        size += n;
        LeaveCriticalSection(&cs);
    }

private:
    static DWORD WINAPI RenderThreadThunk(void* param) noexcept {
        auto* self = static_cast<Win32WasapiSoundOutput*>(param);
        self->RenderLoop();
        return 0;
    }

    void RenderLoop() noexcept {
        for (;;) {
            if (stopThread) {
                return;
            }
            const DWORD wr = WaitForSingleObject(event, 3000U);
            if (stopThread) {
                return;
            }
            if (wr != WAIT_OBJECT_0) {
                continue;
            }
            if (client == nullptr || render == nullptr) {
                return;
            }

            UINT32 padding = 0;
            if (FAILED(client->GetCurrentPadding(&padding))) {
                continue;
            }
            if (padding >= bufferFrames) {
                continue;
            }
            const UINT32 available = bufferFrames - padding;
            if (available == 0U) {
                continue;
            }

            BYTE* dest = nullptr;
            const HRESULT gh = render->GetBuffer(available, &dest);
            if (FAILED(gh) || dest == nullptr) {
                continue;
            }
            auto* const dst = reinterpret_cast<float*>(dest);
            const std::size_t wantFloats = static_cast<std::size_t>(available) * 2U;

            EnterCriticalSection(&cs);
            const std::size_t take = wantFloats < size ? wantFloats : size;
            for (std::size_t i = 0; i < take; ++i) {
                dst[i] = ring[(head + i) & capMask];
            }
            head = (head + take) & capMask;
            size -= take;
            LeaveCriticalSection(&cs);

            for (std::size_t i = take; i < wantFloats; ++i) {
                dst[i] = 0.0F;
            }

            (void)render->ReleaseBuffer(available, 0);
        }
    }

    [[nodiscard]] static bool IsAcceptableMixFormat(const WAVEFORMATEX* wfx) noexcept {
        if (wfx->nChannels != 2 || wfx->wBitsPerSample != 32) {
            return false;
        }
        if (static_cast<std::uint32_t>(wfx->nSamplesPerSec) != 48000U) {
            /** Engine + mixer run at 48 kHz; resampling is not implemented here yet. */
            return false;
        }
        if (wfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
            return true;
        }
        if (wfx->wFormatTag != WAVE_FORMAT_EXTENSIBLE) {
            return false;
        }
        const auto* const ex = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(wfx);
        return IsEqualGUID(ex->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) != 0;
    }

    /** Releases COM graph created during <c>Start</c> before the render thread exists (no <c>IAudioClient::Stop</c>). */
    void AbortStartupNoDeviceStart() noexcept {
        if (render != nullptr) {
            render->Release();
            render = nullptr;
        }
        if (client != nullptr) {
            client->Release();
            client = nullptr;
        }
        if (event != nullptr) {
            CloseHandle(event);
            event = nullptr;
        }
        bufferFrames = 0;
        TeardownCom();
    }

    void TeardownCom() noexcept {
        if (comDidInit) {
            CoUninitialize();
            comDidInit = false;
        }
    }

    CRITICAL_SECTION cs{};
    Array<float> ring{};
    std::size_t capMask = 0;
    std::size_t head = 0;
    std::size_t size = 0;

    IAudioClient* client = nullptr;
    IAudioRenderClient* render = nullptr;
    HANDLE event = nullptr;
    HANDLE thread = nullptr;
    UINT32 bufferFrames = 0;
    volatile bool stopThread = false;
    bool comDidInit = false;
};

}  // namespace

UniquePtr<ISoundOutput> CreatePlatformSoundOutput() {
    return UniquePtr<ISoundOutput>(MakeUnique<Win32WasapiSoundOutput>().Release());
}

}  // namespace Spark
