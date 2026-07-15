#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>

#include "spark/media/VideoRecorder.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/core/Utf8String.hpp"

#include <cstdio>
#include <cstring>

namespace Spark {

namespace {

constexpr double kAudioSampleRate = 48000.0;
constexpr int kVideoTimescale = 600;

CMTime VideoTimeFromSeconds(const double seconds) {
    return CMTimeMakeWithSeconds(seconds, kVideoTimescale);
}

class MacVideoRecorder final : public VideoRecorder {
public:
    MacVideoRecorder() = default;

    ~MacVideoRecorder() override { (void)End(); }

    bool Begin(
            const VideoRecordingSettings& settings,
            const std::uint32_t captureWidth,
            const std::uint32_t captureHeight) noexcept override {
        (void)End();
        if (settings.outputPath.IsEmpty() || captureWidth == 0 || captureHeight == 0) {
            return false;
        }

        std::uint32_t outW = VideoRecordingSettings::PresetWidth(settings.preset);
        std::uint32_t outH = VideoRecordingSettings::PresetHeight(settings.preset);
        if (outW == 0 || outH == 0) {
            outW = captureWidth;
            outH = captureHeight;
        }

        NSString* path = [NSString stringWithUTF8String:settings.outputPath.CStr()];
        if (path == nil) {
            return false;
        }
        [[NSFileManager defaultManager] removeItemAtPath:path error:nil];

        NSError* error = nil;
        writer_ = [[AVAssetWriter alloc] initWithURL:[NSURL fileURLWithPath:path]
                                            fileType:AVFileTypeMPEG4
                                               error:&error];
        if (writer_ == nil || error != nil) {
            std::fprintf(stderr, "Spark: video recorder failed to create writer\n");
            writer_ = nil;
            return false;
        }
        writer_.shouldOptimizeForNetworkUse = YES;

        NSDictionary* videoSettings = @{
            AVVideoCodecKey : AVVideoCodecTypeH264,
            AVVideoWidthKey : @(static_cast<int>(outW)),
            AVVideoHeightKey : @(static_cast<int>(outH)),
            AVVideoCompressionPropertiesKey : @{
                AVVideoAverageBitRateKey : @(static_cast<int>(settings.videoBitrate)),
                AVVideoExpectedSourceFrameRateKey : @(static_cast<int>(settings.fps > 0 ? settings.fps : 60)),
                AVVideoProfileLevelKey : AVVideoProfileLevelH264HighAutoLevel,
            },
        };

        videoInput_ = [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeVideo outputSettings:videoSettings];
        if (videoInput_ == nil) {
            writer_ = nil;
            return false;
        }
        videoInput_.expectsMediaDataInRealTime = YES;

        NSDictionary* pixelAttrs = @{
            (NSString*)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA),
            (NSString*)kCVPixelBufferWidthKey : @(static_cast<int>(outW)),
            (NSString*)kCVPixelBufferHeightKey : @(static_cast<int>(outH)),
            (NSString*)kCVPixelBufferIOSurfacePropertiesKey : @{},
        };
        pixelAdaptor_ = [AVAssetWriterInputPixelBufferAdaptor assetWriterInputPixelBufferAdaptorWithAssetWriterInput:videoInput_
                                                                                        sourcePixelBufferAttributes:pixelAttrs];
        if (pixelAdaptor_ == nil) {
            writer_ = nil;
            videoInput_ = nil;
            return false;
        }

        NSDictionary* audioSettings = @{
            AVFormatIDKey : @(kAudioFormatMPEG4AAC),
            AVSampleRateKey : @(kAudioSampleRate),
            AVNumberOfChannelsKey : @2,
            AVEncoderBitRateKey : @(static_cast<int>(settings.audioBitrate)),
        };
        audioInput_ = [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeAudio outputSettings:audioSettings];
        if (audioInput_ == nil) {
            writer_ = nil;
            videoInput_ = nil;
            pixelAdaptor_ = nil;
            return false;
        }
        audioInput_.expectsMediaDataInRealTime = YES;

        if (![writer_ canAddInput:videoInput_] || ![writer_ canAddInput:audioInput_]) {
            writer_ = nil;
            videoInput_ = nil;
            audioInput_ = nil;
            pixelAdaptor_ = nil;
            return false;
        }
        [writer_ addInput:videoInput_];
        [writer_ addInput:audioInput_];

