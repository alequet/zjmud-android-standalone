#!/usr/bin/env python3
"""Force-stop the Android app at Chapter 2 checkpoints and verify recovery."""

from __future__ import annotations

import argparse
import socket
import subprocess
import sys
import time

try:
    from .plot_chapter1_smoke import MudClient, admin_call, call_int, login, require
    from .plot_chapter2_smoke import reach_old_storehouse, reset_case
except ImportError:
    from plot_chapter1_smoke import MudClient, admin_call, call_int, login, require
    from plot_chapter2_smoke import reach_old_storehouse, reset_case


PACKAGE = "com.zjmud.android"
ACTIVITY = f"{PACKAGE}/.MainActivity"
MUD_PROCESS = f"{PACKAGE}:mud"


def adb(serial: str, *args: str, check: bool = True) -> str:
    command = ["adb"] + (["-s", serial] if serial else []) + list(args)
    result = subprocess.run(command, check=check, capture_output=True)
    return result.stdout.decode("utf-8", errors="replace")


def process_pid(serial: str, process: str) -> int:
    output = adb(serial, "shell", "pidof", process, check=False).strip()
    return int(output.split()[0]) if output else 0


def wait_for_port(host: str, port: int, timeout: int = 90) -> None:
    deadline = time.monotonic() + timeout
    last_error: OSError | None = None
    while time.monotonic() < deadline:
        try:
            with socket.create_connection((host, port), timeout=1):
                return
        except OSError as error:
            last_error = error
            time.sleep(1)
    raise RuntimeError(f"MUD port did not recover within {timeout}s: {last_error}")


def connect(args: argparse.Namespace) -> MudClient:
    adb(args.serial, "forward", f"tcp:{args.port}", "tcp:3000")
    wait_for_port(args.host, args.port)
    deadline = time.monotonic() + 90
    last_error: Exception | None = None
    while time.monotonic() < deadline:
        client: MudClient | None = None
        try:
            client = MudClient(args.host, args.port)
            login(client, args.account, args.password, args.name)
            return client
        except (OSError, RuntimeError) as error:
            last_error = error
            if client is not None:
                client.close()
            time.sleep(1)
    raise RuntimeError(f"administrator login did not recover: {last_error}")


def clear_web_login(serial: str) -> None:
    adb(
        serial,
        "shell",
        "run-as",
        PACKAGE,
        "rm",
        "-rf",
        "app_webview/Default/Local Storage",
        "app_webview/Default/Session Storage",
        check=False,
    )


def force_stop_and_reconnect(args: argparse.Namespace, client: MudClient) -> MudClient:
    old_pid = process_pid(args.serial, MUD_PROCESS)
    if not old_pid:
        raise RuntimeError(f"{MUD_PROCESS} is not running")
    client.close()
    adb(args.serial, "shell", "am", "force-stop", PACKAGE)
    deadline = time.monotonic() + 20
    while time.monotonic() < deadline and process_pid(args.serial, MUD_PROCESS):
        time.sleep(0.25)
    if process_pid(args.serial, MUD_PROCESS):
        raise RuntimeError(f"{MUD_PROCESS} survived app force-stop")
    clear_web_login(args.serial)
    adb(args.serial, "shell", "am", "start", "-W", "-n", ACTIVITY)
    return connect(args)


def require_value(client: MudClient, expression: str, marker: str, context: str) -> None:
    require(admin_call(client, expression), marker, context)


