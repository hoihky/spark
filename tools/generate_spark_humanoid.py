#!/usr/bin/env python3
"""Generate assets/models/SparkHumanoid.glb — minimal skinned humanoid test rig."""

from __future__ import annotations

import json
import math
import struct
import zlib
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
OUT_PATH = REPO_ROOT / "assets" / "models" / "SparkHumanoid.glb"

# glTF node indices for joints (scene graph).
JOINT_ROOT = 2
JOINT_HIPS = 3
JOINT_SPINE = 4
JOINT_HEAD = 5
JOINT_ARM_L = 6
JOINT_ARM_R = 7
JOINT_LEG_L = 8
JOINT_LEG_R = 9
JOINT_NODE_INDICES = [
    JOINT_ROOT,
    JOINT_HIPS,
    JOINT_SPINE,
    JOINT_HEAD,
    JOINT_ARM_L,
    JOINT_ARM_R,
    JOINT_LEG_L,
    JOINT_LEG_R,
]

# Skin palette indices (JOINTS_0 attribute values index into skin.joints).
SKIN_ROOT = 0
SKIN_HIPS = 1
SKIN_SPINE = 2
SKIN_HEAD = 3
SKIN_ARM_L = 4
SKIN_ARM_R = 5
SKIN_LEG_L = 6
SKIN_LEG_R = 7


def mat4_identity() -> list[float]:
    return [
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    ]


def mat4_mul(a: list[float], b: list[float]) -> list[float]:
    out = [0.0] * 16
    for col in range(4):
        for row in range(4):
            out[col * 4 + row] = sum(a[k * 4 + row] * b[col * 4 + k] for k in range(4))
    return out


def mat4_translation(x: float, y: float, z: float) -> list[float]:
    m = mat4_identity()
    m[12], m[13], m[14] = x, y, z
    return m


def mat4_rotation_x(angle: float) -> list[float]:
    c = math.cos(angle)
    s = math.sin(angle)
    return [
        1.0, 0.0, 0.0, 0.0,
        0.0, c, s, 0.0,
        0.0, -s, c, 0.0,
        0.0, 0.0, 0.0, 1.0,
    ]


def mat4_rotation_z(angle: float) -> list[float]:
    c = math.cos(angle)
    s = math.sin(angle)
    return [
        c, s, 0.0, 0.0,
        -s, c, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    ]


def mat4_try_invert(m: list[float]) -> list[float]:
    # Gauss-Jordan for 4x4 (column-major storage).
    a = [list(m[i::4]) for i in range(4)]
    inv = [[1.0 if i == j else 0.0 for j in range(4)] for i in range(4)]
    for col in range(4):
        pivot = col
        for row in range(col + 1, 4):
            if abs(a[row][col]) > abs(a[pivot][col]):
                pivot = row
        if abs(a[pivot][col]) < 1.0e-12:
            return mat4_identity()
        if pivot != col:
            a[col], a[pivot] = a[pivot], a[col]
            inv[col], inv[pivot] = inv[pivot], inv[col]
        div = a[col][col]
        a[col] = [v / div for v in a[col]]
        inv[col] = [v / div for v in inv[col]]
        for row in range(4):
            if row == col:
                continue
            factor = a[row][col]
            a[row] = [a[row][c] - factor * a[col][c] for c in range(4)]
            inv[row] = [inv[row][c] - factor * inv[col][c] for c in range(4)]
    out = [0.0] * 16
    for col in range(4):
        for row in range(4):
            out[col * 4 + row] = inv[row][col]
    return out


def png_from_rgba(width: int, height: int, rgba: bytes) -> bytes:
    def chunk(tag: bytes, data: bytes) -> bytes:
        return (
            struct.pack(">I", len(data))
            + tag
            + data
            + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
        )

    raw = bytearray()
    stride = width * 4
    for row in range(height):
        raw.append(0)
        raw.extend(rgba[row * stride : (row + 1) * stride])
    return (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(bytes(raw), 9))
        + chunk(b"IEND", b"")
    )


def flat_normal_png() -> bytes:
    return png_from_rgba(2, 2, bytes([128, 128, 255, 255] * 4))