        AudioStreamBasicDescription asbd{};
        asbd.mSampleRate = kAudioSampleRate;
        asbd.mFormatID = kAudioFormatLinearPCM;
        asbd.mFormatFlags = static_cast<UInt32>(kAudioFormatFlagIsFloat) | static_cast<UInt32>(kAudioFormatFlagsNativeEndian) |
                static_cast<UInt32>(kAudioFormatFlagIsPacked);
        asbd.mBytesPerPacket = 8;
        asbd.mFramesPerPacket = 1;
        asbd.mBytesPerFrame = 8;
        asbd.mChannelsPerFrame = 2;
        asbd.mBitsPerChannel = 32;

        CMAudioFormatDescriptionRef audioDesc = nullptr;
        if (CMAudioFormatDescriptionCreate(
                    kCFAllocatorDefault,
                    &asbd,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    nullptr,
                    &audioDesc) != noErr ||
            audioDesc == nullptr) {
            writer_ = nil;
            videoInput_ = nil;
            audioInput_ = nil;
            pixelAdaptor_ = nil;
            return false;
        }
        audioFormat_ = audioDesc;

        if (![writer_ startWriting]) {
            std::fprintf(stderr, "Spark: AVAssetWriter startWriting failed: %s\n",
                    writer_.error != nil ? writer_.error.localizedDescription.UTF8String : "unknown");
            writer_ = nil;
            videoInput_ = nil;
            audioInput_ = nil;
            pixelAdaptor_ = nil;
            audioFormat_ = nullptr;
            return false;
        }
        [writer_ startSessionAtSourceTime:kCMTimeZero];

        outputWidth_ = outW;
        outputHeight_ = outH;
        audioSamplesWritten_ = 0;
        sessionStarted_ = YES;
        active_ = YES;
        outputPath_ = settings.outputPath;
        std::fprintf(stderr, "Spark: recording %ux%u to %s\n", outW, outH, settings.outputPath.CStr());
        return true;
    }

    void AppendVideoFrameBgra(
            const std::uint8_t* bgra,
            const std::uint32_t width,
            const std::uint32_t height,
            const double ptsSeconds) noexcept override {
        if (!active_ || bgra == nullptr || width == 0 || height == 0 || pixelAdaptor_ == nil || videoInput_ == nil) {
            return;
        }
        if (!videoInput_.readyForMoreMediaData) {
            return;
        }

        CVPixelBufferRef pixelBuffer = nullptr;
        const NSDictionary* attrs = @{
            (NSString*)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA),
            (NSString*)kCVPixelBufferWidthKey : @(static_cast<int>(outputWidth_)),
            (NSString*)kCVPixelBufferHeightKey : @(static_cast<int>(outputHeight_)),
            (NSString*)kCVPixelBufferIOSurfacePropertiesKey : @{},
        };
        if (CVPixelBufferCreate(
                    kCFAllocatorDefault,
                    outputWidth_,
                    outputHeight_,
                    kCVPixelFormatType_32BGRA,
                    (__bridge CFDictionaryRef)attrs,
                    &pixelBuffer) != kCVReturnSuccess ||
            pixelBuffer == nullptr) {
            return;
        }

        CVPixelBufferLockBaseAddress(pixelBuffer, 0);
        std::uint8_t* dstBase = static_cast<std::uint8_t*>(CVPixelBufferGetBaseAddress(pixelBuffer));
        const std::size_t dstPitch = static_cast<std::size_t>(CVPixelBufferGetBytesPerRow(pixelBuffer));
        const std::size_t srcPitch = static_cast<std::size_t>(width) * 4U;

        if (width == outputWidth_ && height == outputHeight_) {
            for (std::uint32_t y = 0; y < height; ++y) {
                std::memcpy(dstBase + y * dstPitch, bgra + y * srcPitch, srcPitch);
            }
        } else {
            for (std::uint32_t y = 0; y < outputHeight_; ++y) {
                const std::uint32_t sy = (y * height) / outputHeight_;
                const std::uint8_t* srcRow = bgra + static_cast<std::size_t>(sy) * srcPitch;
                std::uint8_t* dstRow = dstBase + y * dstPitch;
                for (std::uint32_t x = 0; x < outputWidth_; ++x) {
                    const std::uint32_t sx = (x * width) / outputWidth_;
                    const std::size_t si = static_cast<std::size_t>(sx) * 4U;
                    const std::size_t di = static_cast<std::size_t>(x) * 4U;
                    dstRow[di + 0] = srcRow[si + 0];
                    dstRow[di + 1] = srcRow[si + 1];
                    dstRow[di + 2] = srcRow[si + 2];
                    dstRow[di + 3] = srcRow[si + 3];
                }
            }
        }
        CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);

