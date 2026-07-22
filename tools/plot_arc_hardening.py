#!/usr/bin/env python3
"""Cross-chapter static and runtime hardening checks for the original plot arc."""

from __future__ import annotations

import argparse
import re
import sys
import zipfile
from pathlib import Path

try:
    from .plot_chapter1_smoke import MudClient, admin_call, call_int, login, require
except ImportError:
    from plot_chapter1_smoke import MudClient, admin_call, call_int, login, require


ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOT = ROOT / "tools/mudlib/plot"
RUNTIME_ZIP = ROOT / "app/src/main/assets/runtime/zjmud-runtime.zip"
CHAPTERS = (
    ("origin_letter_01", "notice", "aftermath"),
    ("returning_mark_02", "branch_hook", "chapter_close"),
    ("silent_crossing_03", "watch_green_reed_ferry", "chapter_close"),
    ("visitors_from_home_04", "letters_arrive", "chapter_close"),
    ("where_rivers_end_05", "enter_salt_storehouse", "arc_aftermath"),
)
MENU_RE = re.compile(r'"([^"\n:]{1,80}):((?:plot)(?: [^"\n]*))')
AUDIT_RE = re.compile(
    r"PLOT_AUDIT schema=(\d+) state_chars=(\d+) instance_refs=(\d+) objects=(\d+)"
)
FORBIDDEN_WORLD_TEXT = (
    "指间MUD", "指间 MUD", "大秦", "秦朝", "大汉", "汉朝", "大唐", "唐朝",
    "大宋", "宋朝", "大元", "元朝", "大明", "明朝", "大清", "清朝",
)
FORBIDDEN_IDENTITY_WRITES = (
    'set("family")', 'set("family/', 'add("family/', 'delete("family")',
    'delete("family/', "assign_apprentice", "baishi_times", "out_family",
    "betrayer/", "QUEST_D->bonus",
)


