#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import platform
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Mapping


COMPONENT_ID = "athrill-target-v850e2m"
COMPONENT_VERSION = "1.0.2"
VALID_BUILD_TYPES = {"Debug", "Release", "RelWithDebInfo", "MinSizeRel"}
DEFAULT_CONFIG: dict[str, Any] = {
    "version": 1,
    "build": {"type": "Release", "dir": ".hako/build", "parallel": 0},
    "features": {"exdev": False, "mros": False, "vdev": False},
    "validation": {"tests": True},
    "paths": {"athrill_root": "../athrill", "vcpkg_root": ""},
}


class ConfigError(RuntimeError):
    pass


def _strip_comment(text: str) -> str:
    quote: str | None = None
    escaped = False
    out: list[str] = []
    for ch in text:
        if escaped:
            out.append(ch)
            escaped = False
        elif ch == "\\" and quote:
            out.append(ch)
            escaped = True
        elif ch in {"'", '"'}:
            quote = None if quote == ch else ch if quote is None else quote
            out.append(ch)
        elif ch == "#" and quote is None:
            break
        else:
            out.append(ch)
    return "".join(out).rstrip()


def _parse_scalar(text: str) -> Any:
    value = text.strip()
    if not value:
        return {}
    lowered = value.lower()
    if lowered == "true":
        return True
    if lowered == "false":
        return False
    if lowered in {"null", "~"}:
        return None
    if value.startswith(("'", '"')):
        if len(value) < 2 or value[-1] != value[0]:
            raise ConfigError(f"unterminated quoted scalar: {value}")
        if value[0] == '"':
            try:
                return json.loads(value)
            except json.JSONDecodeError as exc:
                raise ConfigError(f"invalid quoted scalar: {value}") from exc
        return value[1:-1].replace("''", "'")
    try:
        return int(value)
    except ValueError:
        return value


def load_simple_yaml(path: Path) -> dict[str, Any]:
    root: dict[str, Any] = {}
    stack: list[tuple[int, dict[str, Any]]] = [(-1, root)]
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as exc:
        raise ConfigError(f"cannot read build manifest: {path}: {exc}") from exc
    for lineno, raw in enumerate(lines, 1):
        if "\t" in raw:
            raise ConfigError(f"{path}:{lineno}: tabs are not allowed")
        line = _strip_comment(raw)
        if not line.strip():
            continue
        stripped = line.lstrip(" ")
        indent = len(line) - len(stripped)
        if stripped.startswith("-"):
            raise ConfigError(f"{path}:{lineno}: sequences are not supported")
        if ":" not in stripped:
            raise ConfigError(f"{path}:{lineno}: expected 'key: value'")
        key, raw_value = stripped.split(":", 1)
        key = key.strip()
        while stack and indent <= stack[-1][0]:
            stack.pop()
        if not key or not stack:
            raise ConfigError(f"{path}:{lineno}: invalid mapping")
        parent = stack[-1][1]
        if key in parent:
            raise ConfigError(f"{path}:{lineno}: duplicate key: {key}")
        value = _parse_scalar(raw_value)
        parent[key] = value
        if isinstance(value, dict):
            stack.append((indent, value))
    return root


def _merge_known(defaults: Mapping[str, Any], overrides: Mapping[str, Any], prefix: str = "") -> dict[str, Any]:
    unknown = sorted(set(overrides) - set(defaults))
    if unknown:
        raise ConfigError(f"unknown key(s) under {prefix or 'root'}: {', '.join(unknown)}")
    result: dict[str, Any] = {}
    for key, default in defaults.items():
        value = overrides.get(key, default)
        path = f"{prefix}.{key}" if prefix else key
        if isinstance(default, Mapping):
            if not isinstance(value, Mapping):
                raise ConfigError(f"{path} must be a mapping")
            result[key] = _merge_known(default, value, path)
        else:
            result[key] = value
    return result


def resolve_config(raw: Mapping[str, Any]) -> dict[str, Any]:
    cfg = _merge_known(DEFAULT_CONFIG, raw)
    if cfg["version"] != 1:
        raise ConfigError("version must be 1")
    if cfg["build"]["type"] not in VALID_BUILD_TYPES:
        raise ConfigError("build.type must be Debug, Release, RelWithDebInfo, or MinSizeRel")
    if not isinstance(cfg["build"]["dir"], str) or not cfg["build"]["dir"].strip():
        raise ConfigError("build.dir must be a non-empty string")
    parallel = cfg["build"]["parallel"]
    if not isinstance(parallel, int) or isinstance(parallel, bool) or parallel < 0:
        raise ConfigError("build.parallel must be a non-negative integer")
    for key in ("exdev", "mros"):
        if cfg["features"][key] not in {True, False, "auto"}:
            raise ConfigError(f"features.{key} must be auto, true, or false")
    for section, keys in {"features": ("vdev",), "validation": ("tests",)}.items():
        for key in keys:
            if not isinstance(cfg[section][key], bool):
                raise ConfigError(f"{section}.{key} must be true or false")
    for key in ("athrill_root", "vcpkg_root"):
        if not isinstance(cfg["paths"][key], str):
            raise ConfigError(f"paths.{key} must be a string")
    return cfg


