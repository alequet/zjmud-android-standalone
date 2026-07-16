#!/usr/bin/env python3
"""Run a timed standalone AI stability test and write JSON/CSV reports."""

from __future__ import annotations

import argparse
import csv
import json
import re
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path

from ai_admin_smoke import AI_IDS, DEFAULT_ID, DEFAULT_NAME, DEFAULT_PASSWORD, MudClient, login


PACKAGE = "com.zjmud.android"
STABILITY_RE = re.compile(
    r"AI_STABILITY objects=(\d+) players=(\d+) profiles=(\d+) paused=(\d+)"
)
PLAYER_RE = re.compile(
    r"AI_STABILITY_PLAYER id=(\S+) loaded=(\d+) errors=(\d+) "
    r"action_failures=(\d+) route_failures=(\d+) relocations=(\d+) "
    r"respawns=(\d+) pending=(\d+) actions=(\d+)"
)
LOG_ERROR_RE = re.compile(
    rb"Bad argument|Denied write permission in save_object|"
    rb"Can.t load objects when no effective user|Unable to (?:save initial login|"
    rb"prepare login uid|export AI account uid|activate AI account uid|"
    rb"initialize runtime uid)|AI_(?:INIT|SECURITY)_DEBUG|ai-save-debug"
)


def adb(serial: str, *args: str, text: bool = True) -> str | bytes:
    command = ["adb"]
    if serial:
        command += ["-s", serial]
    command += list(args)
    result = subprocess.run(command, check=True, capture_output=True)
    if text:
        return result.stdout.decode("utf-8", errors="replace")
    return result.stdout


def run_as(serial: str, *args: str, text: bool = True) -> str | bytes:
    return adb(serial, "exec-out", "run-as", PACKAGE, *args, text=text)


def read_logs(serial: str) -> dict[str, bytes]:
    chunks: dict[str, bytes] = {}
    for path in (
        "files/runtime/current/android-driver.log",
        "files/runtime/current/log/debug.log",
        "files/runtime/current/log/ai-player",
    ):
        result = subprocess.run(
            (["adb"] + (["-s", serial] if serial else []) +
             ["exec-out", "run-as", PACKAGE, "cat", path]),
            check=False,
            capture_output=True,
        )
        if result.returncode == 0:
            chunks[path] = result.stdout
    return chunks


def list_saves(serial: str, kind: str) -> dict[str, tuple[int, int]]:
    root = f"files/runtime/current/data/{kind}/a"
    output = run_as(serial, "find", root, "-maxdepth", "1", "-type", "f", "-name", "ai_*.o")
    result: dict[str, tuple[int, int]] = {}
    for path in str(output).splitlines():
        path = path.strip()
        if not path:
            continue
        stat = str(run_as(serial, "stat", "-c", "%Y %s", path)).strip().split()
        if len(stat) != 2:
            raise RuntimeError(f"Unable to stat AI save: {path}")
        result[Path(path).stem] = (int(stat[0]), int(stat[1]))
    return result


def process_memory(serial: str) -> tuple[int, int]:
    output = str(adb(serial, "shell", "dumpsys", "meminfo", PACKAGE))
    match = re.search(r"TOTAL PSS:\s+(\d+)\s+TOTAL RSS:\s+(\d+)", output)
    if not match:
        match = re.search(r"^\s*TOTAL\s+(\d+).*?\s(\d+)\s+\d+\s+\d+\s*$", output, re.MULTILINE)
    if not match:
        raise RuntimeError("Unable to parse dumpsys meminfo")
    return int(match.group(1)), int(match.group(2))


def parse_stability(output: str) -> tuple[dict[str, int], dict[str, dict[str, int]]]:
    summary_match = STABILITY_RE.search(output)
    if not summary_match:
        raise RuntimeError(f"Unable to parse aiplayer stability output:\n{output[-3000:]}")
    summary = dict(zip(("objects", "players", "profiles", "paused"), map(int, summary_match.groups())))
    players: dict[str, dict[str, int]] = {}
    keys = (
        "loaded", "errors", "action_failures", "route_failures", "relocations",
        "respawns", "pending", "actions",
    )
    for match in PLAYER_RE.finditer(output):
        players[match.group(1)] = dict(zip(keys, map(int, match.groups()[1:])))
    if set(players) != set(AI_IDS):
        raise RuntimeError(f"Expected stability metrics for {AI_IDS}, got {sorted(players)}")
    return summary, players


