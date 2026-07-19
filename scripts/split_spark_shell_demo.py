#!/usr/bin/env python3
"""One-shot generator: split SparkShellDemo.cpp into include/spark/demo/*.hpp (+ util .cpp)."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = (ROOT / "src/spark/demo/SparkShellDemo.cpp").read_text().splitlines(keepends=True)
INC = ROOT / "include/spark/demo"
DEMO_SRC = ROOT / "src/spark/demo"
INC.mkdir(parents=True, exist_ok=True)


def lines(a: int, b: int) -> str:
    """1-based inclusive."""
    return "".join(SRC[a - 1 : b])


# --- Shared includes (original lines 1–48) ---
internal_includes = "".join(SRC[0:48])
(INC / "ShellDemoInternalIncludes.hpp").write_text(
    "#pragma once\n\n" + internal_includes
)

(INC / "DemoMode.hpp").write_text(
    """#pragma once

#include "spark/math/Constants.hpp"

#include <cstdint>

namespace Spark {

/** Uniform scale for unit cube [-1,1]³; local translation.y = kCubeScale places bottom on y=0. */
constexpr float kCubeScale = 1.0F;
constexpr float kCubeSpinRadPerSec = TwoPi * 0.35F;

enum class DemoMode : std::uint8_t {
    Menu,
    ThreeD,
    Sky,
    Particles,
    Terrain,
    Character,
    Platformer2D,
};

}  // namespace Spark
"""
)

(INC / "ShellDemoSceneUtil.hpp").write_text(
    """#pragma once

#include "spark/math/Matrix4.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/core/Array.hpp"

namespace Spark {

Quaternion QuaternionFromRotationColumns(
        const Vector3& col0, const Vector3& col1, const Vector3& col2) noexcept;

int DrawSortKey(const SceneDrawItem& it);

void StableSortDrawItems(Array<SceneDrawItem>& items);

[[nodiscard]] bool TerrainScreenToWorldRay(
        int fbW,
        int fbH,
        float px,
        float py,
        const Matrix4& invViewProj,
        Vector3& outOrigin,
        Vector3& outDir);

}  // namespace Spark
"""
)

body_q = lines(52, 79).replace("Spark::", "")
scene_util_cpp = (
    '#include "spark/demo/ShellDemoSceneUtil.hpp"\n\n'
    "#include <algorithm>\n#include <cmath>\n\n"
    "namespace Spark {\n\nnamespace {\n\n"
    "int DrawSortLayer(SceneMeshSlot s) {\n"
    "    switch (s) {\n"
    "    case SceneMeshSlot::GroundPlane:\n"
    "        return 0;\n"
    "    case SceneMeshSlot::UnitCube:\n"
    "        return 1;\n"
    "    case SceneMeshSlot::Custom:\n"
    "        return 2;\n"
    "    }\n"
    "    return 1;\n"
    "}\n\n}  // namespace\n\n"
    + body_q
    + "\n"
    + """int DrawSortKey(const SceneDrawItem& it) {
    if (it.skyMode != SceneSkyMode::None) {
        return -100;
    }
    return DrawSortLayer(it.mesh);
}

"""
    + lines(101, 112).replace("Spark::", "")
    + "\n"
    + lines(1717, 1744).replace("Spark::", "")
    + "\n}  // namespace Spark\n"
)

(DEMO_SRC / "ShellDemoSceneUtil.cpp").write_text(scene_util_cpp)

(INC / "ShellDemoUi.hpp").write_text(
    """#pragma once

#include "spark/demo/ShellDemoInternalIncludes.hpp"

#include "spark/gui/Widget.hpp"

namespace Spark {

"""
    + lines(130, 226)
    + """
void MountUiFont(GameWorld& w);

}  // namespace Spark
"""
)

(DEMO_SRC / "ShellDemoUi.cpp").write_text(
    """#include "spark/demo/ShellDemoUi.hpp"

namespace Spark {

"""
    + lines(228, 272)
    + """
}  // namespace Spark
"""
)


def demo_header(name: str, body_lines: tuple[int, int], extra: str = "") -> None:
    text = (
        "#pragma once\n\n"
        '#include "spark/demo/ShellDemoInternalIncludes.hpp"\n'
        '#include "spark/demo/DemoMode.hpp"\n'
        '#include "spark/demo/ShellDemoSceneUtil.hpp"\n'
        + extra
        + "\nnamespace Spark {\n\n"
        + lines(body_lines[0], body_lines[1])
        + "\n}  // namespace Spark\n"
    )
    (INC / f"{name}.hpp").write_text(text)


demo_header("ThreeDDemo", (274, 744))
demo_header("SkyDemo", (746, 1119))
demo_header("ParticleDemo", (1121, 1340))
demo_header("TwoDDemo", (1341, 1520))
demo_header("Platformer2DDemo", (1521, 1715))
demo_header("TerrainDemo", (1747, 2079))

(INC / "CharacterCameraDemo.hpp").write_text(
    "#pragma once\n\n"
    '#include "spark/demo/ShellDemoInternalIncludes.hpp"\n'
    '#include "spark/demo/ShellDemoSceneUtil.hpp"\n\n'
    "namespace {\n\n"
    + lines(2087, 2123)
    + "\n}  // namespace\n\n"
    "namespace Spark {\n\n"
    + lines(2128, 2671)
    + "\n}  // namespace Spark\n"
)

shell_cpp = (
    '#include "spark/demo/ShellDemoInternalIncludes.hpp"\n'
    '#include "spark/demo/DemoMode.hpp"\n'
    '#include "spark/demo/ShellDemoUi.hpp"\n'
    '#include "spark/demo/ThreeDDemo.hpp"\n'
    '#include "spark/demo/SkyDemo.hpp"\n'
    '#include "spark/demo/ParticleDemo.hpp"\n'
    '#include "spark/demo/TerrainDemo.hpp"\n'
    '#include "spark/demo/CharacterCameraDemo.hpp"\n'
    '#include "spark/demo/TwoDDemo.hpp"\n'
    '#include "spark/demo/Platformer2DDemo.hpp"\n'
    "\nnamespace Spark {\n\n"
    + lines(2673, 3332)
)
(ROOT / "src/spark/demo/SparkShellDemo.cpp").write_text(shell_cpp)

print("OK: headers under include/spark/demo/, SparkShellDemo.cpp + ShellDemoSceneUtil.cpp + ShellDemoUi.cpp")
