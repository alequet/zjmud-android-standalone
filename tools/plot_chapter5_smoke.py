#!/usr/bin/env python3
"""Run the complete fifth-chapter branch, ending, and Boss matrix."""

from __future__ import annotations

import sys

try:
    from .plot_chapter1_smoke import (
        MudClient, admin_call, call_int, login, require,
        trigger_core_perform, verify_martial_configuration,
    )
except ImportError:
    from plot_chapter1_smoke import (
        MudClient, admin_call, call_int, login, require,
        trigger_core_perform, verify_martial_configuration,
    )


ENTRIES = ("invitation", "boatmen", "joint", "drain")
CONTROL_METHODS = ("fight", "boatmen", "evidence")
COMBAT_SCENARIOS = (
    (1_000, False, "newcomer", 0),
    (125_000, False, "midgame", 1),
    (5_000_000, False, "top", 2),
    (1_000, True, "newcomer_meridian", 0),
    (125_000, True, "midgame_meridian", 1),
    (5_000_000, True, "top_meridian", 2),
)


def reset_case(client: MudClient, experience: int, meridian: bool) -> None:
    admin_call(client, "me->remove_all_enemy(1)")
    admin_call(client, "me->remove_all_killer()")
    admin_call(client, "me->reincarnate()")
    admin_call(client, 'me->move("/d/city/wumiao")', 0.8)
    admin_call(client, 'me->delete("plot/where_rivers_end_05")')
    admin_call(client, 'me->delete_temp("plot/where_rivers_end_05")')
    admin_call(client, 'me->set("registered",1)')
    admin_call(client, 'me->set("born","扬州人氏")')
    admin_call(client, 'me->set("startroom","/d/city/wumiao")')
    admin_call(client, f'me->set("combat_exp",{experience})')
    admin_call(client, 'me->set_temp("apply/attack",-100000)')
    admin_call(client, 'me->set_temp("apply/damage",-100000)')
    admin_call(client, 'me->set_temp("apply/unarmed_damage",-100000)')
    # Keep the QA player alive while the high-tier NPC's mappings and
    # prerequisites are read over several administrator calls.
    for key in ("max_qi", "max_jing", "max_neili", "qi", "eff_qi", "jing", "eff_jing", "neili"):
        admin_call(client, f'me->set("{key}",100000)')
    for key in ("food", "water"):
        admin_call(client, f'me->set("{key}",500)')
    for key in ("meridian/ap", "meridian/dp", "gain/attack", "gain/defense"):
        admin_call(client, f'me->delete("{key}")')
    if meridian:
        admin_call(client, 'me->set("meridian/ap","unarmed")')
        admin_call(client, 'me->set("meridian/dp","dodge")')
        admin_call(client, 'me->set("gain/attack",180)')
        admin_call(client, 'me->set("gain/defense",180)')
    for chapter, stage in (
        ("origin_letter_01", "aftermath"),
        ("returning_mark_02", "chapter_close"),
        ("silent_crossing_03", "chapter_close"),
        ("visitors_from_home_04", "chapter_close"),
    ):
        admin_call(client, f'me->set("plot/{chapter}/version",1)')
        admin_call(client, f'me->set("plot/{chapter}/status","completed")')
        admin_call(client, f'me->set("plot/{chapter}/stage","{stage}")')
    admin_call(client, 'me->set("plot/silent_crossing_03/flags/records_saved",2)')
    admin_call(client, 'me->set("plot/visitors_from_home_04/flags/invitation_quality",2)')
    admin_call(client, 'me->set("plot/visitors_from_home_04/flags/evidence_prepared",2)')
    admin_call(client, 'me->set("plot/arc/hometown_letters_01/version",1)')
    admin_call(client, 'me->set("plot/arc/hometown_letters_01/status","active")')
    admin_call(client, 'me->set("plot/arc/hometown_letters_01/current_chapter",5)')
    admin_call(client, 'me->delete("plot/arc/hometown_letters_01/choices")')
    admin_call(client, 'me->delete("plot/arc/hometown_letters_01/completed_at")')
    admin_call(client, 'me->set("plot/arc/hometown_letters_01/relations/boatmen",2)')
    admin_call(client, 'me->set("plot/arc/hometown_letters_01/relations/hometown_hall",0)')
    admin_call(client, 'me->set("plot/arc/hometown_letters_01/relations/yamen",0)')
    admin_call(client, 'me->set("plot/arc/hometown_letters_01/relations/grain_house",-1)')
    admin_call(client, 'me->set("plot/where_rivers_end_05/version",1)')
    admin_call(client, 'me->set("plot/where_rivers_end_05/status","available")')
    admin_call(client, 'me->set("plot/where_rivers_end_05/stage","enter_salt_storehouse")')
    require(client.command_until("plot where_rivers_end_05", "可接取"), "可接取", "chapter-five availability")


