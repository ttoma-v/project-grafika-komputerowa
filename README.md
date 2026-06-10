# Bathyscaphe — Deep Underwater Scene

Interactive underwater OpenGL 3.3 application (C++ / GLSL). View from a bathyscaphe interior toward a dark seabed.

## Implemented methods

| Method | Status |
|--------|--------|
| Quaternion camera control | Yes — `src/Camera.cpp` |
| Underwater skybox / cubemap | Yes — `Texture.cpp`, `skybox.*` |
| PBR lighting (metallic/roughness) | Yes — `assets/shaders/pbr.frag` |
| Normal mapping (TBN, sand + coral) | Yes — `pbr.vert`, `pbr.frag`, `Texture.cpp` |
| Shadow mapping (depth bias + 3×3 PCF) | Yes — `shadow_depth.*`, `pbr.frag`, `main.cpp` |
| Parallel Transport Frames (kelp spline) | Yes — `src/PTF.cpp`, kelp path in `main.cpp` |
| **B13** Moving point lights / headlights | Yes — camera-mounted headlights + lure light in `main.cpp` |

## Build

```bash
cmake -S . -B build
cmake --build build --config Release
```

Windows:

```bash
build\Release\underwater_bathy.exe
```

On first configure CMake fetches GLFW, GLM, cgltf, and stb via FetchContent (internet required).

### macOS

```bash
cmake -S . -B build
cmake --build build --config Release
./build/underwater_bathy
```

Run from the build directory so `assets/` (shaders + GLB models) is found next to the binary.

## Controls

| Key | Action |
|-----|--------|
| W / A / S / D | Move forward / left / back / right |
| Space | Move up |
| Left Shift | Move down |
| Mouse | Look around (quaternion camera) |
| **F** | Toggle bathyscaphe headlights on/off |
| Esc | Quit |

## Scene notes

### Kelp (PTF — commit 4)

1. **PTF spline mesh** — three curved kelp stalks built with `Geometry::makeKelpAlongSpline()` along Catmull-Rom paths using Parallel Transport Frames.
2. **Scattered instances** — 48 vertical `makeKelpSegment()` blades with seeded random placement.

Kelp sways via `useKelpSway` in `pbr.vert` / `shadow_depth.vert`.

### Fish and props (commit 5)

- **Anglerfish** — swims a circular path around `(0, 1.6, -20)`, radius 8; skeletal swim animation; glowing lure drives the 3rd point light.
- **Piranha** — faster circular path near `(12, 3, -12)`.
- **Static props** — chest, anchor, barrel, urchin on the seabed (GLB models via `ModelLoader.cpp`).

### Headlights (B13)

Two warm point lights are parented to the camera (forward offset, left/right) with subtle sine sway. Press **F** to toggle them. A third teal light follows the anglerfish lure.

Shadow map is rendered each frame from the left headlight direction; skinned fish and static props are included in the depth pass.
