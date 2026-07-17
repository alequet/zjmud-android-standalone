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
import zipfile
from datetime import datetime
from pathlib import Path

from ai_admin_smoke import (
    AI_IDS,
    DEFAULT_ID,
    DEFAULT_NAME,
    DEFAULT_PASSWORD,
    MudClient,
    inspect_money,
    login,
    require,
)


PACKAGE = "com.zjmud.android"
ACTIVITY = f"{PACKAGE}/.MainActivity"
MUD_PROCESS = f"{PACKAGE}:mud"
FAULT_RECEIVER = f"{PACKAGE}/.FaultInjectionReceiver"
FAULT_ACTION = f"{PACKAGE}.DEBUG_KILL_MUD_PROCESS"
UI_FAULT_ACTION = f"{PACKAGE}.DEBUG_KILL_UI_PROCESS"
TEST_ROOM = "/d/standalone/ai_test"
TEST_ROOMS = {
    TEST_ROOM,
    "/d/standalone/ai_test_retreat",
    "/d/standalone/ai_test_blocked",
    "/d/standalone/ai_test_safe",
}
AI_SAVE_INTERVAL = 300
RECOVERY_RE = re.compile(r"AI_RECOVERY\s+([^\r\n]+)")
PAIR_RE = re.compile(r"(\w+)=([^\s]+)")
STABILITY_PLAYER_RE = re.compile(
    r"AI_STABILITY_PLAYER id=(\S+) loaded=(\d+) errors=(\d+) "
    r"action_failures=(\d+) route_failures=(\d+) relocations=(\d+) "
    r"respawns=(\d+) pending=(\d+) actions=(\d+)"
)
CASES = ("patrol", "rest", "social", "supplies", "combat", "savepoint", "migration", "travel")
LIFECYCLE_CASES = (
    "force-stop",
    "low-memory",
    "background",
    "webview",
    "doze",
    "reboot",
    "upgrade",
)
V16_RUNTIME_SHA = "e2c4e388d8d0b7d240ceb6708933a73fcd6db6f840156a1ead60599b4c5b8fd5"


def adb(serial: str, *args: str, check: bool = True) -> str:
    command = ["adb"] + (["-s", serial] if serial else []) + list(args)
    result = subprocess.run(command, check=check, capture_output=True)
    return result.stdout.decode("utf-8", errors="replace")


def run_as(serial: str, *args: str, check: bool = True) -> str:
    return adb(serial, "exec-out", "run-as", PACKAGE, *args, check=check)


def mud_pid(serial: str) -> int:
    return process_pid(serial, MUD_PROCESS)


def process_pid(serial: str, process: str) -> int:
    output = adb(serial, "shell", "pidof", process, check=False).strip()
    if not output:
        return 0
    return int(output.split()[0])


def wait_for_pid(
    serial: str,
    process: str,
    predicate,
    timeout: int = 30,
) -> int:
    deadline = time.monotonic() + timeout
    latest = 0
    while time.monotonic() < deadline:
        latest = process_pid(serial, process)
        if predicate(latest):
            return latest
        time.sleep(0.25)
    raise RuntimeError(f"process condition did not become true for {process}: pid={latest}")


def start_app(serial: str) -> None:
    adb(serial, "shell", "am", "start", "-W", "-n", ACTIVITY)


def broadcast_fault(serial: str, action: str) -> str:
    return adb(
        serial,
        "shell",
        "am",
        "broadcast",
        "-n",
        FAULT_RECEIVER,
        "-a",
        action,
        check=False,
    )


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


def ai_save_mtimes(serial: str) -> dict[str, int]:
    result: dict[str, int] = {}
    for ai_id in AI_IDS:
        path = f"files/runtime/current/data/user/a/{ai_id}.o"
        output = run_as(serial, "stat", "-c", "%Y", path).strip()
        if not output:
            raise RuntimeError(f"unable to stat AI save {path}")
        result[ai_id] = int(output)
    return result


def stability_actions(client: MudClient) -> int:
    output = client.command("aiplayer stability", 1.2)
    players = list(STABILITY_PLAYER_RE.finditer(output))
    if {match.group(1) for match in players} != set(AI_IDS):
        raise RuntimeError(f"unable to read stability actions:\n{output[-3000:]}")
    return sum(int(match.group(9)) for match in players)


