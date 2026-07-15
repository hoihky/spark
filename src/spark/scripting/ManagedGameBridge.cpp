#include "spark/scripting/ManagedGameBridge.hpp"

namespace Spark {

void ManagedGameBridge::SetCallbacks(SparkManagedGameCallbacks callbacks) {
    callbacks_ = callbacks;
}

void ManagedGameBridge::OnAttach(IEngineContext& context) {
    if (callbacks_.onAttach != nullptr) {
        callbacks_.onAttach(
                callbacks_.userData,
                reinterpret_cast<SparkEngineContext*>(&context));
    }
}

void ManagedGameBridge::OnDetach() {
    if (callbacks_.onDetach != nullptr) {
        callbacks_.onDetach(callbacks_.userData);
    }
}

void ManagedGameBridge::OnUpdate(const FrameTiming& timing, IEngineContext& context) {
    if (callbacks_.onUpdate != nullptr) {
        const SparkFrameTiming nativeTiming{
                .deltaTimeSeconds = timing.deltaTimeSeconds,
                .totalTimeSeconds = timing.totalTimeSeconds,
                .frameIndex = timing.frameIndex,
        };
        callbacks_.onUpdate(
                callbacks_.userData,
                &nativeTiming,
                reinterpret_cast<SparkEngineContext*>(&context));
    }
}

void ManagedGameBridge::OnRender(IRenderFrame& frame, IEngineContext& context) {
    if (callbacks_.onRender != nullptr) {
        callbacks_.onRender(
                callbacks_.userData,
                reinterpret_cast<SparkRenderFrame*>(&frame),
                reinterpret_cast<SparkEngineContext*>(&context));
    }
}

}  // namespace Spark
