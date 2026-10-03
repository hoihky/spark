#nullable enable

using Spark.Bindings;
using Spark.Bindings.Components;

namespace Spark.Scripting;

/// <summary>
/// Base class for C# games hosted by <c>spark_script_host_run</c>. Default update runs world simulation hooks.
/// </summary>
public abstract class SparkGame : Game
{
    protected GameWorld? World { get; private set; }

    public override void OnAttach(IEngineContext context)
    {
        World = context.TryGetScene()?.GetWorld();
    }

    /// <summary>Queues a bundled WAV/OGG/MP3 one-shot on the object's <see cref="SoundCueComponent"/>.</summary>
    protected static void PlayBundledSound(GameObject owner, string assetPath, float volume = 1f)
    {
        var cue = owner.GetOrAddSoundCue();
        cue?.QueueBundledClip(assetPath, volume);
    }

    /// <summary>Procedural SFX preset (no asset file).</summary>
    protected static void PlayPresetSound(GameObject owner, ProceduralSoundPreset preset, float volume = 1f)
    {
        var cue = owner.GetOrAddSoundCue();
        cue?.QueuePreset(preset, volume);
    }
}