def runtime_sha_from_apk(path: Path) -> str:
    if not path.is_file():
        raise RuntimeError(f"APK not found: {path}")
    with zipfile.ZipFile(path) as archive:
        data = archive.read("assets/runtime/manifest.properties").decode("ascii")
    for line in data.splitlines():
        if line.startswith("runtime_sha256="):
            return line.split("=", 1)[1]
    raise RuntimeError(f"runtime SHA missing from APK: {path}")


def wait_for_boot(serial: str, timeout: int = 180) -> None:
    command = ["adb"] + (["-s", serial] if serial else []) + ["wait-for-device"]
    subprocess.run(command, check=True, capture_output=True, timeout=timeout)
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        completed = adb(serial, "shell", "getprop", "sys.boot_completed", check=False).strip()
        if completed == "1":
            adb(serial, "shell", "wm", "dismiss-keyguard", check=False)
            return
        time.sleep(2)
    raise RuntimeError("Android did not finish booting")


def wake_device(serial: str) -> None:
    adb(serial, "shell", "dumpsys", "deviceidle", "unforce", check=False)
    power = adb(serial, "shell", "dumpsys", "power", check=False)
    if "mWakefulness=Awake" not in power:
        adb(serial, "shell", "input", "keyevent", "KEYCODE_WAKEUP", check=False)
    adb(serial, "shell", "wm", "dismiss-keyguard", check=False)


def grant_notifications(serial: str) -> None:
    for _ in range(5):
        adb(
            serial,
            "shell",
            "pm",
            "grant",
            PACKAGE,
            "android.permission.POST_NOTIFICATIONS",
            check=False,
        )
        package_dump = adb(serial, "shell", "dumpsys", "package", PACKAGE, check=False)
        if "android.permission.POST_NOTIFICATIONS: granted=true" in package_dump:
            return
        time.sleep(1)
    raise RuntimeError("unable to grant POST_NOTIFICATIONS after clearing app data")


def sleep_device(serial: str) -> None:
    power = adb(serial, "shell", "dumpsys", "power", check=False)
    if "mWakefulness=Awake" in power:
        adb(serial, "shell", "input", "keyevent", "KEYCODE_SLEEP")


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
    result = broadcast_fault(args.serial, FAULT_ACTION)
    deadline = time.monotonic() + 10
    while time.monotonic() < deadline and mud_pid(args.serial) == pid:
        time.sleep(0.2)
    if mud_pid(args.serial) == pid:
        raise RuntimeError(f"unable to kill {MUD_PROCESS} pid={pid}: {result}")

    # The service is START_NOT_STICKY. Recreate the app task after the targeted
    # service-process kill so MainActivity starts it with the deployed config.
    adb(args.serial, "shell", "am", "force-stop", PACKAGE)
    start_app(args.serial)
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
        if state.get("room") in TEST_ROOMS:
            raise RuntimeError(f"{ai_id} remained in the isolated test room")
        if state.get("fighting") == "0" and state.get("combat") in {
            "defend",
            "retreat",
            "trapped",
            "dead",
            "incapacitated",
        }:
            raise RuntimeError(f"{ai_id} retained stale combat policy state: {state}")


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
    if state.get("room") in TEST_ROOMS:
        raise RuntimeError(f"combat recovery retained the isolated room: {state}")
    if state.get("combat") != "idle":
        raise RuntimeError(f"combat recovery retained policy state: {state}")
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


TRAVEL_RECOVERY_RE = re.compile(r"AI_TRAVEL_RECOVERY\s+([^\r\n]+)")
TRAVEL_PAIR_RE = re.compile(r"(\w+)=([^\s]+)")


def travel_recovery(client: MudClient) -> dict[str, str]:
    output = client.command("aiplayer travel recovery ai_qingfeng", 0.7)
    match = TRAVEL_RECOVERY_RE.search(output)
    if not match:
        raise RuntimeError(f"unable to parse travel recovery state:\n{output[-2000:]}")
    state = dict(TRAVEL_PAIR_RE.findall(match.group(1)))
    if state.get("capability") != "v2.4":
        raise RuntimeError(f"travel contract version changed: {state}")
    return state


