#nullable enable

using Spark.Bindings;
using Spark.Bindings.Components;
using Spark.Scripting;

namespace HelloCsGame;

/// <summary>Minimal gameplay sample: transform + sound cue + optional game state (compile-checked against Spark.Bindings).</summary>
public sealed class HelloGame : SparkGame
{
    private GameObject? _player;

    public override void OnAttach(IEngineContext context)
    {
        base.OnAttach(context);
        var world = context.TryGetScene()?.GetWorld();
        if (world is null)
        {
            return;
        }

        _player = world.CreateGameObject("CsPlayer");
        _player.GetOrAddTransform().Translation = new Vector3 { X = 0f, Y = 2f, Z = 0f };
        _player.AddSoundCue();
        var state = _player.GetOrAddGameState();
        state?.PushState(SparkGameFlowState.SparkGameFlowState_Playing);
    }

    public override void OnUpdate(FrameTiming timing, IEngineContext context)
    {
        base.OnUpdate(timing, context);
        if (_player is null)
        {
            return;
        }

        var input = context.GetInput();
        if (input.IsKeyPressedThisFrame(32)) // Space
        {
            PlayPresetSound(_player, ProceduralSoundPreset.Jump);
        }
    }
}