def enter_from_safe_room(client: MudClient, expected: str) -> None:
    admin_call(client, 'me->move("/d/city/beimen")')
    require(
        client.command("plot act returning_mark_02 enter", 1.0),
        expected,
        "re-enter Chapter 2 instance",
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--serial", default="")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=3000)
    parser.add_argument("--account", default="codexaiqa")
    parser.add_argument("--password", default="codex-ai-qa")
    parser.add_argument("--name", default="春风明月")
    args = parser.parse_args()

    client = connect(args)
    try:
        reset_case(client, "hall", 100_000, True)
        expected = reach_old_storehouse(client, "hall", True)
        print("[alive-enemy] checkpoint prepared", flush=True)

        client = force_stop_and_reconnect(args, client)
        require_value(
            client,
            'me->query("plot/returning_mark_02/stage")',
            '"protect_meng_si"',
            "alive-enemy stage recovery",
        )
        enter_from_safe_room(client, "推开旧仓沉重的木门")
        require_value(
            client,
            'qing peng->query("plot_combat/profile")',
            '"green_hood"',
            "alive-enemy profile recovery",
        )
        if call_int(client, 'qing peng->query("combat_exp")') != expected["enemy_exp"]:
            raise RuntimeError("alive-enemy EXP changed across recovery")
        if call_int(client, 'qing peng->query("plot_combat/skill_level")') != expected["skill"]:
            raise RuntimeError("alive-enemy skill changed across recovery")
        if call_int(client, 'qing peng->query("max_neili")') != expected["max_neili"]:
            raise RuntimeError("alive-enemy max neili changed across recovery")
        if call_int(client, 'qing peng->query("neili")') != expected["neili"]:
            raise RuntimeError("alive-enemy current neili changed across recovery")
        require_value(
            client,
            'qing peng->query("attitude")',
            '"peaceful"',
            "alive-enemy peaceful recovery",
        )
        print("[alive-enemy] passed", flush=True)

        admin_call(client, "meng si->unconcious()", 0.8)
        require_value(
            client,
            'me->query("plot/returning_mark_02/flags/meng_injured")',
            "= 1",
            "Meng Si injury checkpoint saved",
        )
        print("[injured-condition] checkpoint prepared", flush=True)

        client = force_stop_and_reconnect(args, client)
        require_value(
            client,
            'me->query("plot/returning_mark_02/stage")',
            '"protect_meng_si"',
            "Meng Si injury stage recovery",
        )
        enter_from_safe_room(client, "推开旧仓沉重的木门")
        require_value(
            client,
            'meng si->query("plot_condition")',
            '"injured"',
            "Meng Si injury condition recovery",
        )
        if call_int(client, 'meng si->query("qi")') <= 0:
            raise RuntimeError("Meng Si did not rebuild alive after injury")
        print("[injured-condition] passed", flush=True)

        require(
            client.command("plot act returning_mark_02 fight_qing", 1.0),
            "青篷客翻过箱垛",
            "start fight before death checkpoint",
        )
        if call_int(client, "qing peng->is_fighting()") != 1:
            raise RuntimeError("Qing Peng Ke did not enter combat before death checkpoint")
        admin_call(client, "qing peng->die()", 0.8)
        require_value(
            client,
            'me->query("plot/returning_mark_02/flags/full_token_recovered")',
            "= 1",
            "enemy-death checkpoint saved",
        )
        print("[enemy-death] checkpoint prepared", flush=True)

        client = force_stop_and_reconnect(args, client)
        require_value(
            client,
            'me->query("plot/returning_mark_02/stage")',
            '"protect_meng_si"',
            "enemy-death stage recovery",
        )
        enter_from_safe_room(client, "推开旧仓沉重的木门")
        require(
            client.command("plot act returning_mark_02 fight_qing", 1.0),
            "潮印案台",
            "resolved enemy did not respawn and route transition recovered",
        )
        require_value(
            client,
            'me->query("plot/returning_mark_02/stage")',
            '"reconstruct_route"',
            "enemy-death transition stage",
        )
        print("[enemy-death] passed", flush=True)

        client.command("plot act returning_mark_02 route_tide_2")
        client.command("plot act returning_mark_02 route_ship_23")
        print("[mark-chamber] checkpoint prepared", flush=True)

        client = force_stop_and_reconnect(args, client)
        require_value(
            client,
            'me->query("plot/returning_mark_02/choices/route_tide")',
            '"2"',
            "mark-chamber tide recovery",
        )
        require_value(
            client,
            'me->query("plot/returning_mark_02/choices/route_ship")',
            '"23"',
            "mark-chamber ship recovery",
        )
        enter_from_safe_room(client, "潮印案台")
        client.command("plot act returning_mark_02 route_warehouse_7")
        require(
            client.command("plot act returning_mark_02 confirm_route", 1.0),
            "青芦渡",
            "mark-chamber completion after recovery",
        )
        score_before = call_int(client, 'me->query("score")')
        require(
            client.command("plot act returning_mark_02 chapter_close", 1.0),
            "第二章「旧印新痕」完成",
            "chapter completion after recovery",
        )
        if call_int(client, 'me->query("score")') != score_before + 12:
            raise RuntimeError("chapter score was not granted exactly once")
        print("[mark-chamber] passed", flush=True)

        score_after = score_before + 12
        client = force_stop_and_reconnect(args, client)
        require_value(
            client,
            'me->query("plot/returning_mark_02/status")',
            '"completed"',
            "completed status recovery",
        )
        client.command("plot act returning_mark_02 chapter_close", 1.0)
        if call_int(client, 'me->query("score")') != score_after:
            raise RuntimeError("completed chapter duplicated score after recovery")
        print("[completed-idempotency] passed", flush=True)
        print("Second chapter force-stop recovery passed.")
        return 0
    finally:
        try:
            admin_call(client, 'me->set("born","扬州人氏")')
            admin_call(client, 'me->set("startroom","/d/city/wumiao")')
            admin_call(client, 'me->set("combat_exp",0)')
            for key in ("meridian/ap", "meridian/dp", "gain/attack", "gain/defense"):
                admin_call(client, f'me->delete("{key}")')
        except (OSError, RuntimeError):
            pass
        client.close()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError, ValueError) as error:
        print(f"Second chapter force-stop recovery failed: {error}", file=sys.stderr)
        raise SystemExit(1)
