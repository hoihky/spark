#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/editor/EditorContext.hpp"

namespace Spark {

class GameObject;

namespace Editor {

/**
 * Facade for hierarchy mutations. Panels call these helpers; commands own undo semantics.
 */
class EditorHierarchyActions final {
public:
    [[nodiscard]] static bool CanEdit(const EditorContext& ctx) noexcept;

    static void CreateEmpty(EditorContext& ctx, GameObject* parent);
    static void DeleteObject(EditorContext& ctx, GameObject& target);
    static void DuplicateObject(EditorContext& ctx, GameObject& source);
    static void ReparentToRoot(EditorContext& ctx, GameObject& child);

    [[nodiscard]] static GameObject* ResolveContextTarget(
            const EditorContext& ctx,
            GameObject* contextObject) noexcept;

private:
    EditorHierarchyActions() = delete;
};

}  // namespace Editor
}  // namespace Spark