def _host_platform() -> tuple[str, str]:
    os_name = "windows" if sys.platform == "win32" else "macos" if sys.platform == "darwin" else "linux"
    machine = platform.machine().lower()
    arch = {"amd64": "x64", "x86_64": "x64", "arm64": "arm64", "aarch64": "arm64"}.get(machine, machine or "unknown")
    return os_name, arch


def _resolve_path(value: str, repo_root: Path) -> Path:
    path = Path(value).expanduser()
    return (repo_root / path).resolve() if not path.is_absolute() else path.resolve()


def _find_vcpkg_root(cfg: Mapping[str, Any], repo_root: Path) -> Path | None:
    candidates = [cfg["paths"]["vcpkg_root"], os.environ.get("VCPKG_ROOT", "")]
    if sys.platform == "win32":
        candidates += [r"C:\project\vcpkg", str(repo_root.parent / "vcpkg")]
    for value in candidates:
        if not value:
            continue
        path = _resolve_path(value, repo_root)
        if (path / "scripts" / "buildsystems" / "vcpkg.cmake").is_file():
            return path
    return None


def _find_vsdevcmd() -> Path | None:
    install = os.environ.get("VSINSTALLDIR")
    if install:
        candidate = Path(install) / "Common7" / "Tools" / "VsDevCmd.bat"
        if candidate.is_file():
            return candidate
    roots = [os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"), os.environ.get("ProgramFiles", r"C:\Program Files")]
    for root in roots:
        vswhere = Path(root) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
        if vswhere.is_file():
            result = subprocess.run([str(vswhere), "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], capture_output=True, text=True, check=False)
            if result.returncode == 0 and result.stdout.strip():
                candidate = Path(result.stdout.strip()) / "Common7" / "Tools" / "VsDevCmd.bat"
                if candidate.is_file():
                    return candidate
    return None


@dataclass
class BuildContext:
    repo_root: Path
    manifest: Path
    cfg: dict[str, Any]
    build_dir: Path
    athrill_root: Path
    vcpkg_root: Path | None
    platform_name: str
    arch: str
    vsdevcmd: Path | None
    dry_run: bool


def create_context(manifest: Path, repo_root: Path, dry_run: bool = False) -> BuildContext:
    cfg = resolve_config(load_simple_yaml(manifest))
    platform_name, arch = _host_platform()
    for feature in ("exdev", "mros"):
        if cfg["features"][feature] == "auto":
            cfg["features"][feature] = platform_name != "windows"
    if platform_name == "windows" and any(cfg["features"].values()):
        raise ConfigError("Windows currently requires features.exdev/mros/vdev=false")
    return BuildContext(
        repo_root=repo_root,
        manifest=manifest,
        cfg=cfg,
        build_dir=_resolve_path(cfg["build"]["dir"], repo_root),
        athrill_root=_resolve_path(cfg["paths"]["athrill_root"], repo_root),
        vcpkg_root=_find_vcpkg_root(cfg, repo_root),
        platform_name=platform_name,
        arch=arch,
        vsdevcmd=_find_vsdevcmd() if platform_name == "windows" else None,
        dry_run=dry_run,
    )


def _command_path(path: Path, repo_root: Path) -> str:
    try:
        return os.path.relpath(path, repo_root)
    except ValueError:
        return str(path)


def _run(ctx: BuildContext, command: list[str]) -> None:
    print("+", subprocess.list2cmdline(command) if ctx.platform_name == "windows" else " ".join(command))
    if ctx.dry_run:
        return
    if ctx.platform_name != "windows":
        subprocess.run(command, cwd=ctx.repo_root, check=True)
        return
    if not ctx.vsdevcmd:
        raise ConfigError("Visual Studio C++ developer environment was not found")
    if not ctx.vcpkg_root:
        raise ConfigError("vcpkg was not found; set paths.vcpkg_root or VCPKG_ROOT")
    native = subprocess.list2cmdline(command)
    script = (
        f'call "{ctx.vsdevcmd}" -arch=x64 -host_arch=x64'
        f' && pushd "{ctx.repo_root}"'
        f' && set "VCPKG_ROOT={ctx.vcpkg_root}"'
        f" && {native}"
    )
    # Pass a raw command line: list2cmdline escapes embedded quotes with
    # backslashes, but cmd.exe does not use backslash as its quote escape.
    subprocess.run("cmd.exe /d /c " + script, cwd=os.environ.get("SystemDrive", "C:") + "\\", check=True)


def _cmake_bool(value: bool) -> str:
    return "ON" if value else "OFF"


def configure(ctx: BuildContext) -> None:
    command = [
        "cmake", "--fresh", "-S", ".", "-B", _command_path(ctx.build_dir, ctx.repo_root), "-G", "Ninja",
        f'-DCMAKE_BUILD_TYPE={ctx.cfg["build"]["type"]}',
        f'-DATHRILL_SOURCE_DIR={_command_path(ctx.athrill_root, ctx.repo_root)}',
        f'-DATHRILL_ENABLE_EXDEV={_cmake_bool(ctx.cfg["features"]["exdev"])}',
        f'-DATHRILL_ENABLE_MROS={_cmake_bool(ctx.cfg["features"]["mros"])}',
        f'-DATHRILL_ENABLE_VDEV={_cmake_bool(ctx.cfg["features"]["vdev"])}',
        f'-DBUILD_TESTING={_cmake_bool(ctx.cfg["validation"]["tests"])}',
    ]
    if ctx.platform_name == "windows":
        if not ctx.vcpkg_root:
            raise ConfigError("vcpkg was not found; set paths.vcpkg_root or VCPKG_ROOT")
        command += [f'-DCMAKE_TOOLCHAIN_FILE={ctx.vcpkg_root / "scripts" / "buildsystems" / "vcpkg.cmake"}', "-DVCPKG_TARGET_TRIPLET=x64-windows-static"]
    _run(ctx, command)
    write_resolved(ctx)


def build(ctx: BuildContext) -> None:
    configure(ctx)
    command = ["cmake", "--build", _command_path(ctx.build_dir, ctx.repo_root), "--config", ctx.cfg["build"]["type"]]
    if ctx.cfg["build"]["parallel"]:
        command += ["--parallel", str(ctx.cfg["build"]["parallel"])]
    _run(ctx, command)


def test(ctx: BuildContext) -> None:
    if not ctx.cfg["validation"]["tests"]:
        print("SKIP validation.tests=false")
        return
    _run(ctx, ["ctest", "--test-dir", _command_path(ctx.build_dir, ctx.repo_root), "-C", ctx.cfg["build"]["type"], "--output-on-failure"])


def smoke(ctx: BuildContext) -> None:
    test(ctx)


def _yaml_scalar(value: Any) -> str:
    if isinstance(value, bool):
        return str(value).lower()
    if isinstance(value, int):
        return str(value)
    if value is None:
        return "null"
    return json.dumps(str(value), ensure_ascii=False)


def _dump_yaml(data: Mapping[str, Any], indent: int = 0) -> str:
    lines: list[str] = []
    prefix = " " * indent
    for key, value in data.items():
        if isinstance(value, Mapping):
            lines.append(f"{prefix}{key}:")
            lines.append(_dump_yaml(value, indent + 2).rstrip())
        else:
            lines.append(f"{prefix}{key}: {_yaml_scalar(value)}")
    return "\n".join(lines) + "\n"


def _resolved_record(ctx: BuildContext) -> dict[str, Any]:
    return {
        "version": 1,
        "component": COMPONENT_ID,
        "source_manifest": str(ctx.manifest),
        "platform": {"os": ctx.platform_name, "architecture": ctx.arch},
        "build": {**ctx.cfg["build"], "dir": str(ctx.build_dir)},
        "features": ctx.cfg["features"],
        "validation": ctx.cfg["validation"],
        "paths": {"athrill_root": str(ctx.athrill_root), "vcpkg_root": str(ctx.vcpkg_root or "")},
    }


def write_resolved(ctx: BuildContext) -> Path:
    path = ctx.repo_root / ".hako" / "resolved-build.yaml"
    if not ctx.dry_run:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(_dump_yaml(_resolved_record(ctx)), encoding="utf-8")
    return path


def _git_revision(repo: Path) -> str:
    result = subprocess.run(
        ["git", "-c", f"safe.directory={repo}", "-C", str(repo), "rev-parse", "HEAD"],
        capture_output=True,
        text=True,
        check=False,
    )
    return result.stdout.strip() if result.returncode == 0 else "unknown"


def write_receipt(ctx: BuildContext, install_dir: Path) -> Path:
    executable = Path("bin") / ("athrill2.exe" if ctx.platform_name == "windows" else "athrill2")
    if not (install_dir / executable).is_file():
        raise ConfigError(f"installed executable not found: {install_dir / executable}")
    receipt_root = install_dir / "share" / "hakoniwa" / "receipts"
    resolved_relative = Path("share/hakoniwa/receipts/resolved") / f"{COMPONENT_ID}.yaml"
    receipt = receipt_root / f"{COMPONENT_ID}.yaml"
    capabilities = {
        "target_v850e2m": True,
        "usage_smoke": ctx.cfg["validation"]["tests"],
        **ctx.cfg["features"],
    }
    lines = [
        "schema_version: 1",
        "component:",
        f"  id: {COMPONENT_ID}",
        f"  version: {COMPONENT_VERSION}",
        f"  source_revision: {_yaml_scalar(_git_revision(ctx.repo_root))}",
        "platform:",
        f"  os: {ctx.platform_name}",
        f"  architecture: {ctx.arch}",
        f"  toolchain: {'msvc' if ctx.platform_name == 'windows' else 'host-default'}",
        "install:",
        f"  prefix: {_yaml_scalar(install_dir)}",
        "capabilities:",
    ]
    for key, value in capabilities.items():
        lines.append(f"  {key}: {_yaml_scalar(value)}")
    lines += [
        "build_limits: {}",
        "dependencies: {}",
        "artifacts:",
        f"  - path: {_yaml_scalar(executable.as_posix())}",
        "    kind: executable",
        f"resolved_manifest: {_yaml_scalar(resolved_relative.as_posix())}",
    ]
    if not ctx.dry_run:
        receipt_root.mkdir(parents=True, exist_ok=True)
        stored = install_dir / resolved_relative
        stored.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(write_resolved(ctx), stored)
        receipt.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return receipt


def install(ctx: BuildContext, install_dir: Path) -> None:
    _run(ctx, ["cmake", "--install", _command_path(ctx.build_dir, ctx.repo_root), "--config", ctx.cfg["build"]["type"], "--prefix", _command_path(install_dir, ctx.repo_root)])
    print(f"Receipt: {write_receipt(ctx, install_dir)}")


def doctor(ctx: BuildContext) -> bool:
    checks = {
        "cmake": shutil.which("cmake") is not None,
        "ninja": shutil.which("ninja") is not None,
        "athrill common CMake": (ctx.athrill_root / "cmake" / "AthrillCore.cmake").is_file(),
    }
    if ctx.platform_name == "windows":
        checks["Visual Studio C++"] = ctx.vsdevcmd is not None
        checks["vcpkg"] = ctx.vcpkg_root is not None
    for name, ok in checks.items():
        print(f"{'OK' if ok else 'NG'} {name}")
    print(f"INFO platform={ctx.platform_name}/{ctx.arch}")
    print(f"INFO build_dir={ctx.build_dir}")
    print(f"INFO athrill_root={ctx.athrill_root}")
    if ctx.vcpkg_root:
        print(f"INFO vcpkg_root={ctx.vcpkg_root}")
    if ctx.platform_name == "windows" and str(ctx.repo_root).startswith("\\\\"):
        print("INFO UNC workspace detected; hako.py will map it with cmd pushd")
    return all(checks.values())


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Business Pack build entry point for Athrill V850E2M")
    parser.add_argument("--config", help="build manifest (default: repository root/hakoniwa-build.yaml)")
    parser.add_argument("--install-dir", help="install prefix (required for install)")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("operation", choices=("doctor", "configure", "build", "test", "install", "smoke"))
    args = parser.parse_args(argv)
    repo_root = Path(__file__).resolve().parents[1]
    manifest = Path(args.config).resolve() if args.config else repo_root / "hakoniwa-build.yaml"
    try:
        ctx = create_context(manifest, repo_root, args.dry_run)
        if args.operation == "doctor":
            return 0 if doctor(ctx) else 1
        if args.operation == "configure":
            configure(ctx)
        elif args.operation == "build":
            build(ctx)
        elif args.operation == "test":
            test(ctx)
        elif args.operation == "smoke":
            smoke(ctx)
        elif args.operation == "install":
            if not args.install_dir:
                raise ConfigError("install requires --install-dir")
            install(ctx, Path(args.install_dir).resolve())
        return 0
    except (ConfigError, subprocess.CalledProcessError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
