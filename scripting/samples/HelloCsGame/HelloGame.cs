using Spark.Bindings;

namespace HelloCsGame;

/// <summary>
/// 2D platformer sample: Kenney textures (or procedural fallbacks), <see cref="Sprite2DCharacterAnimFsmComponent"/>,
/// gems + attack-arc collection via 2D physics queries, HUD, fall respawn + summit goal.
/// </summary>
public sealed class HelloGame : Game
{
    private const uint PlayerAtlasRows = 1;
    private const int KeyA = 65;
    private const int KeyD = 68;
    private const int KeyLeft = 263;
    private const int KeyRight = 262;
    private const int KeySpace = 32;
    private const int KeyJ = 74;
    private const int KeyK = 75;
    private const int MaxQueryHits = 32;

    private GameWorld? _world;
    private GameObject? _player;
    private TransformComponent? _playerTransform;
    private Rigidbody2DComponent? _playerBody;
    private SpriteAnimatorComponent? _playerAnim;
    private Sprite2DCharacterAnimFsmComponent? _playerFsm;
    private TextOverlayComponent? _hud;
    private readonly List<GameObject> _gems = new();
    private Camera2D _camera = Camera2D.DefaultPlatformer;
    private float _sceneTime;

    private Platformer2DAssetsInfo _assets;
    private float _playerBaseScaleX = Platformer2DLayout.PlayerHalfW * 2f;
    private float _playerBaseScaleY = Platformer2DLayout.PlayerHalfH * 2f;
    private bool _facingLeft;
    private bool _goalReached;
    private int _gemsCollected;
    private readonly PhysicsQueryHit2D[] _attackHitScratch = new PhysicsQueryHit2D[MaxQueryHits];

    private static readonly PhysicsQueryFilter2D WeaponQueryFilter = new()
    {
        QueryCategoryBits = CollisionFilter2D.LayerBit(2),
        QueryMaskBits = Platformer2DLayout.GemHurtboxCategoryBits,
        HitSolids = false,
        HitTriggers = true,
    };

    public override void OnAttach(IEngineContext context)
    {
        base.OnAttach(context);
        _world = context.TryGetScene()?.GetWorld();
        if (_world is null)
        {
            return;
        }

        _ = _world.MountPlatformerUiFont();
        _ = _world.RegisterPlatformer2DDemoTextures(out _assets);

        BuildPlatforms(_world);
        BuildGems(_world);
        _player = BuildPlayer(_world, out _playerTransform, out _playerBody, out _playerAnim, out _playerFsm);
        _hud = BuildHud(_world);

        var spawnY = Platformer2DLayout.GroundSurfaceY + Platformer2DLayout.PlayerHalfH;
        _camera.Position = new Vector3 { X = Platformer2DLayout.PlayerSpawnX, Y = spawnY + 1.2f, Z = 0f };
        _camera.HalfExtentY = 8.5f;

        context.GetInput().SetCursorCaptured(false);
    }

