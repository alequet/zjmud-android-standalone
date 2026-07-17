#!/usr/bin/env python3
"""Run standalone AI administrator smoke checks over the loopback MUD port."""

from __future__ import annotations

import argparse
import re
import select
import socket
import sys
import time


ENCODING = "gb18030"
HANDSHAKE = "zjmDMaIpOvxdb\n"
DEFAULT_ID = "codexaiqa"
DEFAULT_PASSWORD = "codex-ai-qa"
DEFAULT_NAME = "春风明月"
AI_IDS = ("ai_qingfeng", "ai_songlan", "ai_wantang", "ai_yanqiu", "ai_zhiyuan")
PATROL_ACTIVITIES = {
    "ai_qingfeng": ("city_watch", "巡视城中"),
    "ai_songlan": ("lake_walk", "沿湖巡游"),
    "ai_wantang": ("gather_news", "打听消息"),
    "ai_yanqiu": ("mountain_watch", "巡视山路"),
    "ai_zhiyuan": ("guard_gate", "照看山门"),
}
REST_ACTIVITIES = {
    "ai_qingfeng": ("inn_rest", "客栈歇脚"),
    "ai_songlan": ("lakeside_rest", "湖畔歇脚"),
    "ai_wantang": ("inn_rest", "客栈休整"),
    "ai_yanqiu": ("quiet_rest", "静坐调息"),
    "ai_zhiyuan": ("gate_rest", "山门静息"),
}
DEFENSE_SCENARIOS = (
    ("ai_qingfeng", "defend", ("decision=defend", "attempted=0")),
    ("ai_wantang", "retreat", ("decision=retreat", "attempted=1", "retreated=1")),
    ("ai_yanqiu", "noexit", ("decision=retreat", "attempted=0", "trapped=1")),
    ("ai_zhiyuan", "blocked", ("decision=retreat", "attempted=1", "attempts=3", "trapped=1")),
    ("ai_songlan", "unconscious", ("incapacitated=1",)),
    ("ai_qingfeng", "death", ("death=1", "respawned=1")),
)
CREATE_CHARACTER = "\x1b0000008"
LOGIN_SUCCEEDED = "\x1b0000007"
WORLD_READY = "\x1b002"
MONEY_RE = re.compile(r"携带货币：(\d+) 文")


class MudClient:
    def __init__(self, host: str, port: int) -> None:
        self.sock = socket.create_connection((host, port), timeout=5)
        self.sock.setblocking(False)

    def close(self) -> None:
        self.sock.close()

    def send(self, line: str) -> None:
        self.sock.sendall((line + "\n").encode(ENCODING))

    def read_for(self, seconds: float) -> str:
        deadline = time.monotonic() + seconds
        chunks: list[bytes] = []
        while time.monotonic() < deadline:
            readable, _, _ = select.select([self.sock], [], [], min(0.25, deadline - time.monotonic()))
            if not readable:
                continue
            data = self.sock.recv(65536)
            if not data:
                break
            chunks.append(data)
        return b"".join(chunks).decode(ENCODING, errors="replace")

    def command(self, command: str, wait: float = 1.0) -> str:
        self.send(command)
        return self.read_for(wait)


def require(output: str, marker: str, context: str) -> None:
    if marker not in output:
        raise RuntimeError(f"{context}: expected {marker!r}\n{output[-3000:]}")


def inspect_money(client: MudClient, ai_id: str) -> int:
    output = client.command(f"aiplayer inspect {ai_id}", 0.8)
    match = MONEY_RE.search(output)
    if not match:
        raise RuntimeError(f"unable to read carried money for {ai_id}:\n{output[-3000:]}")
    return int(match.group(1))


def login(client: MudClient, account: str, password: str, name: str) -> str:
    client.read_for(1.0)
    client.send(HANDSHAKE.rstrip("\n"))
    client.send("")
    client.read_for(0.5)
    client.send(f"{account}║{password}║local║local@zjmud.invalid")
    output = client.read_for(2.0)
    if CREATE_CHARACTER in output:
        client.send(f"男性║001║{name}")
        output += client.read_for(4.0)
    require(output, LOGIN_SUCCEEDED, "administrator login")
    for _ in range(30):
        if WORLD_READY in output:
            break
        output += client.read_for(1.0)
    require(output, WORLD_READY, "administrator world entry")
    return output


