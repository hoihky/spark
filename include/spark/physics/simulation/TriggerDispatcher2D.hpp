#pragma once

#include <cstdint>

namespace Spark {

class Collider2D;
class GameObject;

/** Emits <c>Physics2DTriggerOverlap</c> signals for static–dynamic and dynamic–dynamic overlaps. */
class TriggerDispatcher2D {
public:
    static void ReportStaticDynamic(
            GameObject& dynamic,
            const Collider2D& col,
            std::uint32_t staticIndex,
            bool dynamicColliderIsTrigger) noexcept;

    static void ReportDynamicDynamic(
            GameObject& a,
            GameObject& b,
            bool aTrigger,
            bool bTrigger) noexcept;
};

}  // namespace Spark