def orm_png(roughness: float, metallic: float) -> bytes:
    g = max(0, min(255, int(round(roughness * 255.0))))
    b = max(0, min(255, int(round(metallic * 255.0))))
    pixel = bytes([0, g, b, 255])
    return png_from_rgba(2, 2, pixel * 4)


def box_vertices(cx: float, cy: float, cz: float, hx: float, hy: float, hz: float, joint: int):
    corners = [
        ((cx - hx, cy - hy, cz - hz), (0.0, 0.0), (0.0, -1.0, 0.0)),
        ((cx + hx, cy - hy, cz - hz), (1.0, 0.0), (0.0, -1.0, 0.0)),
        ((cx + hx, cy + hy, cz - hz), (1.0, 1.0), (0.0, 1.0, 0.0)),
        ((cx - hx, cy + hy, cz - hz), (0.0, 1.0), (0.0, 1.0, 0.0)),
        ((cx - hx, cy - hy, cz + hz), (0.0, 0.0), (0.0, 0.0, 1.0)),
        ((cx + hx, cy - hy, cz + hz), (1.0, 0.0), (0.0, 0.0, 1.0)),
        ((cx + hx, cy + hy, cz + hz), (1.0, 1.0), (0.0, 0.0, 1.0)),
        ((cx - hx, cy + hy, cz + hz), (0.0, 1.0), (0.0, 0.0, 1.0)),
    ]
    verts = []
    for (px, py, pz), (u, v), normal in corners:
        verts.append(
            {
                "pos": (px, py, pz),
                "uv": (u, v),
                "normal": normal,
                "joints": (joint, 0, 0, 0),
                "weights": (1.0, 0.0, 0.0, 0.0),
            }
        )
    return verts


def build_mesh_vertices() -> list[dict]:
    verts: list[dict] = []
    verts += box_vertices(0.0, 1.05, 0.0, 0.14, 0.22, 0.10, SKIN_SPINE)
    verts += box_vertices(0.0, 1.42, 0.0, 0.10, 0.10, 0.10, SKIN_HEAD)
    verts += box_vertices(-0.28, 1.08, 0.0, 0.06, 0.18, 0.06, SKIN_ARM_L)
    verts += box_vertices(0.28, 1.08, 0.0, 0.06, 0.18, 0.06, SKIN_ARM_R)
    verts += box_vertices(-0.10, 0.55, 0.0, 0.07, 0.28, 0.07, SKIN_LEG_L)
    verts += box_vertices(0.10, 0.55, 0.0, 0.07, 0.28, 0.07, SKIN_LEG_R)
    return verts


def cube_indices(base: int) -> list[int]:
    return [
        base + 0, base + 1, base + 2, base + 0, base + 2, base + 3,
        base + 4, base + 6, base + 5, base + 4, base + 7, base + 6,
        base + 0, base + 4, base + 5, base + 0, base + 5, base + 1,
        base + 2, base + 6, base + 7, base + 2, base + 7, base + 3,
        base + 0, base + 3, base + 7, base + 0, base + 7, base + 4,
        base + 1, base + 5, base + 6, base + 1, base + 6, base + 2,
    ]


def joint_local_mats() -> dict[int, list[float]]:
    """Bind-pose local transforms keyed by joint node index."""
    return {
        JOINT_ROOT: mat4_identity(),
        JOINT_HIPS: mat4_translation(0.0, 0.9, 0.0),
        JOINT_SPINE: mat4_translation(0.0, 0.2, 0.0),
        JOINT_HEAD: mat4_translation(0.0, 0.25, 0.0),
        JOINT_ARM_L: mat4_translation(-0.2, 0.15, 0.0),
        JOINT_ARM_R: mat4_translation(0.2, 0.15, 0.0),
        JOINT_LEG_L: mat4_translation(-0.1, -0.05, 0.0),
        JOINT_LEG_R: mat4_translation(0.1, -0.05, 0.0),
    }


