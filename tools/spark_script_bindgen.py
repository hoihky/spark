#!/usr/bin/env python3
"""Generate SparkInterop component C API from SPARK_SCRIPT_BIND markers in *Component.hpp."""
from __future__ import annotations

import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
COMPONENT_ROOT = ROOT / "include/spark/ecs/components"
OUT_H = ROOT / "include/spark/scripting/SparkInteropComponentBindings.generated.h"
OUT_CPP = ROOT / "src/spark/scripting/SparkInteropComponentBindings.generated.cpp"
INTEROP_H = ROOT / "include/spark/scripting/SparkInterop.h"
GEN_INCLUDE_MARKER = '#include "spark/scripting/SparkInteropComponentBindings.generated.h"'

BIND_RE = re.compile(
    r"SPARK_SCRIPT_BIND\((?P<suffix>[\w]+)\)\s*"
    r"(?:(?:\[\[.*?\]\]\s*)*)"
    r"(?:(?:virtual\s+))?"
    r"(?P<ret>[\w:<>,\s*&]+?)\s+"
    r"(?P<name>\w+)\s*"
    r"\((?P<args>[^)]*)\)\s*"
    r"(?P<const>const)?\s*(?:noexcept)?\s*"
    r"(?:\{[^{}]*\}\s*)?;?",
    re.MULTILINE,
)

CLASS_RE = re.compile(
    r"class\s+(?P<name>\w+Component)\s+final\s*:\s*public\s+GameComponent",
    re.MULTILINE,
)

OUT_PARAM_OVERRIDES: dict[tuple[str, str], str] = {
    ("parallax", "get_anchor_world"): "outAnchor",
    ("screen_shake", "get_offset"): "outOffset",
    ("wind_environment", "get_direction"): "outDirectionWorld",
}

def load_prefix_overrides_from_manifest() -> dict[str, str]:
    manifest_path = ROOT / "scripting/bindings/spark-interop-bindings.json"
    overrides: dict[str, str] = {}
    if manifest_path.is_file():
        import json

        data = json.loads(manifest_path.read_text(encoding="utf-8"))
        for prefix, class_name in (data.get("classOverrides") or {}).items():
            if isinstance(class_name, str):
                overrides[class_name] = prefix
        for prefix, binding in (data.get("prefixToClass") or {}).items():
            if isinstance(binding, dict) and "class" in binding:
                overrides.setdefault(binding["class"], prefix)
    return overrides


PREFIX_OVERRIDES: dict[str, str] = {
    "Sprite2DCharacterAnimFsmComponent": "sprite_2d_fsm",
    "Character3DAnimFsmComponent": "char_3d_fsm",
    "AnimationEventReceiverComponent": "anim_event_receiver",
    "FoliageInstancedMeshComponent": "foliage_instanced",
    "GameStateComponent": "game_state",
    "ParallaxLayerComponent": "parallax",
    "ScreenShakeComponent": "screen_shake",
    "GameFlowTriggerComponent": "game_flow_trigger",
    "WindEnvironmentComponent": "wind_environment",
    "GrassFieldComponent": "grass_field",
    "InputActionMapComponent": "input_action_map",
    "PlayerInputComponent": "player_input",
    "OneWayPlatform2DComponent": "one_way_platform_2d",
    "SkinnedMeshComponent": "skinned_mesh",
    "SoundCueComponent": "sound_cue",
    "TerrainComponent": "terrain",
    "Camera2DRigComponent": "camera_2d_rig",
}
# Manifest classOverrides are prefix→class; only apply when they do not clobber explicit entries above.
for _class, _prefix in load_prefix_overrides_from_manifest().items():
    PREFIX_OVERRIDES.setdefault(_class, _prefix)


@dataclass
class BoundMethod:
    suffix: str
    cpp_name: str
    return_type: str
    args: list[tuple[str, str]]
    is_const: bool


@dataclass
class BoundComponent:
    class_name: str
    kind_name: str
    prefix: str
    header: Path
    methods: list[BoundMethod] = field(default_factory=list)


