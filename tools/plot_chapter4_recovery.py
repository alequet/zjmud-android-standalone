#!/usr/bin/env python3
"""Force-stop Chapter 4 checkpoints and verify deterministic reconstruction."""

from __future__ import annotations

import argparse
import sys

try:
    from .plot_chapter1_smoke import admin_call, call_int, require
    from .plot_chapter2_recovery import connect, force_stop_and_reconnect, require_value
    from .plot_chapter4_smoke import choose_action, reach_visitor, reset_case, verify_letters
except ImportError:
    from plot_chapter1_smoke import admin_call, call_int, require
    from plot_chapter2_recovery import connect, force_stop_and_reconnect, require_value
    from plot_chapter4_smoke import choose_action, reach_visitor, reset_case, verify_letters


def enter_from_safe_room(client, marker: str) -> None:
    admin_call(client, 'me->move("/d/city/wumiao")')
    require(client.command("plot act visitors_from_home_04 enter", 1.0), marker, "re-enter Chapter 4 instance")


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
        reset_case(client, "south", 100_000, True)
        reach_visitor(client, injury_case=True)
        print("[injured-visitor] checkpoint prepared", flush=True)

        client = force_stop_and_reconnect(args, client)
        require_value(client, 'me->query("plot/visitors_from_home_04/stage")', '"verify_letters"', "visitor stage recovery")
        enter_from_safe_room(client, "驿站后院")
        require_value(client, 'lu xiaoshuan->query("plot_condition")', '"injured"', "visitor injury recovery")
        if call_int(client, 'lu xiaoshuan->query("qi")') <= 0:
            raise RuntimeError("Stone Seven did not rebuild alive")
        print("[injured-visitor] passed", flush=True)

        verify_letters(client, wrong_first=False)
        expected = choose_action(client, "chase")
        print("[alive-courier] checkpoint prepared", flush=True)

        client = force_stop_and_reconnect(args, client)
        require_value(client, 'me->query("plot/visitors_from_home_04/stage")', '"catch_hua_yaozi"', "courier stage recovery")
        enter_from_safe_room(client, "花鹞子")
        require_value(client, 'hua yaozi->query("plot_combat/profile")', '"courier_scout"', "courier profile recovery")
        if call_int(client, 'hua yaozi->query("combat_exp")') != expected["enemy_exp"]:
            raise RuntimeError("Hua Yaozi EXP changed across recovery")
        if call_int(client, 'hua yaozi->query("plot_combat/skill_level")') != expected["skill"]:
            raise RuntimeError("Hua Yaozi skill changed across recovery")
        print("[alive-courier] passed", flush=True)

        require(client.command("plot act visitors_from_home_04 fight_hua"), "击败他后", "start courier fight")
        admin_call(client, "hua yaozi->die()", 0.8)
        require_value(client, 'me->query("plot/visitors_from_home_04/flags/hua_fate")', '"defeated"', "courier fate saved")
        print("[courier-defeated] checkpoint prepared", flush=True)

        client = force_stop_and_reconnect(args, client)
        require_value(client, 'me->query("plot/visitors_from_home_04/stage")', '"decode_invitation"', "courier defeat recovery")
        enter_from_safe_room(client, "旧驿路")
        require(client.command("plot act visitors_from_home_04 fight_hua"), "请帖需要核对", "resolved courier did not respawn")
        client.command("plot act visitors_from_home_04 decode_place_salt")
        print("[partial-decode] checkpoint prepared", flush=True)

        client = force_stop_and_reconnect(args, client)
        require_value(client, 'me->query("plot/visitors_from_home_04/flags/decoded_place")', '"old_salt_storehouse"', "partial decode recovery")
        require(client.command("plot act visitors_from_home_04 decode_tide_second"), "七号旧盐仓", "finish decode after recovery")
        require(client.command("plot act visitors_from_home_04 prepare_letters"), "1/2", "prepare first evidence")
        print("[evidence-prepared] checkpoint prepared", flush=True)

        client = force_stop_and_reconnect(args, client)
        require_value(client, 'me->query("plot/visitors_from_home_04/stage")', '"prepare_for_meeting"', "evidence stage recovery")
        require_value(client, 'me->query("plot/visitors_from_home_04/flags/evidence_prepared")', "= 1", "evidence count recovery")
        require(client.command("plot act visitors_from_home_04 prepare_invitation"), "2/2", "prepare second evidence")
        require(client.command("plot act visitors_from_home_04 confirm_preparation"), "不形成永久路线", "confirm evidence")
        score_before = call_int(client, 'me->query("score")')
        require(client.command("plot act visitors_from_home_04 chapter_close", 1.0), "第四章「故园来客」完成", "complete chapter")
        if call_int(client, 'me->query("score")') != score_before + 18:
            raise RuntimeError("chapter-four score mismatch")
        print("[completed-idempotency] checkpoint prepared", flush=True)

        client = force_stop_and_reconnect(args, client)
        require_value(client, 'me->query("plot/visitors_from_home_04/status")', '"completed"', "completed status recovery")
        score_after = call_int(client, 'me->query("score")')
        client.command("plot act visitors_from_home_04 chapter_close", 0.8)
        if call_int(client, 'me->query("score")') != score_after:
            raise RuntimeError("chapter-four completion duplicated score")
        print("[completed-idempotency] passed", flush=True)
        print("Fourth chapter force-stop recovery passed.")
        return 0
    finally:
        client.close()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError) as error:
        print(f"Fourth chapter force-stop recovery failed: {error}", file=sys.stderr)
        raise SystemExit(1)
