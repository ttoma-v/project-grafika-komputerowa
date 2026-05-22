# Bathyscaphe — Deep Underwater Scene

Interactive underwater OpenGL 3.3 application (C++ / GLSL). View from a bathyscaphe interior toward a dark seabed.

## Commit 1 — implemented methods

| Method | Status |
|--------|--------|
| Quaternion camera control | Yes |
| Underwater skybox / cubemap | Yes |

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