def pascal_to_prefix(kind: str) -> str:
    if kind.endswith("Component"):
        kind = kind[: -len("Component")]
    parts: list[str] = []
    buf = ""
    for ch in kind:
        if ch.isupper() and buf:
            parts.append(buf.lower())
            buf = ch
        else:
            buf += ch
    if buf:
        parts.append(buf.lower())
    return "_".join(parts)


def parse_args(args: str) -> list[tuple[str, str]]:
    if not args.strip():
        return []
    args = re.sub(r"\s*=\s*[^,()]+", "", args)
    out: list[tuple[str, str]] = []
    for part in args.split(","):
        part = part.strip()
        if not part:
            continue
        if " " in part:
            typ, name = part.rsplit(" ", 1)
            out.append((typ.strip(), name.strip()))
        else:
            out.append((part, ""))
    return out


def normalize_cpp_type(typ: str) -> str:
    return re.sub(r"\s+", " ", typ.replace("const ", "").replace("&", "").strip())


def load_manual_spark_functions() -> set[str]:
    text = INTEROP_H.read_text(encoding="utf-8")
    if GEN_INCLUDE_MARKER in text:
        text = text.split(GEN_INCLUDE_MARKER, 1)[0]
    return set(re.findall(r"\b(spark_[a-z0-9_]+)\s*\(", text))


def spark_enum_type_name(typ: str) -> str | None:
    if "&" in typ or "*" in typ:
        return None
    norm = normalize_cpp_type(typ)
    primitives = {
        "void",
        "bool",
        "float",
        "int",
        "double",
        "Vector2",
        "Vector3",
        "Quaternion",
        "Matrix4",
        "Camera2D",
        "GameObject",
    }
    if not norm or norm in primitives or norm.startswith("std::"):
        return None
    if "::" in norm:
        return None
    if norm.endswith(
        (
            "Settings",
            "Desc",
            "Spec",
            "Filter",
            "Config",
            "Data",
            "Blackboard",
            "Module",
            "Cell",
            "Payload",
            "Clip",
        )
    ):
        return None
    if not re.search(
        r"(Mode2D|Type2D|Type3D|BodyType2D|BodyType3D|Direction3D|PartitionKind|PresetId|Preset|Feature|Marker|"
        r"Mode|State|Source|Shape|Plane|Mask|Flags|Layer|Type)$",
        norm,
    ):
        return None
    if norm[0].isupper():
        return norm
    return None


def is_bindable_method(method: BoundMethod) -> bool:
    ret = method.return_type.strip()
    norm = normalize_cpp_type(ret)
    if ret == "void":
        pass
    elif norm in {"Vector2", "Vector3", "float", "int", "bool"}:
        pass
    elif "int32_t" in ret or "std::int32_t" in ret:
        pass
    elif spark_enum_type_name(ret):
        pass
    elif "uint32_t" in ret or "std::uint32_t" in ret:
        pass
    elif "uint64_t" in ret or "std::uint64_t" in ret:
        pass
    elif "GameFlowState" in ret or "GameFlowTriggerSource" in ret:
        pass
    elif "ParallaxAxisMode" in ret or "ParallaxDriftMode" in ret:
        pass
    elif "&" in ret or "*" in ret:
        return False
    elif norm in {"Matrix4", "Camera2D", "Quaternion"}:
        return False
    else:
        return False

    for typ, _ in method.args:
        arg_norm = normalize_cpp_type(typ)
        if "const char* const*" in typ.replace(" ", ""):
            return False
        if "const char*" in typ and "*" in typ and "const char*" != typ.strip():
            return False
        if arg_norm.endswith(
            ("Settings", "Desc", "Spec", "Filter", "Config", "Data", "Blackboard", "Module", "Cell")
        ):
            return False
        if any(
            frag in typ
            for frag in (
                "Function<",
                "SharedPtr",
                "UniquePtr",
                "Array<",
                "std::function",
                "GameWorld",
                "IEngineContext",
                "FrameTiming",
                "Matrix4",
                "Camera2D",
            )
        ):
            return False
        if typ.strip() == "GameObject&":
            return False
        if arg_norm in {"Vector2", "Vector3"} and "&" in typ and "const" not in typ:
            return False
        if "&" in typ and arg_norm not in {"Vector2", "Vector3", "GameObject"}:
            return False
        if "*" in typ and "GameObject" not in typ and "char" not in typ:
            if "InputActionMapComponent" not in typ:
                return False
    return True


