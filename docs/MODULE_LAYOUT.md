# Spark module layout (top-level `include/spark`)

| Former path | Current path | Notes |
|-------------|--------------|--------|
| `spark/character/` | `spark/animation/character/` | Motors, root motion arbitration, simulation pipeline |
| `spark/memory/` | `spark/core/` | `SharedPtr`, `UniquePtr`, `SparkMemory.hpp` |
| `spark/gameflow/` | `spark/gameplay/gameflow/` | `GameFlowCoordinator`, persistent run data |
| `spark/imgui/` | `spark/ui/imgui/` | Dear ImGui layer + Vulkan backend glue |
| `spark/text/` | `spark/ui/text/` | `Font` (mesh/UI text) |
| `spark/save/` | `spark/gameplay/save/` | `GameSave` JSON helpers |

Animation clip/skeleton/IK code remains under `spark/animation/` (`locomotion/`, `combat/`, `ik/`, …).