def wait_travel(
    client: MudClient,
    predicate,
    description: str,
    timeout: int = 60,
) -> dict[str, str]:
    deadline = time.monotonic() + timeout
    latest: dict[str, str] = {}
    while time.monotonic() < deadline:
        latest = travel_recovery(client)
        if predicate(latest):
            return latest
        time.sleep(0.5)
    raise RuntimeError(f"travel did not reach {description}: {latest}")


def case_travel(args: argparse.Namespace, client: MudClient) -> MudClient:
    active_modes = ("planned", "charged", "executing", "legacy")
    for mode in active_modes:
        reset(client, "ai_qingfeng")
        before = inspect_money(client, "ai_qingfeng")
        require(
            client.command(f"aiplayer travel recovery prepare ai_qingfeng {mode}", 0.8),
            f"已准备旅行恢复测试 {mode}",
            f"prepare travel {mode}",
        )
        prepared = travel_recovery(client)
        if mode == "charged" and prepared.get("state") != "charged":
            raise RuntimeError(f"charged checkpoint was not persisted: {prepared}")
        if mode == "executing" and prepared.get("state") != "executing":
            raise RuntimeError(f"executing checkpoint was not persisted: {prepared}")
        prepared_money = inspect_money(client, "ai_qingfeng")
        client = kill_and_restart(args, client)
        final = wait_travel(
            client,
            lambda state: state.get("state") == "arrived",
            f"arrived after {mode} recovery",
        )
        if final.get("schema") != "2":
            raise RuntimeError(f"travel recovery did not converge to schema 2: {final}")
        if final.get("node") != "wudang_gate" or final.get("edge_index") != "2":
            raise RuntimeError(f"travel recovery stopped at the wrong checkpoint: {final}")
        after = inspect_money(client, "ai_qingfeng")
        expected = before
        if after != expected:
            raise RuntimeError(
                f"travel recovery charged twice or skipped fare for {mode}: "
                f"before={before} prepared={prepared_money} after={after} expected={expected}"
            )
        if mode == "legacy" and final.get("recovery") != "migrated":
            raise RuntimeError(f"legacy travel checkpoint was not migrated: {final}")

    reset(client, "ai_qingfeng")
    require(
        client.command("aiplayer travel recovery prepare ai_qingfeng arrived", 0.8),
        "已准备旅行恢复测试 arrived",
        "prepare idempotent arrival",
    )
    arrived_before = travel_recovery(client)
    money_before = inspect_money(client, "ai_qingfeng")
    client = kill_and_restart(args, client)
    arrived_after = wait_travel(
        client,
        lambda state: state.get("state") == "arrived",
        "idempotent arrived checkpoint",
    )
    if arrived_after.get("schema") != "2":
        raise RuntimeError(f"arrived checkpoint did not retain schema 2: {arrived_after}")
    if arrived_after.get("event_seq") != arrived_before.get("event_seq"):
        raise RuntimeError(f"arrived recovery replayed movement events: {arrived_before} -> {arrived_after}")
    if inspect_money(client, "ai_qingfeng") != money_before:
        raise RuntimeError("arrived recovery changed the actor balance")

    failure_matrix = (
        ("removed", "route_removed"),
        ("disabled", "route_disabled"),
        ("invalid", "unknown_node"),
        ("future", "unsupported_schema"),
        ("legacy_invalid", "migration_failed"),
        ("legacy_removed", "legacy_cancelled"),
    )
    for mode, reason in failure_matrix:
        reset(client, "ai_qingfeng")
        require(
            client.command(f"aiplayer travel recovery prepare ai_qingfeng {mode}", 0.8),
            f"已准备旅行恢复测试 {mode}",
            f"prepare travel {mode}",
        )
        client = kill_and_restart(args, client)
        cancelled = wait_travel(
            client,
            lambda state: state.get("state") == "cancelled",
            f"cancelled {mode} travel",
        )
        if cancelled.get("schema") != "2":
            raise RuntimeError(f"{mode} travel did not converge to schema 2: {cancelled}")
        if cancelled.get("cancel_reason") != reason or cancelled.get("node") != "city_station":
            raise RuntimeError(f"{mode} travel did not safely converge: {cancelled}")
    return client