        const CMTime pts = VideoTimeFromSeconds(ptsSeconds);
        const BOOL ok = [pixelAdaptor_ appendPixelBuffer:pixelBuffer withPresentationTime:pts];
        CVPixelBufferRelease(pixelBuffer);
        if (!ok) {
            std::fprintf(stderr, "Spark: failed to append video frame at %.3f s\n", ptsSeconds);
        }
    }

    void AppendAudioInterleavedFloat(const float* samples, const std::size_t frameCount) noexcept override {
        if (!active_ || samples == nullptr || frameCount == 0 || audioInput_ == nil || audioFormat_ == nullptr) {
            return;
        }
        if (!audioInput_.readyForMoreMediaData) {
            return;
        }

        const std::size_t byteCount = frameCount * 2U * sizeof(float);
        CMBlockBufferRef block = nullptr;
        if (CMBlockBufferCreateWithMemoryBlock(
                    kCFAllocatorDefault,
                    nullptr,
                    byteCount,
                    kCFAllocatorDefault,
                    nullptr,
                    0,
                    byteCount,
                    0,
                    &block) != kCMBlockBufferNoErr ||
            block == nullptr) {
            return;
        }
        if (CMBlockBufferReplaceDataBytes(samples, block, 0, byteCount) != kCMBlockBufferNoErr) {
            CFRelease(block);
            return;
        }

        const CMTime pts = CMTimeMake(
                static_cast<int64_t>(audioSamplesWritten_),
                static_cast<int32_t>(kAudioSampleRate));
        CMSampleBufferRef sample = nullptr;
        const CMSampleTimingInfo timing = {
            .duration = CMTimeMake(static_cast<int64_t>(frameCount), static_cast<int32_t>(kAudioSampleRate)),
            .presentationTimeStamp = pts,
            .decodeTimeStamp = kCMTimeInvalid,
        };
        if (CMSampleBufferCreate(
                    kCFAllocatorDefault,
                    block,
                    true,
                    nullptr,
                    nullptr,
                    audioFormat_,
                    static_cast<CMItemCount>(frameCount),
                    1,
                    &timing,
                    0,
                    nullptr,
                    &sample) != noErr ||
            sample == nullptr) {
            CFRelease(block);
            return;
        }

        const BOOL ok = [audioInput_ appendSampleBuffer:sample];
        CFRelease(sample);
        CFRelease(block);
        if (!ok) {
            std::fprintf(stderr, "Spark: failed to append audio at sample %lld\n", static_cast<long long>(audioSamplesWritten_));
        } else {
            audioSamplesWritten_ += static_cast<int64_t>(frameCount);
        }
    }

    bool End() noexcept override {
        if (!active_) {
            return false;
        }
        active_ = false;

        [videoInput_ markAsFinished];
        [audioInput_ markAsFinished];

        __block BOOL finished = NO;
        __block BOOL success = NO;
        dispatch_semaphore_t sem = dispatch_semaphore_create(0);
        [writer_ finishWritingWithCompletionHandler:^{
            success = (writer_.status == AVAssetWriterStatusCompleted);
            finished = YES;
            dispatch_semaphore_signal(sem);
        }];
        dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);

        if (success) {
            std::fprintf(stderr, "Spark: recording saved to %s\n", outputPath_.CStr());
        } else {
            std::fprintf(stderr, "Spark: recording finalize failed: %s\n",
                    writer_.error != nil ? writer_.error.localizedDescription.UTF8String : "unknown");
        }

        writer_ = nil;
        videoInput_ = nil;
        audioInput_ = nil;
        pixelAdaptor_ = nil;
        if (audioFormat_ != nullptr) {
            CFRelease(audioFormat_);
            audioFormat_ = nullptr;
        }
        sessionStarted_ = NO;
        audioSamplesWritten_ = 0;
        outputWidth_ = 0;
        outputHeight_ = 0;
        return finished && success;
    }

    bool IsActive() const noexcept override { return active_; }

private:
    AVAssetWriter* writer_ = nil;
    AVAssetWriterInput* videoInput_ = nil;
    AVAssetWriterInput* audioInput_ = nil;
    AVAssetWriterInputPixelBufferAdaptor* pixelAdaptor_ = nil;
    CMAudioFormatDescriptionRef audioFormat_ = nullptr;
    int64_t audioSamplesWritten_ = 0;
    bool sessionStarted_ = NO;
    bool active_ = false;
    Utf8String outputPath_;
};

}  // namespace

UniquePtr<VideoRecorder> CreateMacVideoRecorder() {
    return UniquePtr<VideoRecorder>(new MacVideoRecorder());
}

}  // namespace Spark
