# Release packaging notes

This project now includes a simple CMake release target and a helper script that packages the binary into a portable folder for upload to GitHub.

## Linux release

From a Linux machine:

```bash
cmake -S . -B build/release-linux -DCMAKE_BUILD_TYPE=Release
cmake --build build/release-linux --target ParticleLife -j$(nproc)
cmake --build build/release-linux --target release_bundle -j1
```

This produces a folder under `build/release-linux/release/` containing a portable Linux bundle with the executable and bundled libraries.

## Windows release

From a Windows machine or MinGW cross toolchain:

```bash
cmake -S . -B build/release-windows -DCMAKE_BUILD_TYPE=Release
cmake --build build/release-windows --target ParticleLife -j4
cmake --build build/release-windows --target release_bundle -j1
```

The bundle will be created in `build/release-windows/release/` and contains the EXE plus runtime DLLs in the same folder.

## macOS release

This must be done on a macOS machine with Xcode and the Apple SDK installed:

```bash
cmake -S . -B build/release-macos -DCMAKE_BUILD_TYPE=Release
cmake --build build/release-macos --target ParticleLife -j$(sysctl -n hw.ncpu)
cmake --build build/release-macos --target release_bundle -j1
```

The resulting bundle is created in `build/release-macos/release/`.

## Important note

These bundles are intended for GitHub artifact upload and are easier to redistribute than a raw local build. They are not a replacement for code signing on macOS or a full installer for Windows. A dependency-free single-binary release still requires platform-specific bundling and signing steps when distributing to end users.
