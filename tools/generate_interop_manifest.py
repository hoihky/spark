#!/usr/bin/env python3
"""Build spark-interop-bindings.json from SparkInterop.h (prefix → C# class)."""
from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INTEROP = ROOT / "include/spark/scripting/SparkInterop.h"
INTEROP_GENERATED = ROOT / "include/spark/scripting/SparkInteropComponentBindings.generated.h"
OUT = ROOT / "scripting/bindings/spark-interop-bindings.json"

CLASS_OVERRIDES: dict[str, str] = {
    "sprite_2d_fsm": "Sprite2DCharacterAnimFsmComponent",
    "char_3d_fsm": "Character3DAnimFsmComponent",
    "anim_event_receiver": "AnimationEventReceiverComponent",
    "foliage_instanced": "FoliageInstancedMeshComponent",
    "parallax": "ParallaxLayerComponent",
    "grass_field_scatter": "GrassFieldComponent",
    "grass_field_bounds": "GrassFieldComponent",
    "input_action_map": "InputActionMapComponent",
    "player_input": "PlayerInputComponent",
    "screen_shake": "ScreenShakeComponent",
    "game_state": "GameStateComponent",
    "game_flow_trigger": "GameFlowTriggerComponent",
    "wind_environment": "WindEnvironmentComponent",
    "rigidbody_2d": "Rigidbody2DComponent",
    "rigidbody_3d": "Rigidbody3DComponent",
    "box_collider_2d": "BoxCollider2DComponent",
    "circle_collider_2d": "CircleCollider2DComponent",
    "camera_2d": "Camera2DComponent",
    "camera_2d_rig": "Camera2DRigComponent",
    "one_way_platform_2d": "OneWayPlatform2DComponent",
    "one_way_platform2_d": "OneWayPlatform2DComponent",
    "skinned_mesh": "SkinnedMeshComponent",
    "sound_cue": "SoundCueComponent",
    "terrain": "TerrainComponent",
}

PREFIX_MARKERS = (
    "_get_",
    "_set_",
    "_is_",
    "_has_",
    "_queue_",
    "_apply_",
    "_reset_",
    "_regenerate_",
    "_add_",
    "_clear_",
    "_bind_",
    "_find_",
    "_play_",
    "_stop_",
    "_tick_",
    "_push_",
    "_pop_",
    "_request_",
    "_configure_",
    "_prepare_",
    "_scatter_",
    "_bounds_",
)


def extract_prefix(fn: str) -> str | None:
    if not fn.startswith("spark_"):
        return None
    body = fn[6:]
    for marker in PREFIX_MARKERS:
        if marker in body:
            return body[: body.index(marker)]
    return None


def prefix_to_class_name(prefix: str) -> str:
    if prefix in CLASS_OVERRIDES:
        return CLASS_OVERRIDES[prefix]
    parts = prefix.split("_")
    pascal = "".join(p[:1].upper() + p[1:] for p in parts if p)
    return f"{pascal}Component"


def main() -> int:
    text = INTEROP.read_text(encoding="utf-8")
    if INTEROP_GENERATED.is_file():
        text += "\n" + INTEROP_GENERATED.read_text(encoding="utf-8")
    text = re.sub(r"\s+", " ", text)
    names = re.findall(r"spark_[a-z0-9_]+", text)
    prefixes: set[str] = set()
    for name in names:
        if name.startswith("spark_object_") or name.startswith("spark_component_"):
            continue
        p = extract_prefix(name)
        if p:
            prefixes.add(p)

    prefix_to_class = {}
    for p in sorted(prefixes):
        cls = prefix_to_class_name(p)
        prefix_to_class[p] = {
            "class": cls,
            "namespace": "Spark.Bindings.Components",
            "base": "GameComponentHandle",
        }

    manifest = {
        "skipMirrorCodegenClasses": ["AnimationEventReceiverComponent"],
        "classOverrides": CLASS_OVERRIDES,
        "prefixToClass": prefix_to_class,
    }
    OUT.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {OUT} ({len(prefix_to_class)} prefixes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