def joint_parents() -> dict[int, int | None]:
    return {
        JOINT_ROOT: None,
        JOINT_HIPS: JOINT_ROOT,
        JOINT_SPINE: JOINT_HIPS,
        JOINT_HEAD: JOINT_SPINE,
        JOINT_ARM_L: JOINT_SPINE,
        JOINT_ARM_R: JOINT_SPINE,
        JOINT_LEG_L: JOINT_HIPS,
        JOINT_LEG_R: JOINT_HIPS,
    }


def joint_world_mats() -> dict[int, list[float]]:
    locals_ = joint_local_mats()
    parents = joint_parents()
    world: dict[int, list[float]] = {}

    def resolve(joint: int) -> list[float]:
        if joint in world:
            return world[joint]
        parent = parents[joint]
        local = locals_[joint]
        if parent is None:
            world[joint] = local
        else:
            world[joint] = mat4_mul(resolve(parent), local)
        return world[joint]

    for joint in JOINT_NODE_INDICES:
        resolve(joint)
    return world


def pack_floats(values: list[float]) -> bytes:
    return struct.pack(f"<{len(values)}f", *values)


def pack_u16(values: list[int]) -> bytes:
    return struct.pack(f"<{len(values)}H", *values)


def align4(data: bytes) -> bytes:
    pad = (4 - (len(data) % 4)) % 4
    return data + (b"\x00" * pad)


def sin_quat_x(angle: float) -> list[float]:
    half = angle * 0.5
    return [math.sin(half), 0.0, 0.0, math.cos(half)]


def sin_quat_z(angle: float) -> list[float]:
    half = angle * 0.5
    return [0.0, 0.0, math.sin(half), math.cos(half)]


def make_idle_track() -> tuple[int, list[float], list[float]]:
    times = [0.0, 1.0, 2.0]
    angles = [0.0, 0.08, 0.0]
    outputs = [v for a in angles for v in sin_quat_z(a)]
    return JOINT_SPINE, times, outputs


def make_walk_tracks() -> list[tuple[int, list[float], list[float]]]:
    times = [0.0, 0.25, 0.5, 0.75, 1.0]
    leg_l = [0.45 * math.sin(t * math.tau) for t in times]
    leg_r = [0.45 * math.sin((t + 0.5) * math.tau) for t in times]
    arm_l = [0.35 * math.sin((t + 0.5) * math.tau) for t in times]
    arm_r = [0.35 * math.sin(t * math.tau) for t in times]
    return [
        (JOINT_LEG_L, times, [v for a in leg_l for v in sin_quat_x(a)]),
        (JOINT_LEG_R, times, [v for a in leg_r for v in sin_quat_x(a)]),
        (JOINT_ARM_L, times, [v for a in arm_l for v in sin_quat_x(a)]),
        (JOINT_ARM_R, times, [v for a in arm_r for v in sin_quat_x(a)]),
    ]


def make_run_tracks() -> list[tuple[int, list[float], list[float]]]:
    times = [0.0, 0.15, 0.3, 0.45, 0.6]
    leg_l = [0.65 * math.sin(t / 0.6 * math.tau) for t in times]
    leg_r = [0.65 * math.sin((t + 0.3) / 0.6 * math.tau) for t in times]
    arm_l = [0.5 * math.sin((t + 0.3) / 0.6 * math.tau) for t in times]
    arm_r = [0.5 * math.sin(t / 0.6 * math.tau) for t in times]
    return [
        (JOINT_LEG_L, times, [v for a in leg_l for v in sin_quat_x(a)]),
        (JOINT_LEG_R, times, [v for a in leg_r for v in sin_quat_x(a)]),
        (JOINT_ARM_L, times, [v for a in arm_l for v in sin_quat_x(a)]),
        (JOINT_ARM_R, times, [v for a in arm_r for v in sin_quat_x(a)]),
    ]


def make_attack_tracks() -> list[tuple[int, list[float], list[float]]]:
    times = [0.0, 0.15, 0.35, 0.55, 0.8]
    angles = [-0.2, 0.9, 1.2, 0.4, -0.2]
    return [(JOINT_ARM_R, times, [v for a in angles for v in sin_quat_x(a)])]