def enter_and_hear(client: MudClient, entry: str) -> None:
    require(client.command("plot act where_rivers_end_05 begin"), "百川归处", "chapter-five begin")
    marker = {
        "invitation": "先听见集会第一轮",
        "boatmen": "被扣船工",
        "joint": "外围护卫",
        "drain": "排水道",
    }[entry]
    require(client.command(f"plot act where_rivers_end_05 entry_{entry}", 0.9), marker, f"entry {entry}")
    if entry == "drain":
        require(client.command("plot act where_rivers_end_05 reach_assembly"), "锈死", "drain prerequisite")
        require(client.command("plot act where_rivers_end_05 open_drain_gate"), "潮闸", "open drain gate")
    require(client.command("plot act where_rivers_end_05 reach_assembly", 0.9), "合流", "entry convergence")
    require(client.command("plot act where_rivers_end_05 hear_relief"), "救过整村人", "relief testimony")
    require(client.command("plot act where_rivers_end_05 hear_debt"), "欠名契", "debt testimony")
    require(client.command("plot act where_rivers_end_05 hear_profiteering", 0.9), "真正陆小栓", "profiteering testimony")


def rescue_real_lu(client: MudClient, injury_case: bool) -> None:
    require(client.command("plot act where_rivers_end_05 inspect_cell_lock"), "七号仓", "inspect cell lock")
    require(client.command("plot act where_rivers_end_05 open_cell_warehouse"), "相冲突", "wrong cell solution")
    if injury_case:
        admin_call(client, "lu xiaoshuan->unconcious()", 0.7)
        require(admin_call(client, 'lu xiaoshuan->query("plot_condition")'), '"injured"', "real Lu injury")
        if call_int(client, 'lu xiaoshuan->query("qi")') <= 0:
            raise RuntimeError("real Lu did not survive protected injury")
    require(client.command("plot act where_rivers_end_05 open_cell_tide", 0.9), "陆小栓证实石七", "rescue real Lu")


def break_control(client: MudClient, method: str) -> dict[str, int] | None:
    if method == "fight":
        require(client.command("plot act where_rivers_end_05 fight_luo"), "不能跳过", "fight Luo")
        if call_int(client, "luo qiniang->is_fighting()") != 1:
            raise RuntimeError("Luo Qiniang did not enter real combat")
        verify_martial_configuration(
            client,
            "luo qiniang",
            background="baichuan_water_escort",
            style="blade",
            martial_skill="wuhu-duanmendao",
            force_skill="lingyuan-xinfa",
            dodge_skill="feiyan-zoubi",
            prepared_style="hand",
            prepared_skill="yunlong-shou",
            weapon="/clone/weapon/gangdao",
            core_perform="blade.duan",
            min_primary=120,
            min_force=100,
            min_neili=200,
        )
        metrics = {
            "enemy_exp": call_int(client, 'luo qiniang->query("combat_exp")'),
            "skill": call_int(client, 'luo qiniang->query("plot_combat/skill_level")'),
            "qi": call_int(client, 'luo qiniang->query("max_qi")'),
        }
        admin_call(client, "luo qiniang->die()", 0.8)
        return metrics
    if method == "boatmen":
        require(client.command("plot act where_rivers_end_05 turn_boatmen", 0.9), "柳大艄", "turn boatmen")
    else:
        require(client.command("plot act where_rivers_end_05 show_public_evidence", 0.9), "改签存根", "show public evidence")
    return None


def verify_wen_phase(client: MudClient, phase: int) -> None:
    requirements = (
        ("staff.chan", 70, 100, 200),
        ("staff.ban", 100, 100, 300),
        ("staff.wugou", 150, 220, 500),
    )
    core_perform, min_primary, min_force, min_neili = requirements[phase]
    verify_martial_configuration(
        client,
        "wen shouzhuo",
        background="gaibang_old_branch",
        style="staff",
        martial_skill="dagou-bang",
        force_skill="huntian-qigong",
        dodge_skill="feiyan-zoubi",
        prepared_style="strike",
        prepared_skill="dragon-strike",
        weapon="/clone/plot/where_rivers_end/qimei_staff",
        core_perform=core_perform,
        min_primary=min_primary,
        min_force=min_force,
        min_neili=min_neili,
    )
    if call_int(client, 'wen shouzhuo->query("plot_combat/phase")') != phase:
        raise RuntimeError(f"Wen Shouzhuo configured for wrong phase {phase}")


