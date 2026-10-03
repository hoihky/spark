using Spark.Bindings;

namespace Spark.Scripting;

/// <summary>Placeholder when no game assembly overrides <see cref="GameBootstrap"/>.</summary>
public sealed class DefaultScriptGame : Game
{
    public override void OnAttach(IEngineContext context)
    {
        base.OnAttach(context);
        var scene = context.TryGetScene();
        if (scene is null)
        {
            return;
        }

        var world = scene.GetWorld();
        var player = world.CreateGameObject("Player");
        _ = player.GetId();
    }
}