def append_accessor(
    blob: bytearray,
    accessors: list[dict],
    buffer_views: list[dict],
    data: bytes,
    component_type: int,
    accessor_type: str,
    count: int,
    max_vals: list[float] | None = None,
    min_vals: list[float] | None = None,
) -> int:
    pad = (4 - (len(blob) % 4)) % 4
    blob.extend(b"\x00" * pad)
    view_index = len(buffer_views)
    buffer_views.append({"buffer": 0, "byteOffset": len(blob), "byteLength": len(data)})
    blob.extend(data)
    pad = (4 - (len(blob) % 4)) % 4
    blob.extend(b"\x00" * pad)
    accessor = {
        "bufferView": view_index,
        "componentType": component_type,
        "count": count,
        "type": accessor_type,
    }
    if max_vals is not None:
        accessor["max"] = max_vals
    if min_vals is not None:
        accessor["min"] = min_vals
    accessor_index = len(accessors)
    accessors.append(accessor)
    return accessor_index


def build_animation(name: str, tracks: list[tuple[int, list[float], list[float]]], blob: bytearray, accessors: list, buffer_views: list) -> dict:
    channels = []
    samplers = []
    for node, times, outputs in tracks:
        input_index = append_accessor(blob, accessors, buffer_views, pack_floats(times), 5126, "SCALAR", len(times), [max(times)], [min(times)])
        output_index = append_accessor(
            blob,
            accessors,
            buffer_views,
            pack_floats(outputs),
            5126,
            "VEC4",
            len(times),
            [1.0, 1.0, 1.0, 1.0],
            [-1.0, -1.0, -1.0, -1.0],
        )
        sampler_index = len(samplers)
        samplers.append({"input": input_index, "output": output_index, "interpolation": "LINEAR"})
        path = "rotation"
        channels.append({"sampler": sampler_index, "target": {"node": node, "path": path}})
    return {"name": name, "samplers": samplers, "channels": channels}


