using Spark.Bindings;

namespace HelloCsGame;

/// <summary>Level data mirrored from <c>Platformer2DDemo</c> (Kenney tile indices + platform AABBs).</summary>
internal static class Platformer2DLayout
{
    public const int PlatformCount = 16;
    public const int GemCount = 16;
    public const float PlayerHalfW = 0.40f;
    public const float PlayerHalfH = 0.54f;
    public const float GroundSurfaceY = 0f;
    public const float PlayerSpawnX = -8.5f;
    public const float FallRespawnY = -8.0f;
    public const float RunSpeed = 10f;
    public const float JumpSpeed = 12.8f;
    public const float GemDrawScale = 0.68f;
    public const float GemCollectRadius = 0.62f;
    public const float AttackArcRadius = 1.5f;
    public const float AttackArcHalfAngleRad = 0.96f;
    public const uint PlayerAttackClipIndex = 2;

    public static readonly uint[] PlatformTileNumbers =
    {
        1, 1, 2, 3, 20, 40, 2, 3, 1, 4, 20, 40, 3, 20, 1, 2,
    };

    /// <summary>Axis-aligned platforms as (x0, y0, x1, y1).</summary>
    public static readonly float[][] Platforms =
    {
        new[] { -12.0f, -3.25f, 54.0f, 0.0f },
        new[] { -7.25f, 0.2f, -0.2f, 0.95f },
        new[] { 0.05f, 0.9f, 4.35f, 1.52f },
        new[] { 5.65f, 1.95f, 9.15f, 2.42f },
        new[] { 10.35f, 2.85f, 14.85f, 3.38f },
        new[] { 16.1f, 3.82f, 20.9f, 4.32f },
        new[] { 22.35f, 4.68f, 27.85f, 5.22f },
        new[] { 30.2f, 5.82f, 36.25f, 6.38f },
        new[] { 39.35f, 6.92f, 47.25f, 7.48f },
        new[] { 17.85f, 0.52f, 24.15f, 1.08f },
        new[] { 26.4f, 0.82f, 32.1f, 1.38f },
        new[] { 7.85f, 0.18f, 11.15f, 0.62f },
        new[] { -11.0f, 0.0f, -9.2f, 1.75f },
        new[] { 33.85f, 3.15f, 38.65f, 3.68f },
        new[] { 41.5f, 3.95f, 48.25f, 4.48f },
        new[] { 13.85f, 5.05f, 17.65f, 5.55f },
    };

    public static readonly float[][] GemSpawns =
    {
        new[] { -5.2f, 1.25f },
        new[] { 2.35f, 1.95f },
        new[] { 7.4f, 2.85f },
        new[] { 12.6f, 3.75f },
        new[] { 18.5f, 4.75f },
        new[] { 25.1f, 5.65f },
        new[] { 33.2f, 6.85f },
        new[] { 43.3f, 8.05f },
        new[] { 21.0f, 1.55f },
        new[] { 29.25f, 1.65f },
        new[] { 9.5f, 0.95f },
        new[] { 36.2f, 4.25f },
        new[] { -6.5f, 2.15f },
        new[] { 35.25f, 4.05f },
        new[] { 15.75f, 6.05f },
        new[] { 45.0f, 5.35f },
    };

    public static ushort GemHurtboxCategoryBits => CollisionFilter2D.LayerBit(1);

    public static bool IsSummitGoal(float x, float y) =>
        x > 39.0f && x < 47.5f && y > 6.6f && y < 8.4f;
}
