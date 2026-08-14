#nullable enable
namespace Spark.Bindings;

/// <summary>Opaque native <c>IRenderFrame</c> handle from the engine loop.</summary>
public interface IRenderFrame
{
    nint Handle { get; }
}

public sealed class NativeRenderFrame : IRenderFrame
{
    public nint Handle { get; }
    public NativeRenderFrame(nint handle) => Handle = handle;
}
