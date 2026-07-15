using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Spark.Bindings;

namespace Spark.Scripting;

/// <summary>
/// CoreCLR entry invoked by native <c>SparkScriptHost</c> via hostfxr (nethost + hostfxr).
/// </summary>
public static class ScriptHostEntry
{
    private static Game? s_game;

    [UnmanagedCallersOnly]
    public static int Initialize(IntPtr hostApiPtr) => InitializeCore(hostApiPtr);

    public static int InitializeCore(IntPtr hostApiPtr)
    {
        if (hostApiPtr == IntPtr.Zero)
        {
            return -1;
        }

        var hostApi = Marshal.PtrToStructure<SparkHostApi>(hostApiPtr);
        if (hostApi.structSize < (uint)Marshal.SizeOf<SparkHostApi>())
        {
            return -2;
        }

        s_game = GameBootstrap.CreateGame();
        if (s_game is null)
        {
            return -3;
        }

        SparkManagedGameCallbacks callbacks;
        unsafe
        {
            callbacks = new SparkManagedGameCallbacks
            {
                userData = IntPtr.Zero,
                onAttach = (IntPtr)(delegate* unmanaged[Cdecl]<IntPtr, IntPtr, void>)&OnAttach,
                onDetach = (IntPtr)(delegate* unmanaged[Cdecl]<IntPtr, void>)&OnDetach,
                onUpdate = (IntPtr)(delegate* unmanaged[Cdecl]<IntPtr, IntPtr, IntPtr, void>)&OnUpdate,
                onRender = (IntPtr)(delegate* unmanaged[Cdecl]<IntPtr, IntPtr, IntPtr, void>)&OnRender,
            };
        }

        var register = Marshal.GetDelegateForFunctionPointer<RegisterManagedGameDelegate>(hostApi.registerManagedGame);
        return register(ref callbacks);
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static void OnAttach(IntPtr userData, IntPtr contextPtr)
    {
        _ = userData;
        s_game?.OnAttach(new IEngineContext { Handle = contextPtr });
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static void OnDetach(IntPtr userData)
    {
        _ = userData;
        s_game?.OnDetach();
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static void OnUpdate(IntPtr userData, IntPtr timingPtr, IntPtr contextPtr)
    {
        _ = userData;
        if (s_game is null || timingPtr == IntPtr.Zero)
        {
            return;
        }

        var native = Marshal.PtrToStructure<SparkFrameTiming>(timingPtr);
        var timing = FrameTiming.FromNative(native);
        s_game.OnUpdate(timing, new IEngineContext { Handle = contextPtr });
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static void OnRender(IntPtr userData, IntPtr framePtr, IntPtr contextPtr)
    {
        _ = userData;
        IRenderFrame frame = framePtr != IntPtr.Zero
            ? new NativeRenderFrame(framePtr)
            : EmptyRenderFrame.Instance;
        s_game?.OnRender(frame, new IEngineContext { Handle = contextPtr });
    }

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate int RegisterManagedGameDelegate(ref SparkManagedGameCallbacks callbacks);

    [StructLayout(LayoutKind.Sequential)]
    private struct SparkHostApi
    {
        public uint structSize;
        public IntPtr registerManagedGame;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct SparkManagedGameCallbacks
    {
        public IntPtr userData;
        public IntPtr onAttach;
        public IntPtr onDetach;
        public IntPtr onUpdate;
        public IntPtr onRender;
    }

    private sealed class EmptyRenderFrame : IRenderFrame
    {
        public static readonly EmptyRenderFrame Instance = new();
        public nint Handle => IntPtr.Zero;
    }
}