def dedupe_overloads(methods: list[BoundMethod]) -> list[BoundMethod]:
    by_suffix: dict[str, list[BoundMethod]] = {}
    for method in methods:
        by_suffix.setdefault(method.suffix, []).append(method)
    picked: list[BoundMethod] = []
    for group in by_suffix.values():
        if len(group) == 1:
            picked.append(group[0])
            continue
        mutators = [m for m in group if m.return_type.strip() == "void" and not m.is_const]
        if mutators:
            picked.append(mutators[0])
            continue
        const_reads = [m for m in group if m.is_const]
        if const_reads:
            picked.append(const_reads[0])
            continue
        picked.append(group[0])
    return picked


def out_vector_kind(method: BoundMethod) -> str | None:
    ret = normalize_cpp_type(method.return_type)
    if ret == "Vector3":
        return "vector3"
    if ret == "Vector2":
        return "vector2"
    return None


def map_c_return(cpp_ret: str) -> str:
    cpp_ret = cpp_ret.strip()
    if cpp_ret == "void":
        return "void"
    if cpp_ret == "bool":
        return "int"
    if cpp_ret == "float":
        return "float"
    if cpp_ret == "int":
        return "int"
    if "int32_t" in cpp_ret or "std::int32_t" in cpp_ret:
        return "int32_t"
    norm = normalize_cpp_type(cpp_ret)
    enum_map = {
        "GameFlowState": "SparkGameFlowState",
        "GameFlowTriggerSource": "SparkGameFlowTriggerSource",
        "ParallaxAxisMode": "SparkParallaxAxisMode",
        "ParallaxDriftMode": "SparkParallaxDriftMode",
    }
    if norm in enum_map:
        return enum_map[norm]
    if spark_enum_type_name(cpp_ret):
        return "int"
    if norm.endswith("Mode") or norm.endswith("State"):
        return "int"
    if "uint32_t" in cpp_ret or "std::uint32_t" in cpp_ret:
        return "uint32_t"
    if "uint64_t" in cpp_ret or "std::uint64_t" in cpp_ret:
        return "uint64_t"
    return "int"


def c_param_from_cpp(typ: str, name: str, index: int) -> tuple[str, str]:
    """Returns (c_decl, call_expr_fragment). call uses `n` as variable name."""
    n = name or f"arg{index}"
    stripped = typ.strip()
    if stripped in {"GameObject&", "const GameObject&"}:
        return (
            f"SparkGameObject* {n}",
            f"*reinterpret_cast<Spark::GameObject*>({n})",
        )
    if "GameObject*" in stripped:
        return f"SparkGameObject* {n}", f"reinterpret_cast<Spark::GameObject*>({n})"
    if "InputActionMapComponent*" in stripped:
        return (
            f"SparkGameComponent* {n}",
            f"ResolveInputActionMap({n})",
        )
    if stripped == "float*":
        return f"float* {n}", n
    if "Vector3" in typ:
        if "*" in typ:
            return f"SparkVector3* {n}", n
        return f"const SparkVector3* {n}", f"Spark::Scripting::ToVector3(*{n})"
    if "Vector2" in typ:
        if "*" in typ:
            return f"SparkVector2* {n}", n
        return f"const SparkVector2* {n}", f"Spark::Scripting::ToVector2(*{n})"
    if stripped == "bool":
        return f"int {n}", f"{n} != 0"
    if stripped == "const char*":
        return f"const char* {n}", n
    if stripped == "float":
        return f"float {n}", n
    if stripped == "int":
        return f"int {n}", n
    if "uint32_t" in stripped or "std::uint32_t" in stripped:
        return f"uint32_t {n}", n
    if "uint64_t" in stripped or "std::uint64_t" in stripped:
        return f"uint64_t {n}", n
    if stripped in {"float", "const float"}:
        return f"float {n}", n
    if stripped in {"int", "const int"}:
        return f"int {n}", n
    if "GameFlowState" in stripped:
        return (
            f"SparkGameFlowState {n}",
            f"static_cast<Spark::GameFlowState>(static_cast<std::uint8_t>({n}))",
        )
    if "GameFlowTriggerSource" in stripped:
        return (
            f"SparkGameFlowTriggerSource {n}",
            f"static_cast<Spark::GameFlowTriggerSource>(static_cast<std::uint8_t>({n}))",
        )
    if "ParallaxAxisMode" in stripped:
        return (
            f"SparkParallaxAxisMode {n}",
            f"static_cast<Spark::ParallaxAxisMode>(static_cast<std::uint8_t>({n}))",
        )
    if "ParallaxDriftMode" in stripped:
        return (
            f"SparkParallaxDriftMode {n}",
            f"static_cast<Spark::ParallaxDriftMode>(static_cast<std::uint8_t>({n}))",
        )
    norm = normalize_cpp_type(stripped)
    enum_name = spark_enum_type_name(stripped)
    if enum_name:
        return (
            f"int {n}",
            f"static_cast<Spark::{enum_name}>(static_cast<std::uint8_t>({n}))",
        )
    if norm.endswith("Mode") or norm.endswith("State"):
        return (
            f"int {n}",
            f"static_cast<Spark::{norm}>(static_cast<std::uint8_t>({n}))",
        )
    if "Vector3" in stripped and "&" in stripped:
        return f"const SparkVector3* {n}", f"Spark::Scripting::ToVector3(*{n})"
    c_typ = map_c_return(stripped) if stripped not in {"float", "int"} else stripped
    return f"{c_typ} {n}", n


