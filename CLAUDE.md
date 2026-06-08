# CrealityPrint — Build Instructions (Linux)

This file is the authoritative guide for building CrealityPrint from source on Ubuntu/Debian.
The active development branch is **`wayland-gtk3-investigation`**, which adds native Wayland
support and several GTK3 fixes on top of the upstream Creality release.

---

## Requirements

- Ubuntu 22.04 (x86-64) — tested platform. 24.04 should also work.
- ~10 GB free disk space, ~10 GB free RAM during linking.
- Internet access for the first deps build (fetches third-party source tarballs).

---

## Step 1 — Install system dependencies

Run once, requires sudo:

```bash
sudo ./BuildLinux.sh -u
```

This reads `linux.d/debian` and installs all required `apt` packages including GTK3,
WebKitGTK, OpenGL, ninja, cmake, etc.

---

## Step 2 — Build third-party dependencies

Run once. Takes 20–40 minutes on first run; subsequent runs are incremental.

```bash
./BuildLinux.sh -d
```

This configures and builds all bundled deps (wxWidgets, Boost, OpenSSL, OCCT, etc.) into
`deps/build/destdir/`. The wxWidgets build automatically applies the patch at
`deps/wxWidgets/0001-wayland-set-readyToDraw-at-surface-creation.patch`, which is required
for the native Wayland GL canvas to work correctly.

---

## Step 3 — Configure cmake

Run once after a clean checkout, or whenever cmake flags need changing:

```bash
cmake -S . -B build -G Ninja \
    -DCMAKE_PREFIX_PATH="$(pwd)/deps/build/destdir/usr/local" \
    -DSLIC3R_STATIC=1 \
    -DSLIC3R_GTK=3 \
    -DORCA_TOOLS=ON \
    -DGENERATE_ORCA_HEADER=0 \
    -DENABLE_BREAKPAD=ON \
    -DBBL_RELEASE_TO_PUBLIC=1 \
    -DBBL_INTERNAL_TESTING=0 \
    -DUPDATE_ONLINE_MACHINES=1 \
    -DPROJECT_VERSION_EXTRA=Release \
    -DCREALITYPRINT_VERSION=7.1.1.338337 \
    -DCMAKE_C_COMPILER_LAUNCHER=ccache \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
```

**`-DPROJECT_VERSION_EXTRA=Release`** — must be `Release` (not `Alpha`) so the app reads
settings from `~/.config/Creality/Creality Print/7.0` instead of `7.0 Alpha`.

**`-DCREALITYPRINT_VERSION=7.1.1.338337`** — build identifier. Both flags follow the same
mechanism Creality uses in their official CI (`scripts/BuildLinux_Package.sh`); the fallback
values in `version.inc` / `cmake/BuildInfoUtil.cmake` are never reached when these are
passed on the command line.

These values are written to `build/CMakeCache.txt` and persist for all subsequent builds —
you do not need to pass them again unless you wipe the build directory.

---

## Step 4 — Build

```bash
cmake --build build --target CrealityPrint -- -j2
```

Use `-j2` to avoid OOM during linking. ccache makes incremental rebuilds fast regardless of
job count. The binary is produced at `build/src/CrealityPrint`.

---

## Step 5 — Run directly (development)

```bash
./build/src/CrealityPrint
```

The binary finds `resources/` relative to itself; no install step is needed for local use.

---

## Step 6 — Build AppImage (distribution)

**Important:** the packaging script must be run from the `build/` directory, not `build/src/`.
Running it from the wrong directory causes it to overwrite the ELF binary with a shell script.

```bash
cd build && bash src/BuildLinuxImage.sh -i
```

The AppImage is produced at `build/CrealityPrint-V7.1.1.338337-x86_64-Release.AppImage`.

If the binary is accidentally overwritten (symptom: `No such file or directory` with a
doubled path like `/bin/bin/CrealityPrint`), rebuild it and verify before repackaging:

```bash
rm build/src/CrealityPrint
cmake --build build --target CrealityPrint -- -j$(nproc)
file build/src/CrealityPrint   # must say ELF, not shell script
```

---

## Incremental rebuilds

After editing source files, just run:

```bash
cmake --build build --target CrealityPrint -- -j$(nproc)
```

cmake tracks dependencies; only changed translation units are recompiled.

---

## Key source locations

| What | Where |
|---|---|
| Native Wayland GL canvas fix | `deps/wxWidgets/0001-wayland-set-readyToDraw-at-surface-creation.patch` |
| Tab-switching transparent overlay | `src/slic3r/GUI/GLCanvas3D.cpp` — `clear_framebuffer()` |
| Tab-switching wiring | `src/slic3r/GUI/Plater.cpp` — `set_current_panel()` |
| Send to LAN Printer dialog | `src/slic3r/GUI/print_manage/App/SendToPrinter.cpp` |
| Config directory path logic | `src/libslic3r/utils.cpp` — `set_data_dir()` |
| Version / release flags | `cmake/BuildInfoUtil.cmake`, `version.inc` (defaults only) |
