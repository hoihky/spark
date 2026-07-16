#pragma once

namespace Spark {

class Scene;

/**
 * Optional capability for games that own an ECS scene (Interface Segregation).
 * Engine binds <c>TryGetScene()</c> when the running <c>IGame</c> implements this.
 */
class ISceneProvider {
public:
    virtual ~ISceneProvider() = default;

    [[nodiscard]] virtual Scene& GetScene() noexcept = 0;
    [[nodiscard]] virtual const Scene& GetScene() const noexcept = 0;
};

}  // namespace Spark