def out_param_name(prefix: str, suffix: str, kind: str) -> str:
    key = (prefix, suffix)
    if key in OUT_PARAM_OVERRIDES:
        return OUT_PARAM_OVERRIDES[key]
    tail = suffix.removeprefix("get_")
    return f"out{''.join(p.capitalize() for p in tail.split('_'))}"


def component_param(is_const: bool) -> str:
    return (
        "const SparkGameComponent* component"
        if is_const
        else "SparkGameComponent* component"
    )


def method_arg_decls(method: BoundMethod) -> list[tuple[str, str]]:
    decls: list[tuple[str, str]] = []
    for i, (typ, name) in enumerate(method.args, start=1):
        decls.append(c_param_from_cpp(typ, name, i))
    return decls


def emit_c_prototype(comp: BoundComponent, method: BoundMethod) -> str:
    fn = spark_fn_name(comp.prefix, method.suffix)
    params = [component_param(method.is_const)]
    out_kind = out_vector_kind(method)
    if out_kind == "vector3":
        for decl, _ in method_arg_decls(method):
            params.append(decl)
        params.append(f"SparkVector3* {out_param_name(comp.prefix, method.suffix, 'vector3')}")
        return f"SPARK_SCRIPT_API void {fn}({', '.join(params)});"
    if out_kind == "vector2":
        for decl, _ in method_arg_decls(method):
            params.append(decl)
        params.append(f"SparkVector2* {out_param_name(comp.prefix, method.suffix, 'vector2')}")
        return f"SPARK_SCRIPT_API void {fn}({', '.join(params)});"
    ret = map_c_return(method.return_type.strip())
    for i, (typ, name) in enumerate(method.args, start=1):
        decl, _ = c_param_from_cpp(typ, name, i)
        params.append(decl)
    return f"SPARK_SCRIPT_API {ret} {fn}({', '.join(params)});"


def default_return(method: BoundMethod, ret_c: str) -> str:
    if ret_c == "void":
        return "        return;\n"
    if method.return_type.strip() == "bool":
        return "        return 0;\n"
    if ret_c == "float":
        return "        return 0.0F;\n"
    if "GameFlowState" in method.return_type:
        return "        return SparkGameFlowState_Playing;\n"
    if "ParallaxAxisMode" in method.return_type:
        return "        return SparkParallaxAxisMode_Horizontal;\n"
    return "        return 0;\n"