def lifecycle_force_stop(args: argparse.Namespace, client: MudClient) -> MudClient:
    reset(client, "ai_qingfeng")
    require(
        client.command("aiplayer activity run ai_qingfeng inn_rest", 0.6),
        "已强制安排角色活动",
        "start force-stop recovery activity",
    )
    wait_state(
        client,
        "ai_qingfeng",
        lambda state: state.get("step") == "resting",
        "resting before force-stop",
        180,
    )
    client.close()
    adb(args.serial, "shell", "am", "force-stop", PACKAGE)
    wait_for_pid(args.serial, MUD_PROCESS, lambda pid: pid == 0)
    wait_for_pid(args.serial, PACKAGE, lambda pid: pid == 0)
    start_app(args.serial)
    wait_for_saves(args.serial)
    client = connect(args)
    wait_finished(client, "ai_qingfeng", {"rested", "cancelled_timeout"}, 120)
    return client


def lifecycle_low_memory(args: argparse.Namespace, client: MudClient) -> MudClient:
    before_mud = mud_pid(args.serial)
    before_ui = process_pid(args.serial, PACKAGE)
    before_actions = stability_actions(client)
    if not before_mud or not before_ui:
        raise RuntimeError(f"processes missing before low-memory test: ui={before_ui} mud={before_mud}")
    adb(args.serial, "shell", "input", "keyevent", "KEYCODE_HOME")
    for process in (PACKAGE, MUD_PROCESS):
        output = adb(
            args.serial,
            "shell",
            "am",
            "send-trim-memory",
            process,
            "RUNNING_CRITICAL",
            check=False,
        )
        if "Error" in output or "Exception" in output:
            raise RuntimeError(f"unable to send critical trim to {process}: {output}")
    time.sleep(args.memory_wait_seconds)
    if mud_pid(args.serial) != before_mud:
        raise RuntimeError("MUD process changed under critical memory pressure")
    if process_pid(args.serial, PACKAGE) != before_ui:
        raise RuntimeError("UI process was unexpectedly reclaimed during trim-only phase")
    after_actions = stability_actions(client)
    if after_actions <= before_actions:
        raise RuntimeError(
            f"AI heartbeats stopped under critical memory pressure: {before_actions}->{after_actions}"
        )
    return client


def lifecycle_background(args: argparse.Namespace, client: MudClient) -> MudClient:
    expected_mud = mud_pid(args.serial)
    if not expected_mud:
        raise RuntimeError("MUD process missing before background test")
    for cycle in range(1, args.background_cycles + 1):
        adb(args.serial, "shell", "input", "keyevent", "KEYCODE_HOME")
        time.sleep(1)
        if mud_pid(args.serial) != expected_mud:
            raise RuntimeError(f"MUD process changed in background cycle {cycle}")
        start_app(args.serial)
        time.sleep(1)
        if mud_pid(args.serial) != expected_mud:
            raise RuntimeError(f"MUD process restarted in foreground cycle {cycle}")
        recovery(client, "ai_qingfeng")
    return client


def lifecycle_webview(args: argparse.Namespace, client: MudClient) -> MudClient:
    expected_mud = mud_pid(args.serial)
    if not expected_mud:
        raise RuntimeError("MUD process missing before WebView rebuild test")
    for cycle in range(1, args.webview_cycles + 1):
        old_ui = process_pid(args.serial, PACKAGE)
        if not old_ui:
            raise RuntimeError(f"UI process missing before WebView cycle {cycle}")
        result = broadcast_fault(args.serial, UI_FAULT_ACTION)
        wait_for_pid(args.serial, PACKAGE, lambda pid: pid == 0)
        if mud_pid(args.serial) != expected_mud:
            raise RuntimeError(f"MUD process changed while rebuilding WebView cycle {cycle}: {result}")
        start_app(args.serial)
        new_ui = wait_for_pid(args.serial, PACKAGE, lambda pid: pid > 0 and pid != old_ui)
        if new_ui == old_ui:
            raise RuntimeError(f"UI process was not rebuilt in cycle {cycle}")
        recovery(client, "ai_qingfeng")
    return client


