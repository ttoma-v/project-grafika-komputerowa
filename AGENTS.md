# AGENTS.md

## Cursor Cloud specific instructions

This repo is a single C++17 / OpenGL 3.3 desktop application (`underwater_bathy`) built
with CMake. There is no backend, database, web service, or automated test suite — the
"product" is the GUI executable itself. Standard build/run/controls are documented in
`README.md`; only the non-obvious cloud caveats are captured here.

### Services
- `underwater_bathy` — the only service. An interactive GLFW/OpenGL window that renders
  the underwater scene. It runs a render loop until you press `Esc` (it does not exit on
  its own), so start it in the background when you need to capture output.

### Build / lint / test
- Build (see `README.md`): `cmake -S . -B build && cmake --build build --config Release -j"$(nproc)"`.
  The CMake configure step git-clones the dependencies (GLFW 3.4, GLM, cgltf, stb) via
  `FetchContent`, so the first configure needs network access.
- Lint: there is no separate linter. Compiler warnings act as lint (`-Wall -Wextra -Wpedantic`,
  set in `CMakeLists.txt`). One pre-existing harmless warning in `src/Geometry.cpp`.
- Tests: none exist. End-to-end verification = build + run + confirm the console prints
  `Shaders OK` and the scene renders.

### Running headless (no GPU / no display)
The cloud VM has no GPU or physical display, so you must use a virtual X display plus Mesa
software rendering (llvmpipe, which provides OpenGL 4.5 core — well above the required 3.3):

```bash
# Start a virtual display once per session
Xvfb :99 -screen 0 1280x720x24 -ac +extension GLX +render -noreset &

# Run the app (must run from build/ so the copied assets/ are found)
cd build
DISPLAY=:99 LIBGL_ALWAYS_SOFTWARE=1 ./underwater_bathy
```

Notes:
- The `XDG_RUNTIME_DIR is invalid or not set` message at startup is harmless; GLFW falls
  back to the X11 backend under Xvfb and the app runs fine.
- Capture a frame: `DISPLAY=:99 ffmpeg -y -f x11grab -video_size 1280x720 -i :99 -frames:v 1 out.png`.
- Send input for interactive testing with `xdotool` (e.g. `xdotool key --window "$(xdotool search --name Bathyscaphe|head -1)" f` to toggle headlights). Controls are listed in `README.md`.

### Toolchain caveat
The default C/C++ compiler (`cc`/`c++`) is Clang, which selects the gcc-14 toolchain for
its standard library, so `libstdc++-14-dev` must be present (it is installed in the VM
snapshot during setup). Without it, linking fails with `cannot find -lstdc++` even though
`g++`/`gcc-13` work directly.
