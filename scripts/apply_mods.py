#!/usr/bin/env python3
"""Apply STELLAR AI Touch UI V5.1 on top of 78/xiaozhi-esp32.

V4 keeps the stable Waveshare ESP32-S3-Touch-LCD-3.5 hardware/network base and
changes only the display/UI integration:
  1. copy STELLAR display sources into main/display/stellar/
  2. register the sources in main/CMakeLists.txt
  3. instantiate StellarDisplay on this one Waveshare board
  4. disable xiaozhi's WeChat bubble compile path for this board
  5. validate the V5.1 integrated light UI and RLCD-style long-answer behavior

It does NOT modify LCD pins, ST7796 init, touch init, SPI, I2C, PMIC or audio.
"""
from __future__ import annotations

import argparse
import json
import re
import shutil
from pathlib import Path

BOARD_REL = Path("main/boards/waveshare/esp32-s3-touch-lcd-3.5")
BUILD_NAME = "esp32-s3-touch-lcd-3.5"


def copy_overlay(overlay: Path, source: Path) -> None:
    if not overlay.is_dir():
        raise SystemExit(f"Overlay directory not found: {overlay}")
    for item in overlay.rglob("*"):
        if item.is_dir():
            continue
        rel = item.relative_to(overlay)
        dst = source / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(item, dst)
        print(f"[copy] {rel}")


def patch_cmake(source: Path) -> None:
    path = source / "main/CMakeLists.txt"
    text = path.read_text(encoding="utf-8")
    additions = [
        '            "display/stellar/stellar_display.cc"',
        '            "display/stellar/stellar_avatar.c"',
    ]
    if all(line in text for line in additions):
        print("[ok] CMake already contains STELLAR sources")
        return

    needle = '            "display/lcd_display.cc"'
    if needle not in text:
        raise SystemExit(
            "Upstream main/CMakeLists.txt changed: display/lcd_display.cc anchor not found"
        )
    text = text.replace(needle, needle + "\n" + "\n".join(additions), 1)
    path.write_text(text, encoding="utf-8")
    print("[patch] main/CMakeLists.txt")


def patch_board(source: Path) -> None:
    board_file = source / BOARD_REL / "esp32-s3-touch-lcd-3.5.cc"
    if not board_file.exists():
        raise SystemExit(f"Expected board file not found: {board_file}")

    text = board_file.read_text(encoding="utf-8")
    include_line = '#include "display/stellar/stellar_display.h"'
    if include_line not in text:
        anchor = '#include "display/lcd_display.h"'
        if anchor not in text:
            raise SystemExit("Board source changed: lcd_display include anchor not found")
        text = text.replace(anchor, anchor + "\n" + include_line, 1)
        print("[patch] board include -> StellarDisplay")

    if "new StellarDisplay(" not in text:
        pattern = re.compile(r"display_\s*=\s*new\s+SpiLcdDisplay\s*\(")
        text, count = pattern.subn("display_ = new StellarDisplay(", text, count=1)
        if count != 1:
            raise SystemExit(
                "Board source changed: expected exactly one 'new SpiLcdDisplay(' constructor"
            )
        print("[patch] SpiLcdDisplay -> StellarDisplay")

    board_file.write_text(text, encoding="utf-8")


def patch_board_config(source: Path) -> None:
    path = source / BOARD_REL / "config.json"
    data = json.loads(path.read_text(encoding="utf-8"))
    target = next(
        (build for build in data.get("builds", []) if build.get("name") == BUILD_NAME),
        None,
    )
    if target is None:
        raise SystemExit(f"Board config no longer contains build name {BUILD_NAME}")

    options = list(target.get("sdkconfig_append", []))
    new_options = []
    seen_wechat = False
    for option in options:
        if option in ("CONFIG_USE_WECHAT_MESSAGE_STYLE=y", "CONFIG_USE_WECHAT_MESSAGE_STYLE=n"):
            if not seen_wechat:
                new_options.append("CONFIG_USE_WECHAT_MESSAGE_STYLE=n")
                seen_wechat = True
            continue
        new_options.append(option)
    if not seen_wechat:
        new_options.append("CONFIG_USE_WECHAT_MESSAGE_STYLE=n")
    target["sdkconfig_append"] = new_options

    path.write_text(json.dumps(data, ensure_ascii=False, indent=4) + "\n", encoding="utf-8")
    print("[patch] board config -> CONFIG_USE_WECHAT_MESSAGE_STYLE=n")


