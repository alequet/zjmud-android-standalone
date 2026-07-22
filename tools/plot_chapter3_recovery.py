#!/usr/bin/env python3
"""Force-stop the Android app at Chapter 3 checkpoints and verify recovery."""

from __future__ import annotations

import argparse
import socket
import subprocess
import sys
import time

try:
    from .plot_chapter1_smoke import MudClient, admin_call, call_int, login, require
    from .plot_chapter3_smoke import identify_boat, inspect_cargos, reset_case
except ImportError:
    from plot_chapter1_smoke import MudClient, admin_call, call_int, login, require
    from plot_chapter3_smoke import identify_boat, inspect_cargos, reset_case


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
    if not process_pid(args.serial, MUD_PROCESS):
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


def enter_from_safe_room(client: MudClient, marker: str) -> None:
    admin_call(client, 'me->move("/d/city/beimen")')
    require(
        client.command("plot act silent_crossing_03 enter", 1.0),
        marker,
        "re-enter Chapter 3 instance",
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
        reset_case(client, 100_000, True)
        identify_boat(client, False)
        require(
            client.command("plot act silent_crossing_03 board_token", 1.0),
            "放下了跳板",
            "board by token before deck checkpoint",
        )
        print("[boat-deck] checkpoint prepared", flush=True)

        client = force_stop_and_reconnect(args, client)
        require_value(
            client,
            'me->query("plot/silent_crossing_03/stage")',
            '"reach_cargo_hold"',
            "boat-deck stage recovery",
        )
        require_value(
            client,
            'me->query("plot/silent_crossing_03/flags/boarding_method")',
            '"token"',
            "boarding method recovery",
        )
        enter_from_safe_room(client, "踏过湿滑跳板")
        require_value(
            client,
            'liu dashao->query("plot_owner")',
            '"codexaiqa"',
            "boat owner recovery",
        )
        print("[boat-deck] passed", flush=True)

        require(
            client.command("plot act silent_crossing_03 reach_cargo_hold", 1.0),
            "三条登船路线都汇入",
            "reach shared passage before recovery",
        )
        require(
            client.command("plot act silent_crossing_03 reach_cargo_hold", 1.0),
            "阿禾被绳索和药箱压在舱角",
            "reach cargo hold before recovery",
        )
        print("[a-he-rescue] checkpoint prepared", flush=True)

        client = force_stop_and_reconnect(args, client)
        require_value(
            client,
            'me->query("plot/silent_crossing_03/stage")',
            '"save_a_he"',
            "A He rescue stage recovery",
        )
        enter_from_safe_room(client, "钻入货舱")
        if call_int(client, 'a he->query("qi")') <= 0:
            raise RuntimeError("A He did not rebuild alive")
        admin_call(client, "a he->unconcious()", 0.8)
        require_value(
            client,
            'me->query("plot/silent_crossing_03/stage")',
            '"inspect_three_cargos"',
            "A He injury stage persisted",
        )
        print("[injured-condition] checkpoint prepared", flush=True)

        client = force_stop_and_reconnect(args, client)
        require_value(
            client,
            'me->query("plot/silent_crossing_03/stage")',
            '"inspect_three_cargos"',
            "A He injury stage recovery",
        )
        enter_from_safe_room(client, "钻入货舱")
        require_value(
            client,
            'a he->query("plot_condition")',
            '"injured"',
            "A He injury condition recovery",
        )
        if call_int(client, 'a he->query("qi")') <= 0:
            raise RuntimeError("A He did not rebuild alive after injury")
        print("[injured-condition] passed", flush=True)
        print("[a-he-rescue] passed", flush=True)

        expected = inspect_cargos(client)
        print("[burning-records] checkpoint prepared", flush=True)

        client = force_stop_and_reconnect(args, client)
        require_value(
            client,
            'me->query("plot/silent_crossing_03/stage")',
            '"stop_burning_records"',
            "burning-record stage recovery",
        )
        enter_from_safe_room(client, "钻入货舱")
        require_value(
            client,
            'luo qiniang->query("plot_combat/profile")',
            '"ferry_matron"',
            "Luo profile recovery",
        )
        if call_int(client, 'luo qiniang->query("combat_exp")') != expected["enemy_exp"]:
            raise RuntimeError("Luo EXP changed across recovery")
        if call_int(client, 'luo qiniang->query("plot_combat/skill_level")') != expected["skill"]:
            raise RuntimeError("Luo skill changed across recovery")
        if call_int(client, 'luo qiniang->query("max_qi")') != expected["qi"]:
            raise RuntimeError("Luo qi changed across recovery")
        require_value(
            client,
            'luo qiniang->query("attitude")',
            '"peaceful"',
            "Luo peaceful recovery",
        )
        print("[burning-records] passed", flush=True)

        require(
            client.command("plot act silent_crossing_03 fight_luo", 1.0),
            "罗七娘拔出短刀",
            "fight Luo before resolution checkpoint",
        )
        if call_int(client, "luo qiniang->is_fighting()") != 1:
            raise RuntimeError("Luo did not enter real combat")
        admin_call(client, "luo qiniang->die()", 0.8)
        require(
            client.command_until(
                'call me->query("plot/silent_crossing_03/flags/records_saved")',
                "= 2$br#",
            ),
            "= 2$br#",
            "Luo defeat checkpoint saved",
        )
        print("[records-saved] checkpoint prepared", flush=True)

        client = force_stop_and_reconnect(args, client)
        require_value(
            client,
            'me->query("plot/silent_crossing_03/stage")',
            '"settle_the_boat"',
            "records-saved stage recovery",
        )
        enter_from_safe_room(client, "钻入货舱")
        require(
            client.command("plot act silent_crossing_03 settle_aid_first", 0.9),
            "先卸救命粮",
            "settle boat after recovery",
        )
        score_before = call_int(client, 'me->query("score")')
        require(
            client.command("plot act silent_crossing_03 chapter_close", 1.0),
            "第三章「夜渡无声」完成",
            "complete third chapter after recovery",
        )
        if call_int(client, 'me->query("score")') != score_before + 15:
            raise RuntimeError("third chapter score was not granted exactly once")
        print("[records-saved] passed", flush=True)

        score_after = score_before + 15
        client = force_stop_and_reconnect(args, client)
        require_value(
            client,
            'me->query("plot/silent_crossing_03/status")',
            '"completed"',
            "third chapter completed recovery",
        )
        client.command("plot act silent_crossing_03 chapter_close", 1.0)
        if call_int(client, 'me->query("score")') != score_after:
            raise RuntimeError("third chapter duplicated score after recovery")
        print("[completed-idempotency] passed", flush=True)
        print("Third chapter force-stop recovery passed.")
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
        print(f"Third chapter force-stop recovery failed: {error}", file=sys.stderr)
        raise SystemExit(1)
