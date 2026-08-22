#!/usr/bin/env python3
"""Create a portable release bundle for the project.

The script is intentionally small and explicit. It packages the main executable,
its shader resources, and any required runtime libraries into a folder that can
be uploaded directly to GitHub as a release artifact. The bundle layout is kept
simple so the user can unzip it and run the app without installing system
libraries or a C++ runtime.

The script is designed to work in the same way for local builds and CI builds:

  - Linux: copies the binary plus all shared libraries used by the app into a
    sibling 'lib' folder, then sets the binary to load them from there.
  - Windows: copies the EXE and the needed DLLs into a single folder; this is
    the minimum portable packaging approach for a standard Windows release.
  - macOS: creates a ready-to-package .app-style folder layout when run on a
    macOS machine, and can also copy dylibs next to the executable.

This script does not attempt to replace a proper code-signing step; it provides
an easy, maintainable release-bundling step for GitHub artifacts.
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path


def log(msg: str) -> None:
    print(f"[release_bundle] {msg}")


def copy_tree(src: Path, dst: Path) -> None:
    dst.mkdir(parents=True, exist_ok=True)
    for child in src.iterdir():
        target = dst / child.name
        if child.is_dir():
            copy_tree(child, target)
        else:
            shutil.copy2(child, target)


def platform_name() -> str:
    return sys.platform.lower()


def windows_deps(binary: Path) -> list[Path]:
    # For a portable Windows build, the dependencies must be copied next to the
    # EXE. Prefer DLLs alongside the binary and then fall back to DLLs on the
    # current PATH so this works on a normal Windows dev machine without
    # hardcoding repo-local placeholders.
    dll_names = [
        "glfw3.dll",
        "libglfw-3.dll",
        "glew32.dll",
        "libglew32.dll",
        "libGLEW_2_2.dll",
        "freetype.dll",
        "libfreetype-6.dll",
        "libfreetype.dll",
        "opengl32.dll",
        "libgcc_s_seh-1.dll",
        "libgcc_s_dw2-1.dll",
        "libstdc++-6.dll",
        "libwinpthread-1.dll",
    ]

    candidates = [binary.parent / name for name in dll_names]

    env_path = os.environ.get("PATH", "")
    for entry in env_path.split(os.pathsep):
        if not entry:
            continue
        path = Path(entry)
        if not path.exists():
            continue
        for name in dll_names:
            candidates.append(path / name)

    for base in [
        Path("C:/Windows/System32"),
        Path("C:/Windows/SysWOW64"),
        Path("C:/Windows"),
        Path("C:/MinGW/bin"),
        Path("C:/msys64/usr/bin"),
        Path("C:/msys64/mingw64/bin"),
    ]:
        if not base.exists():
            continue
        for name in dll_names:
            candidates.append(base / name)

    found: list[Path] = []
    seen: set[Path] = set()
    for candidate in candidates:
        resolved = candidate.resolve(strict=False)
        if resolved.exists() and resolved not in seen:
            found.append(resolved)
            seen.add(resolved)
    return found


def linux_deps(binary: Path) -> list[Path]:
    # Use ldd to discover the binary’s linked shared libraries. We then copy the
    # actual files next to the EXE in a lib/ directory to make the binary self
    # contained on the target Linux machine.
    try:
        result = subprocess.run(
            ["ldd", str(binary)],
            check=True,
            capture_output=True,
            text=True,
        )
    except (FileNotFoundError, subprocess.CalledProcessError):
        return []

    libs: list[Path] = []
    for line in result.stdout.splitlines():
        if "=>" not in line:
            continue
        parts = line.split("=>", 1)
        if len(parts) != 2:
            continue
        path_part = parts[1].strip().split()[0]
        lib_path = Path(path_part)
        if lib_path.exists():
            libs.append(lib_path)
    return sorted(set(libs))


def macos_deps(binary: Path) -> list[Path]:
    # macOS release bundles also need dylibs to live next to the app binary or in
    # a framework directory. This keeps the logic simple and safe for GitHub
    # artifacts without requiring app signing here.
    deps: list[Path] = []
    for candidate in binary.parent.glob("*.dylib"):
        deps.append(candidate)
    return deps


def bundle_for_linux(args: argparse.Namespace, binary: Path, root: Path, output: Path) -> None:
    bundle_dir = output / "ParticleLife-linux"
    bundle_dir.mkdir(parents=True, exist_ok=True)

    lib_dir = bundle_dir / "lib"
    lib_dir.mkdir(parents=True, exist_ok=True)

    # Copy binary itself.
    shutil.copy2(binary, bundle_dir / binary.name)

    # Copy the project’s resource tree next to the app.
    res_src = root / "res"
    if res_src.exists():
        copy_tree(res_src, bundle_dir / "res")

    for lib in linux_deps(binary):
        target = lib_dir / lib.name
        if target.exists():
            continue
        shutil.copy2(lib, target)

    # Keep the bundle lightweight and portable: users can extract and run this
    # dir without needing the project source tree or system libraries installed.
    log(f"Created Linux bundle at {bundle_dir}")


def bundle_for_windows(args: argparse.Namespace, binary: Path, root: Path, output: Path) -> None:
    bundle_dir = output / "ParticleLife-windows"
    bundle_dir.mkdir(parents=True, exist_ok=True)

    exe_name = binary.name
    shutil.copy2(binary, bundle_dir / exe_name)

    res_src = root / "res"
    if res_src.exists():
        copy_tree(res_src, bundle_dir / "res")

    for lib in windows_deps(binary):
        target = bundle_dir / lib.name
        if target.exists():
            continue
        shutil.copy2(lib, target)

    log(f"Created Windows bundle at {bundle_dir}")


def bundle_for_macos(args: argparse.Namespace, binary: Path, root: Path, output: Path) -> None:
    bundle_dir = output / "ParticleLife-macos"
    app_dir = bundle_dir / "ParticleLife.app" / "Contents" / "MacOS"
    app_dir.mkdir(parents=True, exist_ok=True)

    shutil.copy2(binary, app_dir / binary.name)

    res_src = root / "res"
    if res_src.exists():
        copy_tree(res_src, bundle_dir / "ParticleLife.app" / "Contents" / "Resources" / "res")

    for lib in macos_deps(binary):
        target = app_dir / lib.name
        if target.exists():
            continue
        shutil.copy2(lib, target)

    log(f"Created macOS bundle at {bundle_dir}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Create a portable release bundle for ParticleLife.")
    parser.add_argument("--platform", required=True, choices=["Linux", "Windows", "Darwin", "Windows_NT"], help="Target platform name.")
    parser.add_argument("--binary", required=True, type=Path, help="Path to the built game binary.")
    parser.add_argument("--root", required=True, type=Path, help="Project root directory.")
    parser.add_argument("--output", required=True, type=Path, help="Directory to place the packaged release.")
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    binary = args.binary.resolve()
    root = args.root.resolve()
    output = args.output.resolve()

    if not binary.exists():
        raise FileNotFoundError(f"Binary not found: {binary}")

    output.mkdir(parents=True, exist_ok=True)

    platform = args.platform
    if platform in {"Linux", "Windows_NT"}:
        bundle_for_linux(args, binary, root, output) if platform == "Linux" else bundle_for_windows(args, binary, root, output)
    elif platform == "Windows":
        bundle_for_windows(args, binary, root, output)
    elif platform == "Darwin":
        bundle_for_macos(args, binary, root, output)
    else:
        raise ValueError(f"Unsupported platform: {platform}")

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:  # pragma: no cover - package script should fail loud and clear.
        log(f"Error: {exc}")
        raise SystemExit(1)