def survive_fire_and_solve(client: MudClient, loose_first: bool, wrong_first: bool) -> None:
    if loose_first:
        require(client.command("plot act where_rivers_end_05 save_loose_pages"), "所有人都活着", "NPC safety fallback")
    else:
        require(client.command("plot act where_rivers_end_05 open_people_door"), "人门横木", "open people door")
        require(client.command("plot act where_rivers_end_05 save_loose_pages"), "附加责任链", "save loose pages")
    require(client.command("plot act where_rivers_end_05 save_core_index", 0.9), "核心索引", "save core index")
    for order, marker in (
        ("grain", "旧式账房语"),
        ("debt", "追欠令"),
        ("silence", "缺去右下一点"),
        ("letter", "杭州纸行"),
        ("name", "旧壳"),
    ):
        require(client.command(f"plot act where_rivers_end_05 inspect_order_{order}"), marker, f"inspect {order}")
    if wrong_first:
        require(client.command("plot act where_rivers_end_05 solve_hands_grain_silence"), "不能归为同一只手", "wrong hand grouping")
        require(client.command("plot act where_rivers_end_05 hands_hint_one"), "杭州词法", "first hand hint")
        require(client.command("plot act where_rivers_end_05 hands_hint_two"), "亲笔样本", "second hand hint")
    require(client.command("plot act where_rivers_end_05 solve_hands_grain_debt", 0.9), "沈观澜之名", "solve two hands")


def resolve_wen(client: MudClient, resolution: str) -> dict[str, int] | None:
    require(client.command("plot act where_rivers_end_05 fight_wen"), "必须先用至少两类事实", "combat cannot skip evidence")
    require(client.command("plot act where_rivers_end_05 cite_relief"), "救过人是真的", "cite relief")
    require(client.command("plot act where_rivers_end_05 cite_debt"), "已还清", "cite debt")
    if resolution == "evidence":
        require(client.command("plot act where_rivers_end_05 cite_second_hand"), "双笔迹分组", "cite second hand")
        require(client.command("plot act where_rivers_end_05 force_wen_yield", 0.9), "交出总簿印钥", "evidence resolution")
        return None
    require(client.command("plot act where_rivers_end_05 fight_wen"), "当前战斗阶段", "start Wen Boss")
    metrics = {
        "enemy_exp": call_int(client, 'wen shouzhuo->query("combat_exp")'),
        "effective_level": call_int(client, 'wen shouzhuo->query("plot_combat/effective_level")'),
        "skill": call_int(client, 'wen shouzhuo->query("plot_combat/skill_level")'),
        "qi": call_int(client, 'wen shouzhuo->query("max_qi")'),
        "boost": call_int(client, 'wen shouzhuo->query("plot_combat/meridian_boost")'),
        "max_phase": call_int(client, 'me->query("plot/where_rivers_end_05/flags/boss_max_phase")'),
    }
    for phase in range(metrics["max_phase"] + 1):
        if call_int(client, "wen shouzhuo->is_fighting()") != 1:
            raise RuntimeError(f"Wen Shouzhuo phase {phase} did not enter real combat")
        verify_wen_phase(client, phase)
        trigger_core_perform(
            client,
            "wen shouzhuo",
            ("staff.chan", "staff.ban", "staff.wugou")[phase],
        )
        admin_call(client, "wen shouzhuo->die()", 0.8)
        if phase < metrics["max_phase"]:
            require(client.command("plot act where_rivers_end_05 fight_wen"), "当前战斗阶段", f"start Wen phase {phase + 1}")
    require(admin_call(client, 'me->query("plot/where_rivers_end_05/stage")'), '"choose_archive_disposition"', "Wen convergence")
    return metrics