def build_gltf() -> tuple[dict, bytes]:
    verts = build_mesh_vertices()
    positions: list[float] = []
    normals: list[float] = []
    uvs: list[float] = []
    joints: list[int] = []
    weights: list[float] = []
    for v in verts:
        positions.extend(v["pos"])
        normals.extend(v["normal"])
        uvs.extend(v["uv"])
        joints.extend(v["joints"])
        weights.extend(v["weights"])

    indices: list[int] = []
    for box in range(6):
        indices.extend(cube_indices(box * 8))

    pos_min = [min(positions[0::3]), min(positions[1::3]), min(positions[2::3])]
    pos_max = [max(positions[0::3]), max(positions[1::3]), max(positions[2::3])]

    blob = bytearray()
    accessors: list[dict] = []
    buffer_views: list[dict] = []

    pos_accessor = append_accessor(
        blob, accessors, buffer_views, pack_floats(positions), 5126, "VEC3", len(verts), pos_max, pos_min
    )
    normal_accessor = append_accessor(blob, accessors, buffer_views, pack_floats(normals), 5126, "VEC3", len(verts))
    uv_accessor = append_accessor(blob, accessors, buffer_views, pack_floats(uvs), 5126, "VEC2", len(verts))
    joints_accessor = append_accessor(blob, accessors, buffer_views, pack_u16(joints), 5123, "VEC4", len(verts))
    weights_accessor = append_accessor(blob, accessors, buffer_views, pack_floats(weights), 5126, "VEC4", len(verts))
    index_accessor = append_accessor(blob, accessors, buffer_views, pack_u16(indices), 5123, "SCALAR", len(indices), [max(indices)], [0])

    ibm_values: list[float] = []
    world = joint_world_mats()
    for joint in JOINT_NODE_INDICES:
        ibm_values.extend(mat4_try_invert(world[joint]))
    ibm_accessor = append_accessor(
        blob, accessors, buffer_views, pack_floats(ibm_values), 5126, "MAT4", len(JOINT_NODE_INDICES)
    )

    normal_png = flat_normal_png()
    orm_png_bytes = orm_png(0.65, 0.0)
    normal_image_offset = len(blob)
    blob.extend(normal_png)
    orm_image_offset = len(blob)
    blob.extend(orm_png_bytes)
    normal_image_view = len(buffer_views)
    buffer_views.append(
        {"buffer": 0, "byteOffset": normal_image_offset, "byteLength": len(normal_png)}
    )
    orm_image_view = len(buffer_views)
    buffer_views.append(
        {"buffer": 0, "byteOffset": orm_image_offset, "byteLength": len(orm_png_bytes)}
    )

    animations = [
        build_animation("Idle", [make_idle_track()], blob, accessors, buffer_views),
        build_animation("Walk", make_walk_tracks(), blob, accessors, buffer_views),
        build_animation("Run", make_run_tracks(), blob, accessors, buffer_views),
        build_animation("Attack", make_attack_tracks(), blob, accessors, buffer_views),
    ]

    gltf = {
        "asset": {"version": "2.0", "generator": "generate_spark_humanoid.py"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [
            {"name": "Scene", "children": [1]},
            {"name": "SparkHumanoid", "mesh": 0, "skin": 0},
            {"name": "root", "children": [3]},
            {"name": "hips", "translation": [0.0, 0.9, 0.0], "children": [4, 8, 9]},
            {"name": "spine", "translation": [0.0, 0.2, 0.0], "children": [5, 6, 7]},
            {"name": "head", "translation": [0.0, 0.25, 0.0]},
            {"name": "arm_l", "translation": [-0.2, 0.15, 0.0]},
            {"name": "arm_r", "translation": [0.2, 0.15, 0.0]},
            {"name": "leg_l", "translation": [-0.1, -0.05, 0.0]},
            {"name": "leg_r", "translation": [0.1, -0.05, 0.0]},
        ],
        "meshes": [
            {
                "name": "SparkHumanoidMesh",
                "primitives": [
                    {
                        "attributes": {
                            "POSITION": pos_accessor,
                            "NORMAL": normal_accessor,
                            "TEXCOORD_0": uv_accessor,
                            "JOINTS_0": joints_accessor,
                            "WEIGHTS_0": weights_accessor,
                        },
                        "indices": index_accessor,
                        "material": 0,
                    }
                ],
            }
        ],
        "skins": [
            {
                "name": "SparkHumanoidSkin",
                "inverseBindMatrices": ibm_accessor,
                "joints": JOINT_NODE_INDICES,
                "skeleton": JOINT_ROOT,
            }
        ],
        "images": [
            {"mimeType": "image/png", "bufferView": normal_image_view},
            {"mimeType": "image/png", "bufferView": orm_image_view},
        ],
        "textures": [{"source": 0}, {"source": 1}],
        "samplers": [{"magFilter": 9729, "minFilter": 9729}],
        "materials": [
            {
                "name": "Body",
                "normalTexture": {"index": 0, "scale": 1.0},
                "pbrMetallicRoughness": {
                    "baseColorFactor": [0.42, 0.62, 0.92, 1.0],
                    "metallicFactor": 0.0,
                    "roughnessFactor": 0.65,
                    "metallicRoughnessTexture": {"index": 1},
                },
            }
        ],
        "animations": animations,
        "buffers": [{"byteLength": len(blob)}],
        "accessors": accessors,
        "bufferViews": buffer_views,
    }
    return gltf, bytes(blob)


def pack_glb(gltf: dict, bin_blob: bytes) -> bytes:
    json_bytes = json.dumps(gltf, separators=(",", ":")).encode("utf-8")
    json_bytes += b" " * ((4 - len(json_bytes) % 4) % 4)
    bin_blob += b"\x00" * ((4 - len(bin_blob) % 4) % 4)
    length = 12 + 8 + len(json_bytes) + 8 + len(bin_blob)
    out = bytearray()
    out.extend(struct.pack("<4sII", b"glTF", 2, length))
    out.extend(struct.pack("<I4s", len(json_bytes), b"JSON"))
    out.extend(json_bytes)
    out.extend(struct.pack("<I4s", len(bin_blob), b"BIN\x00"))
    out.extend(bin_blob)
    return bytes(out)


def main() -> None:
    gltf, bin_blob = build_gltf()
    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUT_PATH.write_bytes(pack_glb(gltf, bin_blob))
    print(f"Wrote {OUT_PATH} ({OUT_PATH.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
