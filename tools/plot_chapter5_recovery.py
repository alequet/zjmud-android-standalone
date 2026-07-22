#!/usr/bin/env python3
"""Force-stop Chapter 5 checkpoints and verify deterministic reconstruction."""

from __future__ import annotations

import argparse
import sys

try:
    from .plot_chapter1_smoke import admin_call, call_int, require
    from .plot_chapter2_recovery import connect, force_stop_and_reconnect, require_value
    from .plot_chapter5_smoke import reset_case
except ImportError:
    from plot_chapter1_smoke import admin_call, call_int, require
    from plot_chapter2_recovery import connect, force_stop_and_reconnect, require_value
    from plot_chapter5_smoke import reset_case


def enter_from_safe_room(client, marker: str) -> None:
    admin_call(client, 'me->move("/d/city/wumiao")')
    require(client.command("plot act where_rivers_end_05 enter", 1.0), marker, "re-enter Chapter 5 instance")


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
        reset_case(client, 5_000_000, True)
        require(client.command("plot act where_rivers_end_05 begin"), "百川归处", "begin chapter")
        require(client.command("plot act where_rivers_end_05 entry_drain"), "排水道", "drain entry")
        require(client.command("plot act where_rivers_end_05 open_drain_gate"), "潮闸", "open drain")
        print("[entry-route] checkpoint prepared", flush=True)

        client = force_stop_and_reconnect(args, client)
        require_value(client, 'me->query("plot/where_rivers_end_05/flags/entry_method")', '"drain"', "entry method recovery")
        enter_from_safe_room(client, "排水道")
        require_value(client, 'me->query("plot/where_rivers_end_05/flags/drain_gate_open")', "= 1", "drain gate recovery")
        require(client.command("plot act where_rivers_end_05 reach_assembly"), "合流", "reach assembly")
        require(client.command("plot act where_rivers_end_05 hear_relief"), "救过整村人", "first testimony")
        print("[partial-testimony] checkpoint prepared", flush=True)

        client = force_stop_and_reconnect(args, client)
        require_value(client, 'me->query("plot/where_rivers_end_05/flags/testimonies_heard")', "= 1", "testimony count recovery")
        enter_from_safe_room(client, "主仓")
        client.command("plot act where_rivers_end_05 hear_debt")
        require(client.command("plot act where_rivers_end_05 hear_profiteering"), "真正陆小栓", "complete testimonies")
        client.command("plot act where_rivers_end_05 inspect_cell_lock")
        admin_call(client, "lu xiaoshuan->unconcious()", 0.7)
        require_value(client, 'me->query("plot/where_rivers_end_05/flags/real_lu_condition")', '"injured"', "Lu injury saved")
        print("[injured-real-lu] checkpoint prepared", flush=True)

        client = force_stop_and_reconnect(args, client)
        require_value(client, 'me->query("plot/where_rivers_end_05/stage")', '"rescue_real_lu"', "Lu rescue stage")
        enter_from_safe_room(client, "暗牢")
        require_value(client, 'lu xiaoshuan->query("plot_condition")', '"injured"', "Lu condition rebuild")
        if call_int(client, 'lu xiaoshuan->query("qi")') <= 0:
            raise RuntimeError("real Lu did not rebuild alive")
        require(client.command("plot act where_rivers_end_05 open_cell_tide"), "陆小栓证实石七", "rescue Lu")
        require(client.command("plot act where_rivers_end_05 fight_luo"), "不能跳过", "start Luo fight")
        luo_exp = call_int(client, 'luo qiniang->query("combat_exp")')
        luo_skill = call_int(client, 'luo qiniang->query("plot_combat/skill_level")')
        print("[alive-guard] checkpoint prepared", flush=True)

        client = force_stop_and_reconnect(args, client)
        enter_from_safe_room(client, "吊桥")
        require_value(client, 'luo qiniang->query("plot_combat/profile")', '"ledger_guard"', "Luo profile recovery")
        if call_int(client, 'luo qiniang->query("combat_exp")') != luo_exp or call_int(client, 'luo qiniang->query("plot_combat/skill_level")') != luo_skill:
            raise RuntimeError("Luo adaptive stats changed across recovery")
        client.command("plot act where_rivers_end_05 fight_luo")
        admin_call(client, "luo qiniang->die()", 0.8)
        require(client.command("plot act where_rivers_end_05 open_people_door"), "人门横木", "open people door")
        print("[fire-people-safe] checkpoint prepared", flush=True)

        client = force_stop_and_reconnect(args, client)
        require_value(client, 'me->query("plot/where_rivers_end_05/flags/people_door_open")', "= 1", "people door recovery")
        enter_from_safe_room(client, "起火")
        require(client.command("plot act where_rivers_end_05 save_core_index"), "核心索引", "save index")
        client.command("plot act where_rivers_end_05 inspect_order_grain")
        client.command("plot act where_rivers_end_05 inspect_order_debt")
        print("[partial-two-hands] checkpoint prepared", flush=True)

        client = force_stop_and_reconnect(args, client)
        require_value(client, 'me->query("plot/where_rivers_end_05/flags/orders_seen")', "= 2", "order inspection recovery")
        enter_from_safe_room(client, "五份命令")
        for order in ("silence", "letter", "name"):
            client.command(f"plot act where_rivers_end_05 inspect_order_{order}")
        require(client.command("plot act where_rivers_end_05 solve_hands_grain_debt"), "沈观澜之名", "solve hands")
        client.command("plot act where_rivers_end_05 cite_relief")
        client.command("plot act where_rivers_end_05 cite_debt")
        require(client.command("plot act where_rivers_end_05 fight_wen"), "当前战斗阶段", "start Boss phase zero")
        require_value(client, 'me->query("plot/where_rivers_end_05/flags/boss_max_phase")', "= 2", "top-player Boss phases")
        admin_call(client, "wen shouzhuo->die()", 0.8)
        require_value(client, 'me->query("plot/where_rivers_end_05/flags/boss_phase")', "= 1", "Boss phase checkpoint")
        print("[boss-next-phase] checkpoint prepared", flush=True)

        client = force_stop_and_reconnect(args, client)
        enter_from_safe_room(client, "封印室")
        require_value(client, 'wen shouzhuo->query("plot_combat/phase")', "= 1", "Boss phase-one rebuild")
        require(client.command("plot act where_rivers_end_05 fight_wen"), "当前战斗阶段", "restart Boss phase one")
        admin_call(client, "wen shouzhuo->die()", 0.8)
        require(client.command("plot act where_rivers_end_05 fight_wen"), "当前战斗阶段", "start Boss phase two")
        admin_call(client, "wen shouzhuo->die()", 0.8)
        require_value(client, 'me->query("plot/where_rivers_end_05/stage")', '"choose_archive_disposition"', "Boss completion recovery")
        require(client.command("plot act where_rivers_end_05 confirm_protected_archive"), "选择已经持久化", "confirm archive")
        print("[confirmed-ending] checkpoint prepared", flush=True)

        client = force_stop_and_reconnect(args, client)
        require_value(client, 'me->query("plot/where_rivers_end_05/stage")', '"arc_aftermath"', "ending stage recovery")
        require_value(client, 'me->query("plot/arc/hometown_letters_01/choices/archive_disposition")', '"protected_archive"', "arc choice recovery")
        score_before = call_int(client, 'me->query("score")')
        require(client.command("plot act where_rivers_end_05 aftermath", 1.0), "乡书篇完成", "finish recovered aftermath")
        if call_int(client, 'me->query("score")') != score_before + 25:
            raise RuntimeError("recovered aftermath score mismatch")
        print("[completed-idempotency] checkpoint prepared", flush=True)

        client = force_stop_and_reconnect(args, client)
        require_value(client, 'me->query("plot/where_rivers_end_05/status")', '"completed"', "completed status recovery")
        require_value(client, 'me->query("plot/arc/hometown_letters_01/status")', '"completed"', "completed arc recovery")
        score_after = call_int(client, 'me->query("score")')
        client.command("plot act where_rivers_end_05 aftermath", 0.8)
        if call_int(client, 'me->query("score")') != score_after:
            raise RuntimeError("chapter-five completion duplicated score")
        print("[completed-idempotency] passed", flush=True)
        print("Fifth chapter force-stop recovery passed.")
        return 0
    finally:
        client.close()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError) as error:
        print(f"Fifth chapter force-stop recovery failed: {error}", file=sys.stderr)
        raise SystemExit(1)
