from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from tools import hako


class HakoConfigureTest(unittest.TestCase):
    def context(self, platform_name: str) -> hako.BuildContext:
        temporary = Path(tempfile.mkdtemp())
        cfg = hako.resolve_config({})
        return hako.BuildContext(
            repo_root=temporary / "target",
            manifest=temporary / "target" / "hakoniwa-build.yaml",
            cfg=cfg,
            build_dir=temporary / "build",
            athrill_root=temporary / "athrill",
            vcpkg_root=temporary / "vcpkg" if platform_name == "windows" else None,
            platform_name=platform_name,
            arch="x64",
            vsdevcmd=temporary / "VsDevCmd.bat" if platform_name == "windows" else None,
            dry_run=True,
        )

    def test_posix_uses_host_default_generator(self) -> None:
        for platform_name in ("macos", "linux"):
            with self.subTest(platform_name=platform_name):
                command = hako.configure_command(self.context(platform_name))
                self.assertNotIn("-G", command)
                self.assertNotIn("Ninja", command)

    def test_windows_keeps_ninja_and_vcpkg(self) -> None:
        command = hako.configure_command(self.context("windows"))
        self.assertIn("Ninja", command)
        self.assertTrue(
            any(item.startswith("-DCMAKE_TOOLCHAIN_FILE=") for item in command)
        )
        self.assertIn("-DVCPKG_TARGET_TRIPLET=x64-windows-static", command)


if __name__ == "__main__":
    unittest.main()
