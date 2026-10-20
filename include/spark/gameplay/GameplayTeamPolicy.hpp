#pragma once

namespace Spark {

class GameObject;
class GameplayTeamRelation;

/** Friendly-fire and hostility rules for combat and abilities. */
class GameplayTeamPolicy final {
public:
    void SetAllowFriendlyFire(const bool value) noexcept { allowFriendlyFire = value; }
    [[nodiscard]] bool GetAllowFriendlyFire() const noexcept { return allowFriendlyFire; }

    [[nodiscard]] GameplayTeamRelation Classify(
            const GameObject* source,
            const GameObject& target) const noexcept;

    [[nodiscard]] bool AllowsHostileAction(const GameObject* source, const GameObject& target) const noexcept;

private:
    bool allowFriendlyFire = false;
};

}  // namespace Spark
