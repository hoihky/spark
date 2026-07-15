#pragma once

#include "spark/engine/Game.hpp"
#include "spark/scripting/SparkInterop.h"

namespace Spark {

/** Forwards Game lifecycle to managed callbacks registered via SparkHostApi. */
class ManagedGameBridge final : public Game {
public:
    void SetCallbacks(SparkManagedGameCallbacks callbacks);

    void OnAttach(IEngineContext& context) override;
    void OnDetach() override;
    void OnUpdate(const FrameTiming& timing, IEngineContext& context) override;
    void OnRender(IRenderFrame& frame, IEngineContext& context) override;

private:
    SparkManagedGameCallbacks callbacks_{};
};

}  // namespace Spark