    public override void OnUpdate(FrameTiming timing, IEngineContext context)
    {
        if (_world is null || _player is null || _playerTransform is null || _playerBody is null)
        {
            base.OnUpdate(timing, context);
            return;
        }

        _sceneTime = timing.TotalTimeSeconds;

        var input = context.GetInput();
        var run = 0f;
        if (input.IsKeyDown(KeyA) || input.IsKeyDown(KeyLeft))
        {
            run -= 1f;
        }
        if (input.IsKeyDown(KeyD) || input.IsKeyDown(KeyRight))
        {
            run += 1f;
        }

        if (_playerFsm is not null)
        {
            if (input.IsKeyPressedThisFrame(KeyJ))
            {
                _playerFsm.RequestAttack();
            }
            if (input.IsKeyPressedThisFrame(KeyK))
            {
                _playerFsm.RequestHurt();
            }
        }

        if (System.Math.Abs(run) > 0.5f)
        {
            _facingLeft = run < 0f;
        }
        _playerTransform.Scale = new Vector3
        {
            X = _facingLeft ? -_playerBaseScaleX : _playerBaseScaleX,
            Y = _playerBaseScaleY,
            Z = 1f,
        };

        var velY = _playerBody.Velocity.Y;
        if (input.IsKeyPressedThisFrame(KeySpace) && _playerBody.IsGrounded)
        {
            velY = Platformer2DLayout.JumpSpeed;
        }
        _playerBody.Velocity = new Vector2 { X = run * Platformer2DLayout.RunSpeed, Y = velY };

        _world.PhysicsSimulate2D(
            timing,
            new PhysicsWorld2DSettings { GravityY = -32f, MaxFallSpeed = 46f, ResolveDynamicVsDynamic = true });

        var pos = _playerTransform.Translation;
        if (pos.Y < Platformer2DLayout.FallRespawnY)
        {
            var spawnY = Platformer2DLayout.GroundSurfaceY + Platformer2DLayout.PlayerHalfH;
            _playerTransform.Translation = new Vector3 { X = Platformer2DLayout.PlayerSpawnX, Y = spawnY, Z = pos.Z };
            _playerBody.Velocity = new Vector2 { X = 0f, Y = 0f };
            _goalReached = false;
        }

        if (!_goalReached && Platformer2DLayout.IsSummitGoal(pos.X, pos.Y))
        {
            _goalReached = true;
        }

        TryCollectGemsByProximity(pos);
        TryCollectGemsByAttackArc(pos);

        var follow = System.Math.Min(1f, 7.5f * timing.DeltaTimeSeconds);
        _camera.Position = new Vector3
        {
            X = _camera.Position.X + (pos.X - _camera.Position.X) * follow,
            Y = _camera.Position.Y + ((pos.Y + 1.48f) - _camera.Position.Y) * follow,
            Z = 0f,
        };

        if (_hud is not null)
        {
            var assetNote = _assets.UsingKenneyTilesheet && _assets.UsingKenneyPlayerAtlas
                ? "Kenney PNGs"
                : "procedural fallback";
            var goal = _goalReached ? " — Summit!" : "";
            _hud.SetText(
                $"C# platformer — gems {_gemsCollected}/{Platformer2DLayout.GemCount}{goal} | pos ({pos.X:F1},{pos.Y:F1}) | WASD · Space · J attack arc · K hurt");
        }

        base.OnUpdate(timing, context);
    }

    public override void OnRender(IRenderFrame frame, IEngineContext context)
    {
        _ = frame;
        if (_world is null)
        {
            return;
        }

        SceneSubmit.StandardLit(
            _world,
            context,
            _camera,
            lightDirectionWorld: Normalize(new Vector3 { X = 0.3f, Y = 0.86f, Z = 0.36f }),
            lightColor: new Vector3 { X = 1f, Y = 0.98f, Z = 0.95f },
            lightIntensity: 0.85f,
            ambientColor: new Vector3 { X = 0.16f, Y = 0.18f, Z = 0.24f },
            sceneTimeSeconds: _sceneTime,
            spriteSortMode: SparkSpriteSortMode.SparkSpriteSortMode_SortOrderThenWorldY);
    }

    private void TryCollectGemsByProximity(Vector3 playerPos)
    {
        if (_world is null)
        {
            return;
        }

        var radiusSq = Platformer2DLayout.GemCollectRadius * Platformer2DLayout.GemCollectRadius;
        for (var gi = _gems.Count - 1; gi >= 0; --gi)
        {
            var gem = _gems[gi];
            var gtr = gem.GetTransform();
            if (gtr is null)
            {
                continue;
            }

            var gpos = gtr.Translation;
            var dx = gpos.X - playerPos.X;
            var dy = gpos.Y - playerPos.Y;
            if (dx * dx + dy * dy <= radiusSq)
            {
                CollectGemAt(gi);
            }
        }
    }