def validate_result(source: Path) -> None:
    expected = [
        source / "main/display/stellar/stellar_display.h",
        source / "main/display/stellar/stellar_display.cc",
        source / "main/display/stellar/stellar_avatar.h",
        source / "main/display/stellar/stellar_avatar.c",
    ]
    missing = [str(p) for p in expected if not p.exists()]
    if missing:
        raise SystemExit("Missing overlay output:\n" + "\n".join(missing))

    board_text = (source / BOARD_REL / "esp32-s3-touch-lcd-3.5.cc").read_text(encoding="utf-8")
    if "new StellarDisplay(" not in board_text:
        raise SystemExit("Validation failed: board is not using StellarDisplay")

    config = json.loads((source / BOARD_REL / "config.json").read_text(encoding="utf-8"))
    target = next((b for b in config.get("builds", []) if b.get("name") == BUILD_NAME), None)
    options = target.get("sdkconfig_append", []) if target else []
    if "CONFIG_USE_WECHAT_MESSAGE_STYLE=n" not in options:
        raise SystemExit("Validation failed: WeChat message style was not disabled")
    if "CONFIG_USE_WECHAT_MESSAGE_STYLE=y" in options:
        raise SystemExit("Validation failed: contradictory WeChat y/n options remain")

    stellar = (source / "main/display/stellar/stellar_display.cc").read_text(encoding="utf-8")
    required_markers = [
        "Touch UI V5.1",
        "今日待办",
        "等待天气数据",
        "点击人物开始对话",
        "星语正在回答",
        "kScrollDelayMs = 2000",
        "kScrollMsPerPixel = 120",
        "lv_anim_start(&anim)",
        "SetChatExpandedInternal(true)",
        "home_avatar_, 140, 0",
        "点击人物开始对话",
    ]
    for marker in required_markers:
        if marker not in stellar:
            raise SystemExit(f"V5.1 validation failed: missing marker {marker!r}")

    avatar = (source / "main/display/stellar/stellar_avatar.c").read_text(encoding="utf-8")
    if ".w = 200" not in avatar or ".h = 320" not in avatar:
        raise SystemExit("V5.1 validation failed: expected 200x320 light avatar/scene asset")

    if "LcdDisplay::SetChatMessage(role, content)" in stellar:
        raise SystemExit("V5.1 validation failed: custom chat must not delegate to WeChat bubbles")

    # LVGL 9.5 only provides opacity constants in 10% steps. Catch accidental
    # pseudo-constants here before wasting a full GitHub Actions compile.
    invalid_opa = ["LV_OPA_15", "LV_OPA_55", "LV_OPA_76", "LV_OPA_88"]
    for token in invalid_opa:
        if token in stellar:
            raise SystemExit(f"V5.1 validation failed: unsupported LVGL opacity constant {token}")
    if re.search(r"(?m)^\s*lv_obj_set_style_text_font\s*\(", stellar):
        raise SystemExit(
            "V4 safety check failed: custom labels must not pin a raw runtime font pointer"
        )

    print("[done] STELLAR AI Touch UI V5.1 patch applied successfully")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True, type=Path, help="xiaozhi-esp32 source directory")
    parser.add_argument("--overlay", required=True, type=Path, help="STELLAR overlay directory")
    args = parser.parse_args()

    source = args.source.resolve()
    overlay = args.overlay.resolve()
    if not (source / "main" / "CMakeLists.txt").exists():
        raise SystemExit(f"Not a xiaozhi-esp32 checkout: {source}")

    copy_overlay(overlay, source)
    patch_cmake(source)
    patch_board(source)
    patch_board_config(source)
    validate_result(source)


if __name__ == "__main__":
    main()