def wait_for_activities(
    client: MudClient,
    expected: dict[str, tuple[str, str]],
    outcome: str,
    timeout: int,
) -> None:
    pending = dict(expected)
    deadline = time.monotonic() + timeout
    while pending and time.monotonic() < deadline:
        for ai_id, (_, activity_name) in tuple(pending.items()):
            inspected = client.command(f"aiplayer inspect {ai_id}", 0.5)
            if f"最近活动：{activity_name}" in inspected and f"结果：{outcome}" in inspected:
                del pending[ai_id]
        if pending:
            time.sleep(2)
    if pending:
        raise RuntimeError(f"activities did not finish with {outcome}: {', '.join(sorted(pending))}")


def wait_for_travel_state(
    client: MudClient,
    ai_id: str,
    marker: str,
    timeout: int = 30,
) -> str:
    deadline = time.monotonic() + timeout
    output = ""
    while time.monotonic() < deadline:
        output = client.command(f"aiplayer travel status {ai_id}", 0.8)
        if marker in output:
            return output
        time.sleep(1)
    raise RuntimeError(f"travel state did not reach {marker!r}:\n{output[-3000:]}")


def run_behavior_checks(client: MudClient) -> None:
    for ai_id in AI_IDS:
        client.command(f"aiplayer reset {ai_id}", 0.5)
    for ai_id, (activity_id, _) in PATROL_ACTIVITIES.items():
        require(
            client.command(f"aiplayer activity run {ai_id} {activity_id}", 0.5),
            "已强制安排角色活动",
            f"start patrol {ai_id}",
        )
    wait_for_activities(client, PATROL_ACTIVITIES, "patrol_completed", 360)

    for ai_id in AI_IDS:
        client.command(f"aiplayer reset {ai_id}", 0.5)
    for ai_id, (activity_id, _) in REST_ACTIVITIES.items():
        require(
            client.command(f"aiplayer activity run {ai_id} {activity_id}", 0.5),
            "已强制安排角色活动",
            f"start rest {ai_id}",
        )
    wait_for_activities(client, REST_ACTIVITIES, "rested", 180)

    client.command("aiplayer reset ai_qingfeng", 0.5)
    client.command("aiplayer reset ai_wantang", 0.5)
    require(
        client.command("aiplayer activity run ai_qingfeng meet_wantang", 0.5),
        "已强制安排角色活动",
        "start social meeting",
    )
    wait_for_activities(
        client,
        {"ai_qingfeng": ("meet_wantang", "与苏晚棠碰面")},
        "met_ai_wantang",
        180,
    )