def complete_arc(client: MudClient, choice: str, reward_recovery: bool) -> None:
    marker = "三方共同持钥" if choice == "full_archive" else "隐去受助者姓名"
    require(client.command(f"plot act where_rivers_end_05 choose_{choice}"), marker, "archive preview")
    require(client.command(f"plot act where_rivers_end_05 confirm_{choice}"), "选择已经持久化", "archive confirm")
    score_before = call_int(client, 'me->query("score")')
    require(client.command("plot act where_rivers_end_05 aftermath", 1.0), "乡书篇完成", "arc aftermath")
    if call_int(client, 'me->query("score")') != score_before + 25:
        raise RuntimeError("chapter-five score was not granted exactly once")
    require(admin_call(client, 'me->query("plot/where_rivers_end_05/status")'), '"completed"', "chapter-five status")
    require(admin_call(client, 'me->query("plot/arc/hometown_letters_01/status")'), '"completed"', "arc status")
    require(admin_call(client, 'me->query("plot/arc/hometown_letters_01/choices/archive_disposition")'), f'"{choice}"', "arc choice")
    score_after = score_before + 25
    client.command("plot act where_rivers_end_05 aftermath")
    if call_int(client, 'me->query("score")') != score_after:
        raise RuntimeError("duplicate chapter-five completion changed score")
    if reward_recovery:
        admin_call(client, 'hometown memorial->move("/d/city/beimen")')
        require(client.command("plot act where_rivers_end_05 claim_memorial"), "补出一页", "reward recovery")
        require(client.command("plot act where_rivers_end_05 claim_memorial"), "还在你身上", "duplicate reward")


def verify_adaptive_ladder(metrics: dict[tuple[int, bool], dict[str, int]]) -> None:
    for meridian in (False, True):
        rows = [metrics[(exp, meridian)] for exp in (1_000, 125_000, 5_000_000)]
        for key in ("enemy_exp", "effective_level", "skill", "qi"):
            values = [row[key] for row in rows]
            if values != sorted(values) or len(set(values)) != len(values):
                raise RuntimeError(f"chapter-five Boss adaptive ladder failed: {key}={values}")
        if [row["max_phase"] for row in rows] != [0, 1, 2]:
            raise RuntimeError(f"chapter-five Boss phase ladder failed: {rows}")
    for exp in (1_000, 125_000, 5_000_000):
        plain = metrics[(exp, False)]
        boosted = metrics[(exp, True)]
        if plain["boost"] != 0 or boosted["boost"] != 2:
            raise RuntimeError(f"chapter-five meridian boost mismatch at {exp}")
        if boosted["skill"] <= plain["skill"] or boosted["qi"] <= plain["qi"]:
            raise RuntimeError(f"chapter-five meridian scaling failed at {exp}")


def main() -> int:
    client = MudClient("127.0.0.1", 3000)
    try:
        login(client, "codexaiqa", "codex-ai-qa", "春风明月")
        boss_metrics: dict[tuple[int, bool], dict[str, int]] = {}
        battle_index = 0
        index = 0
        for entry in ENTRIES:
            for control in CONTROL_METHODS:
                resolution = "battle" if index % 2 == 0 else "evidence"
                if resolution == "battle":
                    experience, meridian, scenario, expected_phase = COMBAT_SCENARIOS[battle_index]
                    battle_index += 1
                else:
                    experience, meridian, scenario, expected_phase = (125_000, False, "evidence", 1)
                reset_case(client, experience, meridian)
                enter_and_hear(client, entry)
                rescue_real_lu(client, injury_case=index == 0)
                break_control(client, control)
                survive_fire_and_solve(client, loose_first=index % 2 == 1, wrong_first=index == 0)
                metrics = resolve_wen(client, resolution)
                if metrics is not None:
                    if metrics["max_phase"] != expected_phase:
                        raise RuntimeError(f"unexpected Boss phase for {scenario}: {metrics}")
                    boss_metrics[(experience, meridian)] = metrics
                choice = "full_archive" if index % 2 == 0 else "protected_archive"
                complete_arc(client, choice, reward_recovery=index == 0)
                print(f"passed entry={entry} control={control} resolution={resolution} choice={choice} combat={scenario}")
                index += 1
        verify_adaptive_ladder(boss_metrics)
        print("Fifth chapter runtime matrix passed.")
        return 0
    finally:
        try:
            admin_call(client, 'me->set("combat_exp",0)')
            for key in ("max_qi", "max_jing", "max_neili", "qi", "eff_qi", "jing", "eff_jing", "neili"):
                admin_call(client, f'me->set("{key}",500)')
            for key in ("apply/attack", "apply/damage", "apply/unarmed_damage"):
                admin_call(client, f'me->delete_temp("{key}")')
            for key in ("meridian/ap", "meridian/dp", "gain/attack", "gain/defense"):
                admin_call(client, f'me->delete("{key}")')
        except (OSError, RuntimeError):
            pass
        client.close()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError) as error:
        print(f"Fifth chapter runtime matrix failed: {error}", file=sys.stderr)
        raise SystemExit(1)
