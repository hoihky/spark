#pragma once

#include "spark/engine/FrameTiming.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/IRenderFrame.hpp"

namespace Spark {

/**
 * Game contract — implement or inherit Game for application logic (Dependency Inversion).
 */
class IGame {
public:
    virtual ~IGame() = default;

    /** Called once after the engine has created window, renderer, and input. */
    virtual void OnAttach(IEngineContext& context) = 0;

    /** Called once when the loop exits before subsystems shut down. */
    virtual void OnDetach() = 0;

    /** Fixed-order: input was polled; use context for keys. Simulation / gameplay. */
    virtual void OnUpdate(const FrameTiming& timing, IEngineContext& context) = 0;

    /** Called after OnUpdate, before the frame is presented. */
    virtual void OnRender(IRenderFrame& frame, IEngineContext& context) = 0;
};

}  // namespace Spark
