#!/usr/bin/env python3
"""Deterministic v2.x travel graph contract and route fixture checks."""

from __future__ import annotations

from collections import deque
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "tools" / "mudlib" / "ai-travel.c"
MAX_COST = 500
MAX_RISK = 3
MAX_TIME = 1800
CAPABILITY_VERSION = "v2.4"
SCHEMA_VERSION = 2
STATES = ("planned", "executing", "charged", "arrived", "cancelled")
CHECKPOINT_FIELDS = (
    "actor_id",
    "route_id",
    "state",
    "current_node",
    "edge_index",
    "charged_edges",
    "charged_total",
    "refunded",
    "retries",
    "started_at",
    "updated_at",
    "trigger",
    "event_seq",
    "last_event",
    "last_event_at",
    "cancel_reason",
)
CANCEL_REASONS = (
    "unknown_node",
    "unknown_route",
    "role_denied",
    "no_route",
    "path_limit",
    "budget_exceeded",
    "node_disabled",
    "edge_disabled",
    "actor_denied",
    "actor_unavailable",
    "wrong_start",
    "insufficient_funds",
    "charge_failed",
    "adapter_failed",
    "arrival_failed",
    "execution_timeout",
    "route_removed",
    "route_disabled",
    "invalid_checkpoint",
    "legacy_cancelled",
    "unsupported_schema",
    "migration_failed",
)
STATUS_FIELDS = (
    "actor",
    "capability",
    "schema",
    "state",
    "route",
    "node",
    "edge_index",
    "charged",
    "refunded",
    "retries",
    "trigger",
    "event_seq",
    "last_event",
    "cancel_reason",
)
RECOVERY_FIELDS = (
    "actor",
    "capability",
    "schema",
    "state",
    "route",
    "node",
    "edge_index",
    "charged",
    "refunded",
    "recovery",
    "migrated_from",
    "event_seq",
    "last_event",
    "cancel_reason",
)

EDGES = {
    "city_gate": [("city_station", 0, 0, 120), ("city_wharf", 0, 0, 180)],
    "city_station": [("wudang_foothill", 120, 1, 600)],
    "wudang_foothill": [("wudang_gate", 0, 0, 240)],
    "city_wharf": [("hangzhou_wharf", 80, 1, 720)],
    "hangzhou_wharf": [("shaolin_station", 150, 2, 780)],
    "shaolin_station": [("shaolin_gate", 0, 0, 120)],
}


def route(start: str, target: str) -> tuple[list[str], int, int, int]:
    queue: deque[tuple[str, list[str], int, int, int]] = deque(
        [(start, [start], 0, 0, 0)]
    )
    seen = {start}
    while queue:
        node, path, cost, risk, duration = queue.popleft()
        if node == target:
            return path, cost, risk, duration
        for next_node, edge_cost, edge_risk, edge_time in EDGES.get(node, []):
            if next_node in seen:
                continue
            new_cost = cost + edge_cost
            new_risk = risk + edge_risk
            new_duration = duration + edge_time
            if new_cost > MAX_COST or new_risk > MAX_RISK or new_duration > MAX_TIME:
                continue
            seen.add(next_node)
            queue.append((next_node, path + [next_node], new_cost, new_risk, new_duration))
    raise AssertionError(f"no bounded route from {start} to {target}")


def main() -> int:
    text = SOURCE.read_text(encoding="utf-8")
    required = (
        f"#define AI_TRAVEL_SCHEMA_VERSION {SCHEMA_VERSION}",
        f'#define AI_TRAVEL_CAPABILITY_VERSION "{CAPABILITY_VERSION}"',
        "mapping query_schema()",
        "mapping plan_route",
        "mapping plan_registered_route",
        "mapping create_travel_state",
        "mapping run_registered_travel",
        "mapping recover_travel",
        "int prepare_recovery_test",
        "private mapping migrate_travel_state",
        "private mapping resume_travel",
        "mapping run_schedule_period",
        "int prepare_auto_schedule_test",
        "int should_hold_for_schedule",
        "protected void heart_beat",
        "private mapping cancel_travel",
        "mapping execute_adapter",
        "mapping selftest",
        '"test" : ([',
        '"test_city_to_wudang" : ([',
        '"qingfeng_city_to_wudang" : ([',
        '"qingfeng_wudang_to_city" : ([',
        "mapping schedules = ([",
        '#define AI_TRAVEL_ACTOR "ai_qingfeng"',
        '"side_effects" : 0',
        '"budget_exceeded"',
        '"route_removed"',
        '"route_disabled"',
        '"legacy_cancelled"',
        '"migration_failed"',
        'return "unknown_node";',
        "sort_array(keys(edges)",
    )
    for marker in required:
        assert marker in text, marker

    for state in STATES:
        assert f'"{state}"' in text, state
    for field in CHECKPOINT_FIELDS:
        assert f'"{field}"' in text, field
    for reason in CANCEL_REASONS:
        assert f'"{reason}"' in text, reason

    command = (ROOT / "tools" / "mudlib" / "aiplayer.c").read_text(encoding="utf-8")
    status_format = " ".join(
        part.strip().strip('"')
        for part in (
            "AI_TRAVEL_STATUS actor=%s capability=%s schema=%d state=%s route=%s",
            "node=%s edge_index=%d charged=%d refunded=%d retries=%d trigger=%s",
            "event_seq=%d last_event=%s cancel_reason=%s\\n",
        )
    )
    recovery_format = " ".join(
        part.strip().strip('"')
        for part in (
            "AI_TRAVEL_RECOVERY actor=%s capability=%s schema=%d state=%s route=%s",
            "node=%s edge_index=%d charged=%d refunded=%d recovery=%s",
            "migrated_from=%d event_seq=%d last_event=%s cancel_reason=%s\\n",
        )
    )
    normalized_command = " ".join(command.replace('"', "").split())
    assert " ".join(status_format.split()) in normalized_command
    assert " ".join(recovery_format.split()) in normalized_command
    assert STATUS_FIELDS == tuple(
        token.split("=", 1)[0] for token in status_format.split() if "=" in token
    )
    assert RECOVERY_FIELDS == tuple(
        token.split("=", 1)[0] for token in recovery_format.split() if "=" in token
    )

    path, cost, risk, duration = route("city_gate", "wudang_gate")
    assert path == ["city_gate", "city_station", "wudang_foothill", "wudang_gate"]
    assert (cost, risk, duration) == (120, 1, 960)

    path, cost, risk, duration = route("city_gate", "shaolin_gate")
    assert path == [
        "city_gate",
        "city_wharf",
        "hangzhou_wharf",
        "shaolin_station",
        "shaolin_gate",
    ]
    assert (cost, risk, duration) == (230, 3, 1800)
    print("AI v2.x travel fixture checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