def lifecycle_doze(args: argparse.Namespace, client: MudClient) -> MudClient:
    before_mud = mud_pid(args.serial)
    before_actions = stability_actions(client)
    before_mtimes = ai_save_mtimes(args.serial)
    client.close()
    adb(args.serial, "shell", "input", "keyevent", "KEYCODE_HOME")
    adb(args.serial, "shell", "dumpsys", "battery", "unplug", check=False)
    sleep_device(args.serial)
    idle_output = adb(
        args.serial,
        "shell",
        "dumpsys",
        "deviceidle",
        "force-idle",
        "deep",
        check=False,
    )
    if "deep idle" not in idle_output.lower():
        wake_device(args.serial)
        adb(args.serial, "shell", "dumpsys", "battery", "reset", check=False)
        raise RuntimeError(f"device did not enter forced deep idle: {idle_output}")
    started = time.monotonic()
    next_update = started + 60
    try:
        while time.monotonic() - started < args.screen_off_seconds:
            remaining = args.screen_off_seconds - (time.monotonic() - started)
            time.sleep(min(10, max(0, remaining)))
            if mud_pid(args.serial) != before_mud:
                raise RuntimeError("MUD process changed during screen-off Doze test")
            if time.monotonic() >= next_update:
                elapsed = int(time.monotonic() - started)
                print(f"[doze] {elapsed}/{args.screen_off_seconds}s process alive", flush=True)
                next_update += 60
    finally:
        wake_device(args.serial)
        adb(args.serial, "shell", "dumpsys", "battery", "reset", check=False)
    start_app(args.serial)
    client = connect(args)
    after_actions = stability_actions(client)
    if after_actions <= before_actions:
        raise RuntimeError(f"AI actions did not advance during Doze: {before_actions}->{after_actions}")
    if args.screen_off_seconds >= AI_SAVE_INTERVAL + 30:
        after_mtimes = ai_save_mtimes(args.serial)
        unchanged = [ai_id for ai_id in AI_IDS if after_mtimes[ai_id] <= before_mtimes[ai_id]]
        if unchanged:
            raise RuntimeError("AI periodic saves did not advance during Doze: " + ", ".join(unchanged))
    return client


def lifecycle_reboot(args: argparse.Namespace, client: MudClient) -> MudClient:
    before_mtimes = ai_save_mtimes(args.serial)
    client.close()
    adb(args.serial, "reboot")
    wait_for_boot(args.serial)
    if mud_pid(args.serial):
        raise RuntimeError("MUD service auto-started after reboot; launch must remain user-driven")
    wake_device(args.serial)
    start_app(args.serial)
    wait_for_saves(args.serial)
    client = connect(args)
    after_mtimes = ai_save_mtimes(args.serial)
    if any(after_mtimes[ai_id] < before_mtimes[ai_id] for ai_id in AI_IDS):
        raise RuntimeError("AI save timestamps moved backwards across reboot")
    return client


