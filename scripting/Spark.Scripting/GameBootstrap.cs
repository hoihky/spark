using Spark.Bindings;

namespace Spark.Scripting;

/// <summary>
/// Override in your game assembly to return the entry <see cref="Game"/> type.
/// Default: <see cref="DefaultScriptGame"/>.
/// </summary>
public static class GameBootstrap
{
    public static Func<Game>? Factory { get; set; }

    public static Game? CreateGame() => Factory?.Invoke() ?? new DefaultScriptGame();
}