def check(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def source_files() -> list[Path]:
    return sorted(path for path in SOURCE_ROOT.rglob("*") if path.suffix in {".c", ".h"})


def check_encoding(files: list[Path]) -> None:
    for path in files:
        text = path.read_text(encoding="utf-8")
        encoded = text.encode("gb18030")
        check(encoded.decode("gb18030") == text, f"GB18030 round trip changed {path}")
        for line_number, literal in enumerate(re.findall(r'"([^"\n]*)"', text), 1):
            check(len(literal.encode("gb18030")) <= 2048,
                  f"oversized string literal in {path}:{line_number}")


def check_runtime_copy(files: list[Path]) -> None:
    check(RUNTIME_ZIP.is_file(), f"runtime archive missing: {RUNTIME_ZIP}")
    with zipfile.ZipFile(RUNTIME_ZIP) as archive:
        names = set(archive.namelist())
        for path in files:
            relative = path.relative_to(SOURCE_ROOT).as_posix()
            check(relative in names, f"runtime plot file missing: {relative}")
            runtime_text = archive.read(relative).decode("gb18030")
            check(runtime_text == path.read_text(encoding="utf-8"),
                  f"runtime plot file differs from source: {relative}")


def check_menus(files: list[Path]) -> None:
    menu_count = 0
    for path in files:
        text = path.read_text(encoding="utf-8")
        if "ZJOBACTS2" in text:
            check("ZJMENUF(" in text, f"ZJ action block has no menu format: {path}")
        for match in MENU_RE.finditer(text):
            label, command = match.groups()
            menu_count += 1
            check(len(label) <= 12, f"mobile button label too long ({len(label)}): {label}")
            check(command == "plot" or command.startswith("plot "),
                  f"invalid plot menu command in {path}: {command}")
            check(";" not in command and "|" not in command,
                  f"unsafe plot menu command in {path}: {command}")
    check(menu_count >= 35, f"too few plot menu commands scanned: {menu_count}")


def check_story_boundaries(files: list[Path]) -> None:
    combined = "\n".join(path.read_text(encoding="utf-8") for path in files)
    for phrase in FORBIDDEN_WORLD_TEXT:
        check(phrase not in combined, f"forbidden world/era name in runtime plot: {phrase}")
    for marker in FORBIDDEN_IDENTITY_WRITES:
        check(marker not in combined, f"plot writes or couples identity state: {marker}")
    for marker in ('query("relife', 'query("reincarn', 'query("reborn', 'query("zhuanshi'):
        check(marker not in combined, f"plot reads reincarnation state: {marker}")


def check_instance_contracts() -> None:
    controllers = sorted((SOURCE_ROOT / "adm/daemons/plot").glob("*.c"))
    for path in controllers:
        text = path.read_text(encoding="utf-8")
        if "open_instance(" not in text:
            continue
        for marker in ("void instance_left", "close_instance(me, PLOT_ID)", "no_save_location"):
            if marker == "no_save_location":
                continue
            check(marker in text, f"instance cleanup contract missing in {path.name}: {marker}")
        if "open_instance_room(" in text:
            check("close_instance_rooms(me, PLOT_ID)" in text,
                  f"multi-room cleanup missing in {path.name}")
    plotd = (SOURCE_ROOT / "adm/daemons/plotd.c").read_text(encoding="utf-8")
    for marker in ('room->set("no_save_location", 1)', 'me->delete_temp("plot/" + plot_id + "/instance")',
                   'me->delete_temp("plot/" + plot_id + "/instance_rooms")'):
        check(marker in plotd, f"central instance cleanup contract missing: {marker}")


def run_static() -> None:
    files = source_files()
    check(len(files) >= 35, f"unexpected plot source count: {len(files)}")
    check_encoding(files)
    check_runtime_copy(files)
    check_menus(files)
    check_story_boundaries(files)
    check_instance_contracts()
    print(f"Plot arc static hardening passed: files={len(files)}.")


def set_base_identity(client: MudClient) -> None:
    admin_call(client, 'me->set("registered",1)')
    admin_call(client, 'me->set("startroom","/d/city/wumiao")')
    admin_call(client, 'me->delete("born")')
    admin_call(client, 'me->delete("family")')
    admin_call(client, 'me->delete("betrayer")')


def assert_chapter(client: MudClient, chapter: str, status: str, stage: str) -> None:
    require(admin_call(client, f'me->query("plot/{chapter}/status")'), f'"{status}"', f"{chapter} status")
    require(admin_call(client, f'me->query("plot/{chapter}/stage")'), f'"{stage}"', f"{chapter} stage")
    check(call_int(client, f'me->query("plot/{chapter}/version")') == 2,
          f"{chapter} schema was not migrated")


def run_runtime(args: argparse.Namespace) -> None:
    client = MudClient(args.host, args.port)
    try:
        login(client, args.account, args.password, args.name)

        admin_call(client, 'me->delete("plot")')
        set_base_identity(client)
        require(client.command("plot"), "乡书篇", "historical state initialization")
        assert_chapter(client, CHAPTERS[0][0], "available", CHAPTERS[0][1])
        for chapter, initial, _ in CHAPTERS[1:]:
            assert_chapter(client, chapter, "locked", initial)

        for family_mode in ("none", "member", "betrayer"):
            admin_call(client, 'me->delete("plot")')
            set_base_identity(client)
            if family_mode == "member":
                admin_call(client, 'me->set("family/family_name","少林派")')
            elif family_mode == "betrayer":
                admin_call(client, 'me->set("betrayer/少林派",1)')
            client.command("plot")
            assert_chapter(client, CHAPTERS[0][0], "available", CHAPTERS[0][1])

        admin_call(client, 'me->delete("plot")')
        set_base_identity(client)
        admin_call(client, 'me->set("plot/arc/hometown_letters_01/version",1)')
        admin_call(client, 'me->set("plot/arc/hometown_letters_01/status","available")')
        admin_call(client, 'me->set("plot/arc/hometown_letters_01/current_chapter",1)')
        admin_call(client, 'me->set("plot/origin_letter_01/version",1)')
        admin_call(client, 'me->set("plot/origin_letter_01/status","active")')
        admin_call(client, 'me->set("plot/origin_letter_01/stage","inspect_ledger")')
        admin_call(client, 'me->set("plot/origin_letter_01/flags/legacy_marker",713)')
        client.command("plot")
        assert_chapter(client, CHAPTERS[0][0], "active", "inspect_ledger")
        check(call_int(client, 'me->query("plot/origin_letter_01/flags/legacy_marker")') == 713,
              "valid legacy progress was not preserved")

        admin_call(client, 'me->set("plot/origin_letter_01/status","broken")')
        admin_call(client, 'me->set("plot/origin_letter_01/stage","removed_stage")')
        client.command("plot")
        assert_chapter(client, CHAPTERS[0][0], "available", CHAPTERS[0][1])
        check(call_int(client, 'me->query("plot/origin_letter_01/flags/legacy_marker")') == 0,
              "corrupt unfinished chapter retained unsafe payload")

        admin_call(client, 'me->set("plot/origin_letter_01/status","completed")')
        admin_call(client, 'me->set("plot/origin_letter_01/stage","removed_stage")')
        admin_call(client, 'me->set("plot/origin_letter_01/choices/legacy_choice","hall")')
        client.command("plot")
        assert_chapter(client, CHAPTERS[0][0], "completed", CHAPTERS[0][2])
        require(admin_call(client, 'me->query("plot/origin_letter_01/choices/legacy_choice")'),
                '"hall"', "completed legacy choice preservation")

        admin_call(client, 'me->set("plot/origin_letter_01/version","broken")')
        admin_call(client, 'me->set("plot/origin_letter_01/completed_at",713)')
        admin_call(client, 'me->set("plot/origin_letter_01/stage","removed_stage")')
        client.command("plot")
        assert_chapter(client, CHAPTERS[0][0], "completed", CHAPTERS[0][2])
        require(admin_call(client, 'me->query("plot/origin_letter_01/choices/legacy_choice")'),
                '"hall"', "corrupt-version completion preservation")

        admin_call(client, 'me->set("plot/arc/hometown_letters_01/version",99)')
        admin_call(client, 'me->set("plot/origin_letter_01/version",1)')
        output = client.command("plot act origin_letter_01 notice")
        require(output, "剧情存档来自更新版本", "future schema rejection")
        check(call_int(client, 'me->query("plot/origin_letter_01/version")') == 1,
              "future schema rejection was not atomic")

        admin_call(client, 'me->set("plot/arc/hometown_letters_01/version",2)')
        admin_call(client, 'me->set("plot/returning_mark_02/version",99)')
        output = client.command("plot act origin_letter_01 notice")
        require(output, "剧情存档来自更新版本", "future chapter schema rejection")
        check(call_int(client, 'me->query("plot/origin_letter_01/version")') == 1,
              "future chapter rejection was not atomic")

        admin_call(client, 'me->delete("plot")')
        client.command("plot")
        audit = client.command("plotadmin audit")
        match = AUDIT_RE.search(audit)
        check(match is not None, f"unable to parse plot audit: {audit[-1000:]}")
        schema, state_chars, instance_refs, _ = map(int, match.groups())
        check(schema == 2, f"unexpected runtime plot schema: {schema}")
        check(state_chars <= 32768, f"plot state exceeds 32 KiB character budget: {state_chars}")
        check(instance_refs == 0, f"stale plot instance references remain: {instance_refs}")
        print(f"Plot arc runtime hardening passed: state_chars={state_chars}, instance_refs=0.")
    finally:
        client.close()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--runtime", action="store_true")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=3000)
    parser.add_argument("--account", default="codexaiqa")
    parser.add_argument("--password", default="codex-ai-qa")
    parser.add_argument("--name", default="春风明月")
    args = parser.parse_args()
    run_static()
    if args.runtime:
        run_runtime(args)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, UnicodeError, zipfile.BadZipFile) as error:
        print(f"Plot arc hardening failed: {error}", file=sys.stderr)
        raise SystemExit(1)
