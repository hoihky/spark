#pragma once

namespace Spark {

class GameObject;
class GameplayTeamId;

/** Resolves team membership walking the object hierarchy. */
class GameplayTeamAffiliation final {
public:
    [[nodiscard]] static GameplayTeamId Resolve(const GameObject& object) noexcept;
};

}  // namespace Spark