    private void TryCollectGemsByAttackArc(Vector3 playerPos)
    {
        if (_world is null || _playerAnim is null)
        {
            return;
        }

        var attackActive = _playerAnim.ClipIndex == Platformer2DLayout.PlayerAttackClipIndex
            && !_playerAnim.IsCurrentClipFinished;
        if (!attackActive)
        {
            return;
        }

        var dirX = _facingLeft ? -1f : 1f;
        const float dirY = 0f;
        var originX = playerPos.X + dirX * (Platformer2DLayout.PlayerHalfW * 0.55f);
        var originY = playerPos.Y + Platformer2DLayout.PlayerHalfH * 0.12f;

        var hits = _attackHitScratch.AsSpan();
        var total = _world.QueryOverlapArcWorldStatics2D(
            originX,
            originY,
            Platformer2DLayout.AttackArcRadius,
            dirX,
            dirY,
            Platformer2DLayout.AttackArcHalfAngleRad,
            WeaponQueryFilter,
            hits);

        var count = System.Math.Min(total, hits.Length);
        for (var i = 0; i < count; ++i)
        {
            var owner = hits[i].Owner;
            if (owner is null)
            {
                continue;
            }

            for (var gi = _gems.Count - 1; gi >= 0; --gi)
            {
                if (_gems[gi].Handle == owner.Handle)
                {
                    CollectGemAt(gi);
                }
            }
        }
    }

    private void CollectGemAt(int index)
    {
        if (_world is null || index < 0 || index >= _gems.Count)
        {
            return;
        }

        _world.DestroyGameObject(_gems[index]);
        _gems.RemoveAt(index);
        ++_gemsCollected;
    }

    private static Vector3 Normalize(Vector3 v)
    {
        var len = System.MathF.Sqrt(v.X * v.X + v.Y * v.Y + v.Z * v.Z);
        if (len < 1e-5f)
        {
            return v;
        }
        return new Vector3 { X = v.X / len, Y = v.Y / len, Z = v.Z / len };
    }

    private void BuildPlatforms(GameWorld world)
    {
        for (var i = 0; i < Platformer2DLayout.PlatformCount; ++i)
        {
            var p = Platformer2DLayout.Platforms[i];
            AddPlatform(
                world,
                p[0],
                p[1],
                p[2],
                p[3],
                Platformer2DLayout.PlatformTileNumbers[i],
                40 + i);
        }
    }

    private void BuildGems(GameWorld world)
    {
        _gems.Clear();
        _gemsCollected = 0;

        for (var gi = 0; gi < Platformer2DLayout.GemCount; ++gi)
        {
            var spawn = Platformer2DLayout.GemSpawns[gi];
            var go = world.CreateGameObject("PlatGem");
            var tr = go.GetOrAddTransform();
            tr.Translation = new Vector3
            {
                X = spawn[0],
                Y = spawn[1],
                Z = 0.05f + 0.0003f * gi,
            };
            tr.Scale = new Vector3
            {
                X = Platformer2DLayout.GemDrawScale,
                Y = Platformer2DLayout.GemDrawScale,
                Z = 1f,
            };

            var hue = gi * 0.51f;
            var tint = new Vector4
            {
                X = 0.42f + 0.5f * System.MathF.Abs(System.MathF.Sin(hue)),
                Y = 0.48f + 0.45f * System.MathF.Abs(System.MathF.Sin(hue + 2.05f)),
                Z = 0.72f + 0.28f * System.MathF.Abs(System.MathF.Sin(hue + 4.1f)),
                W = 1f,
            };

            go.AddSprite(
                Platformer2DTextureKeys.Gem,
                tint,
                new Vector4 { X = 0f, Y = 0f, Z = 1f, W = 1f },
                620 + gi);

            var hit = go.AddCircleCollider2D(1.0f);
            hit.SetIsTrigger(true);
            hit.SetCategoryBits(Platformer2DLayout.GemHurtboxCategoryBits);
            hit.SetMaskBits(CollisionFilter2D.AllLayersMask());
            go.AddRigidbody2D(SparkRigidbodyBodyType2D.SparkRigidbodyBodyType2D_Static, 0f);
            _gems.Add(go);
        }
    }