def significant_sustained_growth(values: list[int]) -> bool:
    if len(values) < 6:
        return False
    tail = values[len(values) // 2:]
    nondecreasing = all(current >= previous for previous, current in zip(tail, tail[1:]))
    growth = tail[-1] - tail[0]
    return nondecreasing and growth > max(250, tail[0] // 10)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--duration", type=int, default=1800, help="Test duration in seconds (default: 1800)")
    parser.add_argument("--interval", type=int, default=60, help="Sampling interval in seconds (default: 60)")
    parser.add_argument("--serial", default="")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=3000)
    parser.add_argument("--account", default=DEFAULT_ID)
    parser.add_argument("--password", default=DEFAULT_PASSWORD)
    parser.add_argument("--name", default=DEFAULT_NAME)
    parser.add_argument("--output-dir", default="build/reports/ai-stability")
    args = parser.parse_args()
    if args.duration < 1 or args.interval < 1 or args.interval > args.duration:
        parser.error("duration and interval must be positive, and interval must not exceed duration")

    expected = set(AI_IDS)
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    started = datetime.now().strftime("%Y%m%d-%H%M%S")
    json_path = output_dir / f"stability-{started}.json"
    csv_path = output_dir / f"stability-{started}.csv"

    adb(args.serial, "get-state")
    adb(args.serial, "forward", f"tcp:{args.port}", "tcp:3000")
    baseline_logs = read_logs(args.serial)
    initial_login = list_saves(args.serial, "login")
    initial_user = list_saves(args.serial, "user")
    failures: list[str] = []
    samples: list[dict[str, object]] = []

    if set(initial_login) != expected or set(initial_user) != expected:
        failures.append(
            f"Initial AI saves are incomplete: login={sorted(initial_login)} user={sorted(initial_user)}"
        )

    client = MudClient(args.host, args.port)
    try:
        login(client, args.account, args.password, args.name)
        client.command("aiplayer resume", 1.0)
        baseline_summary, baseline_players = parse_stability(client.command("aiplayer stability", 2.0))
        deadline = time.monotonic() + args.duration
        sample_number = 0
        while True:
            sample_number += 1
            now = time.time()
            summary, players = parse_stability(client.command("aiplayer stability", 2.0))
            login_saves = list_saves(args.serial, "login")
            user_saves = list_saves(args.serial, "user")
            pss_kb, rss_kb = process_memory(args.serial)
            sample = {
                "sample": sample_number,
                "timestamp": int(now),
                "elapsed_seconds": max(0, args.duration - max(0, int(deadline - time.monotonic()))),
                "objects": summary["objects"],
                "players": summary["players"],
                "profiles": summary["profiles"],
                "paused": summary["paused"],
                "pss_kb": pss_kb,
                "rss_kb": rss_kb,
                "login_saves": sorted(login_saves),
                "user_saves": sorted(user_saves),
                "login_mtimes": {key: value[0] for key, value in login_saves.items()},
                "user_mtimes": {key: value[0] for key, value in user_saves.items()},
                "metrics": players,
            }
            samples.append(sample)
            print(
                f"[{sample_number:02d}] elapsed={sample['elapsed_seconds']}s "
                f"objects={summary['objects']} rss={rss_kb}KiB "
                f"actions={sum(item['actions'] for item in players.values())} "
                f"errors={sum(item['errors'] for item in players.values())}",
                flush=True,
            )
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                break
            time.sleep(min(args.interval, remaining))
    finally:
        client.close()

    final_login = list_saves(args.serial, "login")
    final_user = list_saves(args.serial, "user")
    if set(final_login) != expected or set(final_user) != expected:
        failures.append(f"Final AI saves are incomplete: login={sorted(final_login)} user={sorted(final_user)}")
    if set(final_login) != set(final_user):
        failures.append("Login and player save sets differ, indicating a half-initialized AI account")
    unchanged = [ai_id for ai_id in AI_IDS if final_user.get(ai_id) == initial_user.get(ai_id)]
    if args.duration >= 360 and unchanged:
        failures.append(f"Periodic player saves did not update: {', '.join(unchanged)}")

    final_players = samples[-1]["metrics"]
    assert isinstance(final_players, dict)
    for ai_id in AI_IDS:
        baseline = baseline_players[ai_id]
        current = final_players[ai_id]
        for metric in ("errors", "action_failures"):
            delta = current[metric] - baseline[metric]
            if delta:
                failures.append(f"{ai_id} added {delta} {metric}")
        if current["loaded"] != 1:
            failures.append(f"{ai_id} was not loaded at the final sample")
    if samples[-1]["players"] != 5 or samples[-1]["profiles"] != 5:
        failures.append("Final daemon state does not contain exactly five loaded players and profiles")
    object_values = [int(sample["objects"]) for sample in samples]
    if significant_sustained_growth(object_values):
        failures.append(
            f"Loaded object count grew continuously in the second half: {object_values[len(object_values)//2]} -> {object_values[-1]}"
        )

    final_logs = read_logs(args.serial)
    new_logs = b"\n".join(
        content[len(baseline_logs.get(path, b"")):]
        if content.startswith(baseline_logs.get(path, b"")) else content
        for path, content in final_logs.items()
    )
    log_matches = sorted({match.group(0).decode("ascii", errors="replace") for match in LOG_ERROR_RE.finditer(new_logs)})
    if log_matches:
        failures.append("New runtime log errors: " + ", ".join(log_matches))

    report = {
        "started_at": started,
        "duration_seconds": args.duration,
        "interval_seconds": args.interval,
        "passed": not failures,
        "failures": failures,
        "baseline_summary": baseline_summary,
        "baseline_metrics": baseline_players,
        "samples": samples,
        "object_growth": object_values[-1] - object_values[0],
        "rss_growth_kb": int(samples[-1]["rss_kb"]) - int(samples[0]["rss_kb"]),
        "new_log_error_markers": log_matches,
    }
    json_path.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    with csv_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow(("sample", "timestamp", "elapsed_seconds", "objects", "pss_kb", "rss_kb", "actions", "errors", "action_failures", "route_failures", "relocations"))
        for sample in samples:
            metrics = sample["metrics"]
            writer.writerow((
                sample["sample"], sample["timestamp"], sample["elapsed_seconds"], sample["objects"],
                sample["pss_kb"], sample["rss_kb"],
                sum(item["actions"] for item in metrics.values()),
                sum(item["errors"] for item in metrics.values()),
                sum(item["action_failures"] for item in metrics.values()),
                sum(item["route_failures"] for item in metrics.values()),
                sum(item["relocations"] for item in metrics.values()),
            ))

    print(f"JSON report: {json_path}")
    print(f"CSV report: {csv_path}")
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        return 1
    print(
        f"AI stability test passed: object growth={report['object_growth']}, "
        f"RSS growth={report['rss_growth_kb']} KiB."
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"AI stability test failed: {error}", file=sys.stderr)
        raise SystemExit(1)
