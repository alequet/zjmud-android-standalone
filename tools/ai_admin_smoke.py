#!/usr/bin/env python3
"""Run standalone AI administrator smoke checks over the loopback MUD port."""

from __future__ import annotations

import argparse
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
    args = parser.parse_args()

    client = MudClient(args.host, args.port)
    scenarios_paused = False
    behaviors_running = False
    try:
        login(client, args.account, args.password, args.name)
        require(client.command("aiplayer status", 1.5), "AI 玩家正在运行", "aiplayer status")
        require(client.command("aiplayer validate", 3.0), "AI 配置校验通过", "aiplayer validate")
        selftest = client.command("aiplayer selftest", 3.0)
        for ai_id in AI_IDS:
            require(selftest, f"{ai_id}: 通过", "aiplayer selftest")

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
        client.close()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError) as error:
        print(f"AI administrator smoke checks failed: {error}", file=sys.stderr)
        raise SystemExit(1)
