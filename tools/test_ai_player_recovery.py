#!/usr/bin/env python3
"""Kill the standalone MUD process at AI checkpoints and verify recovery."""

from __future__ import annotations

import argparse
import json
import re
import socket
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path

from ai_admin_smoke import (
    AI_IDS,
    DEFAULT_ID,
    DEFAULT_NAME,
    DEFAULT_PASSWORD,
    MudClient,
    login,
    require,
)


PACKAGE = "com.zjmud.android"
ACTIVITY = f"{PACKAGE}/.MainActivity"
MUD_PROCESS = f"{PACKAGE}:mud"
FAULT_RECEIVER = f"{PACKAGE}/.FaultInjectionReceiver"
FAULT_ACTION = f"{PACKAGE}.DEBUG_KILL_MUD_PROCESS"
TEST_ROOM = "/d/standalone/ai_test"
RECOVERY_RE = re.compile(r"AI_RECOVERY\s+([^\r\n]+)")
PAIR_RE = re.compile(r"(\w+)=([^\s]+)")
STABILITY_PLAYER_RE = re.compile(
    r"AI_STABILITY_PLAYER id=(\S+) loaded=(\d+) errors=(\d+) "
    r"action_failures=(\d+) route_failures=(\d+) relocations=(\d+) "
    r"respawns=(\d+) pending=(\d+) actions=(\d+)"
)
CASES = ("patrol", "rest", "social", "supplies", "combat", "savepoint", "migration")


def adb(serial: str, *args: str, check: bool = True) -> str:
    command = ["adb"] + (["-s", serial] if serial else []) + list(args)
    result = subprocess.run(command, check=check, capture_output=True)
    return result.stdout.decode("utf-8", errors="replace")


def run_as(serial: str, *args: str, check: bool = True) -> str:
    return adb(serial, "exec-out", "run-as", PACKAGE, *args, check=check)


def mud_pid(serial: str) -> int:
    output = adb(serial, "shell", "pidof", MUD_PROCESS, check=False).strip()
    if not output:
        return 0
    return int(output.split()[0])


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


def wait_for_saves(serial: str, timeout: int = 90) -> None:
    expected = {f"{ai_id}.o" for ai_id in AI_IDS}
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        found: list[set[str]] = []
        for kind in ("login", "user"):
            output = run_as(
                serial,
                "find",
                f"files/runtime/current/data/{kind}/a",
                "-maxdepth",
                "1",
                "-type",
                "f",
                "-name",
                "ai_*.o",
                "-printf",
                "%f\\n",
                check=False,
            )
            found.append(set(output.splitlines()))
        if found[0] == expected and found[1] == expected:
            return
        time.sleep(1)
    raise RuntimeError("five AI login and player saves did not recover")


def connect(args: argparse.Namespace) -> MudClient:
    adb(args.serial, "forward", f"tcp:{args.port}", "tcp:3000")
    wait_for_port(args.host, args.port)
    time.sleep(3)
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


def recovery(client: MudClient, ai_id: str) -> dict[str, str]:
    output = client.command(f"aiplayer recovery {ai_id}", 0.6)
    match = RECOVERY_RE.search(output)
    if not match:
        raise RuntimeError(f"unable to parse recovery state for {ai_id}:\n{output[-2000:]}")
    return dict(PAIR_RE.findall(match.group(1)))


def wait_state(
    client: MudClient,
    ai_id: str,
    predicate,
    description: str,
    timeout: int,
) -> dict[str, str]:
    deadline = time.monotonic() + timeout
    latest: dict[str, str] = {}
    while time.monotonic() < deadline:
        latest = recovery(client, ai_id)
        if predicate(latest):
            return latest
        time.sleep(0.5)
    raise RuntimeError(f"{ai_id} did not reach {description}: {latest}")


def kill_and_restart(args: argparse.Namespace, client: MudClient) -> MudClient:
    pid = mud_pid(args.serial)
    if not pid:
        raise RuntimeError(f"{MUD_PROCESS} is not running")
    client.close()
    result = adb(
        args.serial,
        "shell",
        "am",
        "broadcast",
        "-n",
        FAULT_RECEIVER,
        "-a",
        FAULT_ACTION,
        check=False,
    )
    deadline = time.monotonic() + 10
    while time.monotonic() < deadline and mud_pid(args.serial) == pid:
        time.sleep(0.2)
    if mud_pid(args.serial) == pid:
        raise RuntimeError(f"unable to kill {MUD_PROCESS} pid={pid}: {result}")

    # The service is START_NOT_STICKY. Recreate the app task after the targeted
    # service-process kill so MainActivity starts it with the deployed config.
    adb(args.serial, "shell", "am", "force-stop", PACKAGE)
    adb(args.serial, "shell", "am", "start", "-n", ACTIVITY)
    wait_for_saves(args.serial)
    return connect(args)