    private void AddPlatform(
        GameWorld world,
        float x0,
        float y0,
        float x1,
        float y1,
        uint tileNumber,
        int sortOrder)
    {
        var go = world.CreateGameObject("Platform");
        var tr = go.GetOrAddTransform();
        var cx = (x0 + x1) * 0.5f;
        var cy = (y0 + y1) * 0.5f;
        tr.Translation = new Vector3 { X = cx, Y = cy, Z = 0.01f + 0.001f * sortOrder };
        tr.Scale = new Vector3 { X = System.Math.Abs(x1 - x0), Y = System.Math.Abs(y1 - y0), Z = 1f };

        var uv = _assets.UsingKenneyTilesheet
            ? GameWorld.KenneyPlatformerTileUv(tileNumber)
            : new Vector4 { X = 0f, Y = 0f, Z = 1f, W = 1f };

        go.AddSprite(
            Platformer2DTextureKeys.Tiles,
            new Vector4 { X = 0.95f, Y = 0.92f, Z = 0.88f, W = 1f },
            uv,
            sortOrder);
        go.AddBoxCollider2D(new Vector2 { X = 0.5f, Y = 0.5f });
        go.AddRigidbody2D(SparkRigidbodyBodyType2D.SparkRigidbodyBodyType2D_Static, 0f);
    }

    private GameObject BuildPlayer(
        GameWorld world,
        out TransformComponent transform,
        out Rigidbody2DComponent body,
        out SpriteAnimatorComponent anim,
        out Sprite2DCharacterAnimFsmComponent fsm)
    {
        var go = world.CreateGameObject("Player");
        transform = go.GetOrAddTransform();
        transform.Scale = new Vector3 { X = _playerBaseScaleX, Y = _playerBaseScaleY, Z = 1f };
        var spawnY = Platformer2DLayout.GroundSurfaceY + Platformer2DLayout.PlayerHalfH;
        transform.Translation = new Vector3 { X = Platformer2DLayout.PlayerSpawnX, Y = spawnY, Z = 0.04f };

        fsm = go.AddSprite2DCharacterAnimFsm();

        var cols = _assets.PlayerAtlasColumns > 0 ? _assets.PlayerAtlasColumns : 5u;
        var idleUv = SpriteAnimatorComponent.ComputeUniformGridUv(cols, PlayerAtlasRows, 0);
        go.AddSprite(
            Platformer2DTextureKeys.PlayerAtlas,
            new Vector4 { X = 0.98f, Y = 0.95f, Z = 0.92f, W = 1f },
            idleUv,
            500);

        anim = go.AddSpriteAnimator();
        anim.SetUniformGrid(cols, PlayerAtlasRows);
        anim.AddClip(new SpriteAnimationClip { FirstFrame = 0, FrameCount = 1, FramesPerSecond = 1f, Loop = true });
        anim.AddClip(new SpriteAnimationClip { FirstFrame = 1, FrameCount = 2, FramesPerSecond = 10f, Loop = true });
        anim.AddClip(new SpriteAnimationClip { FirstFrame = 1, FrameCount = 1, FramesPerSecond = 18f, Loop = false });
        anim.AddClip(new SpriteAnimationClip { FirstFrame = 2, FrameCount = 1, FramesPerSecond = 14f, Loop = false });
        anim.ClipIndex = 0;

        fsm.SetLocomotionClips(0, 1);
        fsm.SetCombatClips(2, 3);
        fsm.SetLocomotionSource(SparkSprite2DAnimLocomotionSource.SparkSprite2DAnimLocomotionSource_SpeedSq);
        fsm.SetMoveSpeedThreshold(0.35f);

        go.AddBoxCollider2D(new Vector2 { X = 0.45f, Y = 0.45f });
        body = go.AddRigidbody2D(SparkRigidbodyBodyType2D.SparkRigidbodyBodyType2D_Dynamic, 1f);
        return go;
    }

    private static TextOverlayComponent BuildHud(GameWorld world)
    {
        var hudGo = world.CreateGameObject("Hud");
        var text = hudGo.AddTextOverlay();
        text.SetScreenPosition(12f, 12f);
        text.SetFontSizePixels(20f);
        text.SetColor(new Vector3 { X = 0.95f, Y = 0.98f, Z = 0.96f });
        text.SetText("2D platformer — loading…");
        return text;
    }
}
