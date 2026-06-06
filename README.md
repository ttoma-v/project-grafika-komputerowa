# Bathyscaphe — Deep Underwater Scene

Interactive underwater OpenGL 3.3 application (C++ / GLSL). View from a bathyscaphe interior toward a dark seabed.

## Implemented methods

| Method | Status |
|--------|--------|
| Quaternion camera control | Yes |
| Underwater skybox / cubemap | Yes |
| PBR lighting (metallic/roughness) | Yes |
| Normal mapping (TBN, sand + coral) | Yes |
| Shadow mapping (depth bias + 3×3 PCF) | Yes — `shadow_depth.*`, `pbr.frag`, `main.cpp` |
| Parallel Transport Frames (kelp spline) | Yes — `src/PTF.cpp`, kelp path in `main.cpp` |

## Build

```bash
cmake -S . -B build
cmake --build build --config Release
```

Windows:

```bash
build\Release\underwater_bathy.exe
```

On first configure CMake fetches GLFW and GLM via FetchContent (internet required).

## Controls

| Key | Action |
|-----|--------|
| W / A / S / D | Move forward / left / back / right |
| Space | Move up |
| Left Shift | Move down |
| Mouse | Look around (quaternion camera) |
| Esc | Quit |

## Scene notes

Kelp is placed in two ways:

1. **PTF spline mesh** — three curved kelp stalks built with `Geometry::makeKelpAlongSpline()` (vertices swept along a Catmull-Rom path using Parallel Transport Frames). Look toward negative Z to see the rising S-curves.
2. **Scattered instances** — 48 vertical `makeKelpSegment()` blades with seeded random placement (same layout as `ref_project`).

Kelp uses a neutral albedo texture tinted per-instance (`materialAlbedoTint`) and vertex sway in `pbr.vert` (`useKelpSway`, `kelpTime`, `kelpSwayPhase`, `kelpSwayAxis`).