def reset(client: MudClient, *ids: str) -> None:
    for ai_id in ids:
        require(
            client.command(f"aiplayer reset {ai_id}", 0.6),
            "行为状态、生命状态和位置已复位",
            f"reset {ai_id}",
        )


def wait_finished(
    client: MudClient,
    ai_id: str,
    outcomes: set[str],
    timeout: int = 240,
) -> dict[str, str]:
    return wait_state(
        client,
        ai_id,
        lambda state: state.get("active") == "0" and state.get("last_outcome") in outcomes,
        "finished outcome " + "/".join(sorted(outcomes)),
        timeout,
    )


def assert_common(client: MudClient) -> None:
    stability = client.command("aiplayer stability", 1.2)
    players = {match.group(1): match.groups()[1:] for match in STABILITY_PLAYER_RE.finditer(stability)}
    if set(players) != set(AI_IDS):
        raise RuntimeError(f"stability output omitted AI players: {sorted(players)}")
    for ai_id, values in players.items():
        loaded, errors, action_failures, _, _, _, pending, _ = map(int, values)
        if loaded != 1 or errors != 0 or action_failures != 0 or pending > 16:
            raise RuntimeError(
                f"unsafe recovered metrics for {ai_id}: loaded={loaded} errors={errors} "
                f"action_failures={action_failures} pending={pending}"
            )
    for ai_id in AI_IDS:
        state = recovery(client, ai_id)
        if state.get("schema") != "2":
            raise RuntimeError(f"{ai_id} did not use activity schema 2: {state}")
        if state.get("fighting") != "0" or state.get("scenario") != "0":
            raise RuntimeError(f"{ai_id} retained combat/scenario state: {state}")
        if state.get("room") == TEST_ROOM:
            raise RuntimeError(f"{ai_id} remained in the isolated test room")


def case_patrol(args: argparse.Namespace, client: MudClient) -> MudClient:
    reset(client, "ai_qingfeng")
    require(
        client.command("aiplayer activity run ai_qingfeng city_watch", 0.6),
        "已强制安排角色活动",
        "start patrol recovery",
    )
    wait_state(
        client,
        "ai_qingfeng",
        lambda state: state.get("step") in {"patrol_1", "patrol_2"},
        "mid-patrol checkpoint",
        180,
    )
    require(
        client.command("aiplayer recovery prepare ai_qingfeng legacy", 0.6),
        "已准备恢复测试 legacy",
        "downgrade patrol activity schema",
    )
    client = kill_and_restart(args, client)
    wait_finished(client, "ai_qingfeng", {"patrol_completed", "cancelled_timeout"})
    metrics = client.command("aiplayer metrics ai_qingfeng", 0.8)
    if not re.search(r"recovery migrations=[1-9]\d*", metrics):
        raise RuntimeError(f"legacy patrol state was not migrated: {metrics[-2000:]}")
    return client


def case_rest(args: argparse.Namespace, client: MudClient) -> MudClient:
    reset(client, "ai_qingfeng")
    require(
        client.command("aiplayer activity run ai_qingfeng inn_rest", 0.6),
        "已强制安排角色活动",
        "start rest recovery",
    )
    wait_state(
        client,
        "ai_qingfeng",
        lambda state: state.get("step") == "resting",
        "resting checkpoint",
        180,
    )
    client = kill_and_restart(args, client)
    wait_finished(client, "ai_qingfeng", {"rested", "cancelled_timeout"}, 180)
    return client


def case_social(args: argparse.Namespace, client: MudClient) -> MudClient:
    reset(client, "ai_qingfeng", "ai_wantang")
    require(
        client.command("aiplayer activity run ai_qingfeng meet_wantang", 0.6),
        "已强制安排角色活动",
        "start social recovery",
    )
    deadline = time.monotonic() + 60
    primary: dict[str, str] = {}
    partner: dict[str, str] = {}
    while time.monotonic() < deadline:
        primary = recovery(client, "ai_qingfeng")
        partner = recovery(client, "ai_wantang")
        if (
            primary.get("active") == "1"
            and primary.get("partner") == "ai_wantang"
            and partner.get("active") == "1"
            and partner.get("partner") == "ai_qingfeng"
        ):
            break
        time.sleep(0.25)
    else:
        raise RuntimeError(
            f"social activities were not checkpointed reciprocally: primary={primary} partner={partner}"
        )
    client = kill_and_restart(args, client)
    wait_finished(
        client,
        "ai_qingfeng",
        {"met_ai_wantang", "cancelled_social_mismatch", "partner_unavailable"},
        240,
    )
    wait_state(
        client,
        "ai_wantang",
        lambda state: state.get("active") == "0",
        "partner release",
        120,
    )
    return client