def emit_c_function(comp: BoundComponent, method: BoundMethod) -> list[str]:
    fn = spark_fn_name(comp.prefix, method.suffix)
    receiver = f"As{comp.class_name}"
    lines: list[str] = []
    out_kind = out_vector_kind(method)
    if out_kind == "vector3":
        out_name = out_param_name(comp.prefix, method.suffix, "vector3")
        arg_pairs = method_arg_decls(method)
        sig_params = [component_param(method.is_const)]
        sig_params.extend(decl for decl, _ in arg_pairs)
        sig_params.append(f"SparkVector3* {out_name}")
        lines.append(f"void {fn}({', '.join(sig_params)}) {{")
        lines.append(f"    auto{'*' if method.is_const else ''} self = {receiver}(component);")
        lines.append(f"    if (self == nullptr || {out_name} == nullptr) {{")
        lines.append("        return;\n    }")
        call_args = [call for _, call in arg_pairs]
        lines.append(
            f"    *{out_name} = Spark::Scripting::FromVector3("
            f"self->{method.cpp_name}({', '.join(call_args)}));"
        )
        lines.append("}")
        return lines
    if out_kind == "vector2":
        out_name = out_param_name(comp.prefix, method.suffix, "vector2")
        arg_pairs = method_arg_decls(method)
        sig_params = [component_param(method.is_const)]
        sig_params.extend(decl for decl, _ in arg_pairs)
        sig_params.append(f"SparkVector2* {out_name}")
        lines.append(f"void {fn}({', '.join(sig_params)}) {{")
        lines.append(f"    auto{'*' if method.is_const else ''} self = {receiver}(component);")
        lines.append(f"    if (self == nullptr || {out_name} == nullptr) {{")
        lines.append("        return;\n    }")
        call_args = [call for _, call in arg_pairs]
        lines.append(
            f"    *{out_name} = Spark::Scripting::FromVector2("
            f"self->{method.cpp_name}({', '.join(call_args)}));"
        )
        lines.append("}")
        return lines

    arg_decls: list[str] = []
    call_args: list[str] = []
    for i, (typ, name) in enumerate(method.args, start=1):
        decl, call = c_param_from_cpp(typ, name, i)
        arg_decls.append(decl)
        if "ResolveInputActionMap" in call:
            call_args.append(call)
        else:
            call_args.append(call)

    ret_c = map_c_return(method.return_type.strip())
    sig = f"{ret_c} {fn}({', '.join([component_param(method.is_const)] + arg_decls)})"
    lines.append(f"{sig} {{")
    lines.append(f"    auto{'*' if method.is_const else ''} self = {receiver}(component);")
    lines.append("    if (self == nullptr) {")
    lines.append(default_return(method, ret_c))
    lines.append("    }")
    if method.return_type.strip() == "void":
        lines.append(f"    self->{method.cpp_name}({', '.join(call_args)});")
    elif method.return_type.strip() == "bool":
        lines.append(f"    return self->{method.cpp_name}({', '.join(call_args)}) ? 1 : 0;")
    elif method.return_type.strip() == "float":
        lines.append(f"    return self->{method.cpp_name}({', '.join(call_args)});")
    elif "uint64_t" in method.return_type or "std::uint64_t" in method.return_type:
        lines.append(f"    return self->{method.cpp_name}({', '.join(call_args)});")
    elif "GameFlowState" in method.return_type:
        lines.append(
            f"    return static_cast<SparkGameFlowState>(static_cast<std::uint8_t>("
            f"self->{method.cpp_name}({', '.join(call_args)})));"
        )
    elif "ParallaxAxisMode" in method.return_type:
        lines.append(
            f"    return static_cast<SparkParallaxAxisMode>(static_cast<std::uint8_t>("
            f"self->{method.cpp_name}({', '.join(call_args)})));"
        )
    elif spark_enum_type_name(method.return_type) or normalize_cpp_type(method.return_type).endswith(
        ("Mode", "State")
    ):
        lines.append(
            f"    return static_cast<int>(static_cast<std::uint8_t>("
            f"self->{method.cpp_name}({', '.join(call_args)})));"
        )
    else:
        lines.append(f"    return self->{method.cpp_name}({', '.join(call_args)});")
    lines.append("}")
    return lines


def spark_fn_name(prefix: str, suffix: str) -> str:
    return f"spark_{prefix}_{suffix}"


