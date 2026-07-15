using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Spark.Scripting;

namespace HelloCsGame;

/// <summary>Native host entry — loads this assembly and calls <see cref="Initialize"/>.</summary>
public static class GameEntry
{
    [ModuleInitializer]
    internal static void RegisterGame() => GameBootstrap.Factory = static () => new HelloGame();

    [UnmanagedCallersOnly]
    public static int Initialize(IntPtr hostApiPtr) => ScriptHostEntry.InitializeCore(hostApiPtr);
}