def lifecycle_upgrade(args: argparse.Namespace, client: MudClient) -> MudClient:
    legacy_apk = Path(args.legacy_apk).expanduser().resolve()
    current_apk = Path(args.current_apk).expanduser().resolve()
    legacy_sha = runtime_sha_from_apk(legacy_apk)
    current_sha = runtime_sha_from_apk(current_apk)
    if legacy_sha != V16_RUNTIME_SHA:
        raise RuntimeError(f"legacy APK is not the v1.6 baseline: {legacy_sha}")
    if current_sha == legacy_sha:
        raise RuntimeError("current APK unexpectedly contains the v1.6 runtime")

    client.close()
    adb(args.serial, "install", "-r", "-d", str(legacy_apk))
    adb(args.serial, "shell", "pm", "clear", PACKAGE)
    grant_notifications(args.serial)
    start_app(args.serial)
    wait_for_saves(args.serial)
    legacy_client = connect(args)
    try:
        reset(legacy_client, "ai_qingfeng")
        require(
            legacy_client.command("aiplayer activity run ai_qingfeng city_watch", 0.6),
            "已强制安排角色活动",
            "create v1.6 schema-less activity save",
        )
        require(legacy_client.command("aiplayer save", 1.0), "档案已保存", "save v1.6 AI state")
    finally:
        legacy_client.close()
    adb(args.serial, "shell", "am", "force-stop", PACKAGE)

    adb(args.serial, "install", "-r", str(current_apk))
    start_app(args.serial)
    wait_for_saves(args.serial)
    client = connect(args)
    state = recovery(client, "ai_qingfeng")
    if state.get("schema") != "2":
        raise RuntimeError(f"v1.6 activity did not migrate to schema 2: {state}")
    metrics = client.command("aiplayer metrics ai_qingfeng", 0.8)
    if not re.search(r"recovery migrations=[1-9]\d*", metrics):
        raise RuntimeError(f"v1.6 coverage upgrade did not record a migration: {metrics[-2000:]}")
    wait_for_saves(args.serial)
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
    parser.add_argument("--lifecycle", action="store_true", help="Run every Android lifecycle case")
    parser.add_argument(
        "--lifecycle-case",
        action="append",
        choices=LIFECYCLE_CASES,
        dest="lifecycle_cases",
    )
    parser.add_argument("--screen-off-seconds", type=int, default=3600)
    parser.add_argument("--memory-wait-seconds", type=int, default=30)
    parser.add_argument("--background-cycles", type=int, default=5)
    parser.add_argument("--webview-cycles", type=int, default=3)
    parser.add_argument("--legacy-apk", default="")
    parser.add_argument("--current-apk", default="app/build/outputs/apk/debug/app-debug.apk")
    parser.add_argument("--yes", action="store_true", help="Allow reboot and destructive upgrade tests")
    parser.add_argument("--output-dir", default="build/reports/ai-recovery")
    args = parser.parse_args()
    if (
        args.screen_off_seconds < 1
        or args.memory_wait_seconds < 1
        or args.background_cycles < 1
        or args.webview_cycles < 1
    ):
        parser.error("lifecycle durations and cycle counts must be positive")

    selected_recovery = list(args.cases or ())
    selected_lifecycle = list(args.lifecycle_cases or ())
    if args.lifecycle:
        selected_lifecycle = list(LIFECYCLE_CASES)
    if not selected_recovery and not selected_lifecycle:
        selected_recovery = list(CASES)
    disruptive = {"reboot", "upgrade"}.intersection(selected_lifecycle)
    if disruptive and not args.yes:
        parser.error("--yes is required for reboot and upgrade lifecycle cases")
    if "upgrade" in selected_lifecycle and not args.legacy_apk:
        parser.error("--legacy-apk is required for the upgrade lifecycle case")

    adb(args.serial, "get-state")
    results: list[dict[str, object]] = []
    client = connect(args)
    recovery_handlers = {
        "patrol": case_patrol,
        "rest": case_rest,
        "social": case_social,
        "supplies": case_supplies,
        "combat": case_combat,
        "savepoint": case_savepoint,
        "migration": case_migration,
        "travel": case_travel,
    }
    lifecycle_handlers = {
        "force-stop": lifecycle_force_stop,
        "low-memory": lifecycle_low_memory,
        "background": lifecycle_background,
        "webview": lifecycle_webview,
        "doze": lifecycle_doze,
        "reboot": lifecycle_reboot,
        "upgrade": lifecycle_upgrade,
    }
    selected = (
        [("recovery", case, recovery_handlers[case]) for case in selected_recovery]
        + [("lifecycle", case, lifecycle_handlers[case]) for case in selected_lifecycle]
    )
    try:
        for kind, case, handler in selected:
            started = time.monotonic()
            print(f"[{kind}:{case}] running", flush=True)
            client = handler(args, client)
            assert_common(client)
            results.append(
                {
                    "kind": kind,
                    "case": case,
                    "passed": True,
                    "seconds": round(time.monotonic() - started, 1),
                }
            )
            print(f"[{kind}:{case}] passed", flush=True)
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
        json.dumps(
            {
                "schema": 3,
                "screen_off_seconds": args.screen_off_seconds,
                "cases": results,
                "passed": len(results) == len(selected),
            },
            indent=2,
        )
        + "\n",
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