def scan_components() -> list[BoundComponent]:
    manual_fns = load_manual_spark_functions()
    components: list[BoundComponent] = []
    for path in sorted(COMPONENT_ROOT.rglob("*Component.hpp")):
        text = path.read_text(encoding="utf-8", errors="ignore")
        cm = CLASS_RE.search(text)
        if not cm:
            continue
        class_name = cm.group("name")
        prefix = PREFIX_OVERRIDES.get(class_name, pascal_to_prefix(class_name))
        kind = class_name[: -len("Component")] if class_name.endswith("Component") else class_name
        comp = BoundComponent(class_name=class_name, kind_name=kind, prefix=prefix, header=path)
        for m in BIND_RE.finditer(text):
            method = BoundMethod(
                suffix=m.group("suffix"),
                cpp_name=m.group("name"),
                return_type=m.group("ret").strip(),
                args=parse_args(m.group("args")),
                is_const=m.group("const") is not None,
            )
            if not is_bindable_method(method):
                continue
            fn = spark_fn_name(prefix, method.suffix)
            if fn in manual_fns:
                continue
            comp.methods.append(method)
        comp.methods = dedupe_overloads(comp.methods)
        if comp.methods:
            components.append(comp)
    return components


def emit_header(components: list[BoundComponent]) -> str:
    lines = [
        "/* <auto-generated /> — SPARK_SCRIPT_BIND. Do not edit. */",
        "#pragma once",
        "/* Included from SparkInterop.h inside extern \"C\" — do not wrap linkage here. */",
        "",
    ]
    for comp in components:
        for method in comp.methods:
            lines.append(emit_c_prototype(comp, method))
        lines.append("")
    return "\n".join(lines)


def emit_cpp(components: list[BoundComponent]) -> str:
    includes = set()
    lines = [
        "/* <auto-generated /> */",
        '#include "spark/scripting/SparkInterop.h"',
        '#include "spark/scripting/SparkInteropInternal.hpp"',
        '#include "spark/ecs/GameObject.hpp"',
        "",
    ]
    needs_input_map = any(
        m.args and any("InputActionMapComponent" in t for t, _ in m.args)
        for c in components
        for m in c.methods
    )
    if needs_input_map:
        lines.append('#include "spark/ecs/components/input/InputActionMapComponent.hpp"')
        lines.append("")

    for comp in components:
        rel = comp.header.relative_to(ROOT / "include")
        includes.add(f'#include "{rel.as_posix()}"')
    lines.extend(sorted(includes))
    lines.append("")
    lines.append("namespace {")
    lines.append("")
    for comp in components:
        lines.append(f"Spark::{comp.class_name}* As{comp.class_name}(SparkGameComponent* c) {{")
        lines.append(f"    return Spark::Scripting::AsComponent<Spark::{comp.class_name}>(")
        lines.append(f"            c, Spark::ComponentKind::{comp.kind_name});")
        lines.append("}")
        lines.append(f"const Spark::{comp.class_name}* As{comp.class_name}(const SparkGameComponent* c) {{")
        lines.append(f"    return Spark::Scripting::AsComponent<const Spark::{comp.class_name}>(")
        lines.append(f"            c, Spark::ComponentKind::{comp.kind_name});")
        lines.append("}")
        lines.append("")
    if needs_input_map:
        lines.append("Spark::InputActionMapComponent* ResolveInputActionMap(SparkGameComponent* c) {")
        lines.append(
            "    return Spark::Scripting::AsComponent<Spark::InputActionMapComponent>("
        )
        lines.append("            c, Spark::ComponentKind::InputActionMap);")
        lines.append("}")
        lines.append("")

    lines.append("}  // namespace")
    lines.append("")
    lines.append("extern \"C\" {")
    lines.append("")

    for comp in components:
        for method in comp.methods:
            lines.extend(emit_c_function(comp, method))
            lines.append("")

    lines.append("}  // extern \"C\"")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    components = scan_components()
    OUT_H.parent.mkdir(parents=True, exist_ok=True)
    OUT_CPP.parent.mkdir(parents=True, exist_ok=True)
    OUT_H.write_text(emit_header(components), encoding="utf-8")
    OUT_CPP.write_text(emit_cpp(components), encoding="utf-8")
    print(
        f"spark_script_bindgen: {len(components)} components, "
        f"{sum(len(c.methods) for c in components)} bindings -> {OUT_H.name}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
