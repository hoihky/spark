#include "spark/scripting/ManagedGameBridge.hpp"

namespace Spark {

void ManagedGameBridge::SetCallbacks(SparkManagedGameCallbacks value) {
    callbacks = value;
}

void ManagedGameBridge::OnAttach(IEngineContext& context) {
    if (callbacks.onAttach != nullptr) {
        callbacks.onAttach(
                callbacks.userData,
                reinterpret_cast<SparkEngineContext*>(&context));
    }
}

void ManagedGameBridge::OnDetach() {
    if (callbacks.onDetach != nullptr) {
        callbacks.onDetach(callbacks.userData);
    }
}

void ManagedGameBridge::OnUpdate(const FrameTiming& timing, IEngineContext& context) {
    if (callbacks.onUpdate != nullptr) {
        const SparkFrameTiming nativeTiming{
                .deltaTimeSeconds = timing.deltaTimeSeconds,
                .totalTimeSeconds = timing.totalTimeSeconds,
                .frameIndex = timing.frameIndex,
        };
        callbacks.onUpdate(
                callbacks.userData,
                &nativeTiming,
                reinterpret_cast<SparkEngineContext*>(&context));
    }
}

void ManagedGameBridge::OnRender(IRenderFrame& frame, IEngineContext& context) {
    if (callbacks.onRender != nullptr) {
        callbacks.onRender(
                callbacks.userData,
                reinterpret_cast<SparkRenderFrame*>(&frame),
                reinterpret_cast<SparkEngineContext*>(&context));
    }
}

}  // namespace Spark