def run_defense_scenarios(client: MudClient) -> None:
    for ai_id, mode, markers in DEFENSE_SCENARIOS:
        client.command(f"aiplayer reset {ai_id}", 0.5)
        started = ""
        for _ in range(15):
            started = client.command(f"aiplayer scenario defense {ai_id} {mode}", 0.6)
            if f"已启动有限自卫场景 {mode}" in started:
                break
            time.sleep(1)
        require(started, f"已启动有限自卫场景 {mode}", f"start defense {mode}")

        deadline = time.monotonic() + 20
        status = ""
        while time.monotonic() < deadline:
            status = client.command(f"aiplayer scenario status {ai_id}", 0.5)
            if f"mode={mode} status=passed" in status:
                break
            if f"mode={mode} status=failed" in status:
                raise RuntimeError(f"defense scenario {mode} failed:\n{status[-3000:]}")
            time.sleep(0.5)
        require(status, f"mode={mode} status=passed", f"defense scenario {mode}")
        require(status, "restored=1", f"defense restore {mode}")
        for marker in markers:
            require(status, marker, f"defense postcondition {mode}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=3000)
    parser.add_argument("--account", default=DEFAULT_ID)
    parser.add_argument("--password", default=DEFAULT_PASSWORD)
    parser.add_argument("--name", default=DEFAULT_NAME)
    parser.add_argument(
        "--scenarios",
        action="store_true",
        help="Run the six-mode finite-defense matrix and the supply adapter scenario",
    )
    parser.add_argument("--behaviors", action="store_true", help="Run patrol, rest, and social activity checks")
    parser.add_argument(
        "--travel",
        action="store_true",
        help="Run the v2.1 single-actor registered travel closure",
    )
    args = parser.parse_args()

    client = MudClient(args.host, args.port)
    scenarios_paused = False
    behaviors_running = False
    travel_paused = False
    try:
        login(client, args.account, args.password, args.name)
        require(client.command("aiplayer status", 1.5), "AI 玩家正在运行", "aiplayer status")
        require(client.command("aiplayer validate", 3.0), "AI 配置校验通过", "aiplayer validate")
        require(
            client.command("aiplayer travel validate", 1.5),
            "AI_TRAVEL_VALIDATE status=passed",
            "aiplayer travel validate",
        )
        require(
            client.command("aiplayer travel selftest", 1.5),
            "AI_TRAVEL_SELFTEST capability=v2.4 schema=2 status=passed",
            "aiplayer travel selftest",
        )
        travel_route = client.command(
            "aiplayer travel route test_city_to_wudang", 1.5
        )
        require(travel_route, "state=planned", "aiplayer travel route")
        require(travel_route, "cost=120 risk=1 time=960", "aiplayer travel route budgets")
        selftest = client.command("aiplayer selftest", 3.0)
        for ai_id in AI_IDS:
            require(selftest, f"{ai_id}: 通过", "aiplayer selftest")

        if args.travel:
            client.command("aiplayer pause", 0.8)
            travel_paused = True
            client.command("aiplayer reset ai_qingfeng", 1.0)
            before_arrival = inspect_money(client, "ai_qingfeng")
            travel = client.command(
                "aiplayer travel run qingfeng_city_to_wudang seed", 2.0
            )
            require(travel, "state=arrived", "v2.1 travel arrival")
            require(travel, "node=wudang_gate", "v2.1 travel destination")
            require(travel, "charged=120 refunded=0 retries=0", "v2.1 travel budget")
            after_arrival = inspect_money(client, "ai_qingfeng")
            expected_arrival = before_arrival
            if after_arrival != expected_arrival:
                raise RuntimeError(
                    f"v2.1 travel charged wrong amount: before={before_arrival} "
                    f"after={after_arrival} expected={expected_arrival}"
                )
            travel_status = client.command("aiplayer travel status ai_qingfeng", 1.0)
            require(travel_status, "capability=v2.4 schema=2 state=arrived", "v2.1 travel status")
            require(travel_status, "route=qingfeng_city_to_wudang", "v2.1 travel checkpoint")

            client.command("aiplayer reset ai_qingfeng", 1.0)
            before_insufficient = inspect_money(client, "ai_qingfeng")
            insufficient = client.command(
                "aiplayer travel run qingfeng_city_to_wudang insufficient", 2.0
            )
            require(insufficient, "state=cancelled", "v2.1 insufficient funds")
            require(insufficient, "node=city_station", "v2.1 insufficient safe return")
            require(
                insufficient,
                "charged=0 refunded=0 retries=0 cancel_reason=insufficient_funds",
                "v2.1 insufficient settlement",
            )
            after_insufficient = inspect_money(client, "ai_qingfeng")
            if after_insufficient != before_insufficient:
                raise RuntimeError(
                    f"v2.1 insufficient funds changed balance: before={before_insufficient} "
                    f"after={after_insufficient}"
                )

            client.command("aiplayer reset ai_qingfeng", 1.0)
            before_disabled = inspect_money(client, "ai_qingfeng")
            disabled = client.command(
                "aiplayer travel run qingfeng_city_to_wudang edge_disabled", 2.0
            )
            require(disabled, "state=cancelled", "v2.1 disabled edge")
            require(disabled, "node=city_station", "v2.1 disabled safe return")
            require(
                disabled,
                "charged=120 refunded=1 retries=0 cancel_reason=edge_disabled",
                "v2.1 disabled refund",
            )
            after_disabled = inspect_money(client, "ai_qingfeng")
            expected_disabled = before_disabled
            if after_disabled != expected_disabled:
                raise RuntimeError(
                    f"v2.1 disabled edge refund mismatch: before={before_disabled} "
                    f"after={after_disabled} expected={expected_disabled}"
                )

            client.command("aiplayer reset ai_qingfeng", 1.0)
            before_schedule = inspect_money(client, "ai_qingfeng")
            outbound = client.command(
                "aiplayer travel schedule ai_qingfeng day seed", 2.0
            )
            require(outbound, "state=arrived", "v2.2 outbound schedule")
            require(outbound, "route=qingfeng_city_to_wudang", "v2.2 outbound whitelist")
            require(outbound, "node=wudang_gate", "v2.2 outbound destination")
            require(outbound, "trigger=schedule_test_day event_seq=4", "v2.2 outbound events")
            if inspect_money(client, "ai_qingfeng") != before_schedule:
                raise RuntimeError("v2.2 outbound test funding changed actor balance")

            inbound = client.command(
                "aiplayer travel schedule ai_qingfeng morning seed", 2.0
            )
            require(inbound, "state=arrived", "v2.2 inbound schedule")
            require(inbound, "route=qingfeng_wudang_to_city", "v2.2 inbound whitelist")
            require(inbound, "node=city_station", "v2.2 inbound destination")
            require(inbound, "trigger=schedule_test_morning event_seq=4", "v2.2 inbound events")
            if inspect_money(client, "ai_qingfeng") != before_schedule:
                raise RuntimeError("v2.2 inbound test funding changed actor balance")

            denied_schedule = client.command(
                "aiplayer travel schedule ai_wantang day seed", 1.0
            )
            require(denied_schedule, "state=cancelled", "v2.2 unapproved actor")
            require(
                denied_schedule,
                "actor=ai_wantang period=day",
                "v2.2 unapproved actor identity",
            )
            require(
                denied_schedule,
                "cancel_reason=unknown_route",
                "v2.2 unapproved actor denial",
            )

            client.command("aiplayer reset ai_qingfeng", 1.0)
            before_auto = inspect_money(client, "ai_qingfeng")
            require(
                client.command("aiplayer travel auto ai_qingfeng day", 1.0),
                "已准备自动旅行日程测试 day",
                "v2.2 prepare auto schedule",
            )
            client.command("aiplayer resume", 0.8)
            travel_paused = False
            auto_status = wait_for_travel_state(
                client,
                "ai_qingfeng",
                "state=arrived route=qingfeng_city_to_wudang",
                30,
            )
            require(auto_status, "trigger=schedule_day", "v2.2 automatic trigger")
            require(auto_status, "event_seq=4 last_event=arrived", "v2.2 automatic events")
            if inspect_money(client, "ai_qingfeng") != before_auto:
                raise RuntimeError("v2.2 automatic test funding changed actor balance")
            client.command("aiplayer pause", 0.8)
            travel_paused = True
            client.command("aiplayer reset ai_qingfeng", 1.0)
            client.command("aiplayer resume", 0.8)
            travel_paused = False

        if args.behaviors:
            behaviors_running = True
            run_behavior_checks(client)
            behaviors_running = False

        if args.scenarios:
            client.command("aiplayer pause", 1.0)
            scenarios_paused = True
            run_defense_scenarios(client)

            client.command("aiplayer reset ai_wantang", 1.0)
            supplies = ""
            for _ in range(15):
                supplies = client.command("aiplayer activity supplies ai_wantang seed", 1.0)
                if "已安排补给活动并注入一次测试银两" in supplies:
                    break
                time.sleep(1)
            require(supplies, "已安排补给活动并注入一次测试银两", "start supply scenario")
            time.sleep(10)
            metrics = client.command("aiplayer metrics ai_wantang", 1.5)
            require(metrics, "adapters attempts=", "supply metrics")
            client.command("aiplayer resume", 1.0)
            scenarios_paused = False

        print("AI administrator smoke checks passed.")
        return 0
    finally:
        if behaviors_running:
            for ai_id in AI_IDS:
                try:
                    client.command(f"aiplayer reset {ai_id}", 0.5)
                except OSError:
                    break
        if scenarios_paused:
            try:
                client.command("aiplayer resume", 1.0)
            except OSError:
                pass
        if travel_paused:
            try:
                client.command("aiplayer reset ai_qingfeng", 0.5)
                client.command("aiplayer resume", 0.5)
            except OSError:
                pass
        client.close()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError) as error:
        print(f"AI administrator smoke checks failed: {error}", file=sys.stderr)
        raise SystemExit(1)