def case_supplies(args: argparse.Namespace, client: MudClient) -> MudClient:
    reset(client, *AI_IDS)
    require(
        client.command("aiplayer recovery prepare ai_wantang supplies", 0.8),
        "已准备恢复测试 supplies",
        "prepare supply recovery",
    )
    purchased = wait_state(
        client,
        "ai_wantang",
        lambda state: state.get("step") == "purchased_water",
        "post-purchase checkpoint",
        180,
    )
    purchased_money = int(purchased["money"])
    if purchased_money != 1500 or int(purchased["water_items"]) != 1:
        raise RuntimeError(f"water purchase checkpoint was not deterministic: {purchased}")
    client = kill_and_restart(args, client)
    final = wait_finished(client, "ai_wantang", {"supplies_adapted"}, 240)
    final_money = int(final["money"])
    food_spend = purchased_money - final_money
    if food_spend < 50 or food_spend > 500 or food_spend % 50 != 0:
        raise RuntimeError(
            f"supply recovery duplicated or skipped a purchase: checkpoint={purchased_money} final={final}"
        )
    if int(final["food_items"]) > 0 or int(final["water_items"]) != 1:
        raise RuntimeError(f"supply recovery left duplicate consumables: {final}")
    return client


def case_combat(args: argparse.Namespace, client: MudClient) -> MudClient:
    reset(client, "ai_yanqiu")
    require(
        client.command("aiplayer scenario combat ai_yanqiu", 0.4),
        "已启动隔离非致死战斗场景",
        "start combat recovery",
    )
    wait_state(
        client,
        "ai_yanqiu",
        lambda state: state.get("scenario") == "1" and state.get("fighting") == "1",
        "running isolated combat",
        4,
    )
    client = kill_and_restart(args, client)
    state = wait_state(
        client,
        "ai_yanqiu",
        lambda current: current.get("fighting") == "0" and current.get("scenario") == "0",
        "cleared combat state",
        30,
    )
    if state.get("room") == TEST_ROOM:
        raise RuntimeError(f"combat recovery retained the isolated room: {state}")
    return client


def case_savepoint(args: argparse.Namespace, client: MudClient) -> MudClient:
    reset(client, "ai_zhiyuan")
    require(
        client.command("aiplayer recovery prepare ai_zhiyuan savepoint", 0.4),
        "已准备恢复测试 savepoint",
        "prepare save boundary",
    )
    wait_state(
        client,
        "ai_zhiyuan",
        lambda state: int(state.get("save_in", "99")) <= 1,
        "periodic save boundary",
        5,
    )
    return kill_and_restart(args, client)


def case_migration(args: argparse.Namespace, client: MudClient) -> MudClient:
    reset(client, "ai_songlan")
    require(
        client.command("aiplayer activity run ai_songlan lake_walk", 0.5),
        "已强制安排角色活动",
        "start invalid migration test",
    )
    require(
        client.command("aiplayer recovery prepare ai_songlan invalid", 0.5),
        "已准备恢复测试 invalid",
        "persist invalid legacy target",
    )
    client = kill_and_restart(args, client)
    wait_finished(client, "ai_songlan", {"cancelled_invalid_target"}, 60)
    return client


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--serial", default="")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=3000)
    parser.add_argument("--account", default=DEFAULT_ID)
    parser.add_argument("--password", default=DEFAULT_PASSWORD)
    parser.add_argument("--name", default=DEFAULT_NAME)
    parser.add_argument("--case", action="append", choices=CASES, dest="cases")
    parser.add_argument("--output-dir", default="build/reports/ai-recovery")
    args = parser.parse_args()

    selected = args.cases or list(CASES)
    adb(args.serial, "get-state")
    results: list[dict[str, object]] = []
    client = connect(args)
    handlers = {
        "patrol": case_patrol,
        "rest": case_rest,
        "social": case_social,
        "supplies": case_supplies,
        "combat": case_combat,
        "savepoint": case_savepoint,
        "migration": case_migration,
    }
    try:
        for case in selected:
            started = time.monotonic()
            print(f"[{case}] injecting service-process failure", flush=True)
            client = handlers[case](args, client)
            assert_common(client)
            results.append({"case": case, "passed": True, "seconds": round(time.monotonic() - started, 1)})
            print(f"[{case}] passed", flush=True)
    finally:
        try:
            reset(client, *AI_IDS)
        except (OSError, RuntimeError):
            try:
                client.close()
            except OSError:
                pass
            try:
                client = connect(args)
                reset(client, *AI_IDS)
            except (OSError, RuntimeError, subprocess.CalledProcessError):
                pass
        finally:
            try:
                client.close()
            except OSError:
                pass

    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    report_path = output_dir / f"recovery-{datetime.now().strftime('%Y%m%d-%H%M%S')}.json"
    report_path.write_text(
        json.dumps({"schema": 2, "cases": results, "passed": len(results) == len(selected)}, indent=2) + "\n",
        encoding="utf-8",
    )
    print(f"AI recovery fault injection passed. Report: {report_path}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError, ValueError) as error:
        print(f"AI recovery fault injection failed: {error}", file=sys.stderr)
        raise SystemExit(1)
