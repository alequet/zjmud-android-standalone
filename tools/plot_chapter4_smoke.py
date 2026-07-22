#!/usr/bin/env python3
"""Run the complete fourth-chapter branch and adaptive-combat matrix."""

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


ACTIONS = ("protect", "chase", "decoy")
CATCH_METHODS = ("fight", "block", "lure")
ORIGIN_GROUPS = ("north", "central", "south", "southeast", "southwest", "west", "family", "compat")
COMBAT_SCENARIOS = (
    (1_000, False, "newcomer"),
    (100_000, False, "midgame"),
    (1_000_000, False, "advanced"),
    (1_000, True, "newcomer_meridian"),
    (100_000, True, "midgame_meridian"),
    (1_000_000, True, "advanced_meridian"),
)


def reset_case(client: MudClient, group: str, experience: int, meridian: bool) -> None:
    admin_call(client, "me->remove_all_enemy(1)")
    admin_call(client, "me->remove_all_killer()")
    admin_call(client, "me->reincarnate()")
    admin_call(client, 'me->move("/d/city/wumiao")', 0.8)
    admin_call(client, 'me->delete("plot/visitors_from_home_04")')
    admin_call(client, 'me->delete_temp("plot/visitors_from_home_04")')
    admin_call(client, 'me->delete("plot/where_rivers_end_05")')
    admin_call(client, 'me->delete_temp("plot/where_rivers_end_05")')
    admin_call(client, 'me->set("registered",1)')
    admin_call(client, 'me->set("born","扬州人氏")')
    admin_call(client, 'me->set("startroom","/d/city/wumiao")')
    admin_call(client, f'me->set("combat_exp",{experience})')
    admin_call(client, 'me->set_temp("apply/attack",-100000)')
    admin_call(client, 'me->set_temp("apply/damage",-100000)')
    admin_call(client, 'me->set_temp("apply/unarmed_damage",-100000)')
    for key in ("max_qi", "max_jing", "max_neili", "qi", "eff_qi", "jing", "eff_jing", "neili"):
        admin_call(client, f'me->set("{key}",100000)')
    for key in ("food", "water"):
        admin_call(client, f'me->set("{key}",200)')
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
    ):
        admin_call(client, f'me->set("plot/{chapter}/version",1)')
        admin_call(client, f'me->set("plot/{chapter}/status","completed")')
        admin_call(client, f'me->set("plot/{chapter}/stage","{stage}")')
    admin_call(client, f'me->set("plot/origin_letter_01/flags/origin_group","{group}")')
    admin_call(client, 'me->set("plot/silent_crossing_03/flags/home_letters_surge_known",1)')
    admin_call(client, 'me->set("plot/arc/hometown_letters_01/version",1)')
    admin_call(client, 'me->set("plot/arc/hometown_letters_01/status","active")')
    admin_call(client, 'me->set("plot/arc/hometown_letters_01/current_chapter",4)')
    admin_call(client, 'me->set("plot/visitors_from_home_04/version",1)')
    admin_call(client, 'me->set("plot/visitors_from_home_04/status","available")')
    admin_call(client, 'me->set("plot/visitors_from_home_04/stage","letters_arrive")')
    require(client.command_until("plot visitors_from_home_04", "可接取"), "可接取", "chapter-four availability")


def reach_visitor(client: MudClient, injury_case: bool) -> None:
    require(client.command("plot act visitors_from_home_04 begin"), "故园来客", "chapter-four begin")
    require(client.command("plot act visitors_from_home_04 inspect_relief"), "杭州纸", "relief letter")
    require(client.command("plot act visitors_from_home_04 inspect_boatmen"), "二十三号夜船", "boatmen letter")
    require(client.command("plot act visitors_from_home_04 inspect_visitor"), "七个人名", "visitor letter")
    require(client.command("plot act visitors_from_home_04 meet_visitor", 0.9), "驿站后院", "enter relay court")
    require(admin_call(client, 'lu xiaoshuan->query("real_name")'), '"石七"', "Stone Seven identity object")
    if injury_case:
        admin_call(client, "lu xiaoshuan->unconcious()", 0.7)
        require(admin_call(client, 'lu xiaoshuan->query("plot_condition")'), '"injured"', "Stone Seven injury")
        if call_int(client, 'lu xiaoshuan->query("qi")') <= 0:
            raise RuntimeError("Stone Seven did not survive protected injury")
    require(client.command("plot act visitors_from_home_04 question_visitor"), "鞋底的细白沙", "question visitor")


def verify_letters(client: MudClient, wrong_first: bool) -> None:
    if wrong_first:
        require(client.command("plot act visitors_from_home_04 verify_format"), "不能单独证明", "format-only rejection")
        require(client.command("plot act visitors_from_home_04 verify_accent"), "不能解释", "accent-only rejection")
    require(client.command("plot act visitors_from_home_04 verify_timeline"), "寄出、入库和留宿时间", "timeline proof")
    require(client.command("plot act visitors_from_home_04 verify_travel"), "无面局换名人石七", "travel proof")


def choose_action(client: MudClient, action: str) -> dict[str, int]:
    marker = {"protect": "许三娘封住后院", "chase": "立刻追出北门", "decoy": "假回信"}[action]
    require(client.command(f"plot act visitors_from_home_04 choose_{action}", 0.9), marker, f"choose {action}")
    require(admin_call(client, 'hua yaozi->query("plot_combat/profile")'), '"courier_scout"', "Hua adaptive profile")
    verify_martial_configuration(
        client,
        "hua yaozi",
        background="yunlong_courier",
        style="whip",
        martial_skill="yunlong-bian",
        force_skill="yunlong-shengong",
        dodge_skill="yunlong-shenfa",
        prepared_style="hand",
        prepared_skill="yunlong-shou",
        weapon="/clone/weapon/changbian",
        core_perform="whip.chan",
        min_primary=70,
        min_force=100,
        min_neili=150,
    )
    return {
        "enemy_exp": call_int(client, 'hua yaozi->query("combat_exp")'),
        "effective_level": call_int(client, 'hua yaozi->query("plot_combat/effective_level")'),
        "skill": call_int(client, 'hua yaozi->query("plot_combat/skill_level")'),
        "qi": call_int(client, 'hua yaozi->query("max_qi")'),
        "boost": call_int(client, 'hua yaozi->query("plot_combat/meridian_boost")'),
    }


def catch_hua(client: MudClient, method: str) -> None:
    if method == "fight":
        require(client.command("plot act visitors_from_home_04 fight_hua"), "击败他后", "fight Hua")
        if call_int(client, "hua yaozi->is_fighting()") != 1:
            raise RuntimeError("Hua Yaozi did not enter real combat")
        trigger_core_perform(client, "hua yaozi", "whip.chan")
        admin_call(client, "hua yaozi->die()", 0.8)
    elif method == "block":
        if call_int(client, 'me->query("plot/visitors_from_home_04/flags/road_observed")') == 0:
            require(client.command("plot act visitors_from_home_04 block_hua"), "先查看", "block prerequisite")
            require(client.command("plot act visitors_from_home_04 observe_road"), "浅沟能截马", "observe road")
        require(client.command("plot act visitors_from_home_04 block_hua"), "交出完整请帖", "block Hua")
    else:
        if call_int(client, 'me->query("plot/visitors_from_home_04/flags/decoy_ready")') == 0:
            require(client.command("plot act visitors_from_home_04 lure_hua"), "先让杜宽准备", "lure prerequisite")
            require(client.command("plot act visitors_from_home_04 prepare_decoy"), "带错批号", "prepare decoy")
        require(client.command("plot act visitors_from_home_04 lure_hua"), "脱口纠正", "lure Hua")
    require(admin_call(client, 'me->query("plot/visitors_from_home_04/stage")'), '"decode_invitation"', "catch convergence")


def decode_and_complete(client: MudClient, wrong_first: bool, reward_recovery: bool) -> None:
    if wrong_first:
        require(client.command("plot act visitors_from_home_04 decode_place_grain"), "不是粮行新仓", "wrong place")
        require(client.command("plot act visitors_from_home_04 decode_tide_first"), "不是早潮", "wrong tide")
    client.command("plot act visitors_from_home_04 decode_place_salt")
    require(client.command("plot act visitors_from_home_04 decode_tide_second"), "七号旧盐仓", "decode invitation")
    require(client.command("plot act visitors_from_home_04 prepare_letters"), "1/2", "prepare letters")
    require(client.command("plot act visitors_from_home_04 prepare_invitation"), "2/2", "prepare invitation")
    require(client.command("plot act visitors_from_home_04 confirm_preparation"), "不形成永久路线", "confirm evidence")
    score_before = call_int(client, 'me->query("score")')
    require(client.command("plot act visitors_from_home_04 chapter_close", 1.0), "第四章「故园来客」完成", "chapter close")
    if call_int(client, 'me->query("score")') != score_before + 18:
        raise RuntimeError("chapter-four score was not granted exactly once")
    require(admin_call(client, 'me->query("plot/visitors_from_home_04/status")'), '"completed"', "chapter-four status")
    require(admin_call(client, 'me->query("plot/arc/hometown_letters_01/current_chapter")'), "= 5", "arc chapter advance")
    require(admin_call(client, 'me->query("plot/where_rivers_end_05/status")'), '"available"', "chapter five becomes available")
    score_after = score_before + 18
    client.command("plot act visitors_from_home_04 chapter_close")
    if call_int(client, 'me->query("score")') != score_after:
        raise RuntimeError("duplicate chapter-four completion changed score")
    if reward_recovery:
        admin_call(client, 'salt invitation->move("/d/city/beimen")')
        require(client.command("plot act visitors_from_home_04 claim_invitation"), "补出一张", "reward recovery")
        require(client.command("plot act visitors_from_home_04 claim_invitation"), "还在你身上", "duplicate reward")


def verify_adaptive_ladder(metrics: dict[tuple[int, bool], dict[str, int]]) -> None:
    for meridian in (False, True):
        rows = [metrics[(exp, meridian)] for exp in (1_000, 100_000, 1_000_000)]
        for key in ("enemy_exp", "effective_level", "skill", "qi"):
            values = [row[key] for row in rows]
            if values != sorted(values) or len(set(values)) != len(values):
                raise RuntimeError(f"chapter-four adaptive ladder failed: {key}={values}")
    for exp in (1_000, 100_000, 1_000_000):
        plain = metrics[(exp, False)]
        boosted = metrics[(exp, True)]
        if plain["boost"] != 0 or boosted["boost"] != 2:
            raise RuntimeError(f"chapter-four meridian boost mismatch at {exp}")
        if boosted["skill"] <= plain["skill"] or boosted["qi"] <= plain["qi"]:
            raise RuntimeError(f"chapter-four meridian scaling failed at {exp}")


def main() -> int:
    client = MudClient("127.0.0.1", 3000)
    try:
        login(client, "codexaiqa", "codex-ai-qa", "春风明月")
        metrics: dict[tuple[int, bool], dict[str, int]] = {}
        index = 0
        for action in ACTIONS:
            for method in CATCH_METHODS:
                experience, meridian, scenario = COMBAT_SCENARIOS[index % len(COMBAT_SCENARIOS)]
                group = ORIGIN_GROUPS[index % len(ORIGIN_GROUPS)]
                reset_case(client, group, experience, meridian)
                reach_visitor(client, injury_case=index == 0)
                verify_letters(client, wrong_first=index == 0)
                metrics[(experience, meridian)] = choose_action(client, action)
                catch_hua(client, method)
                decode_and_complete(client, wrong_first=index == 0, reward_recovery=index == 0)
                print(f"passed action={action} catch={method} origin={group} combat={scenario}")
                index += 1
        verify_adaptive_ladder(metrics)
        print("Fourth chapter runtime matrix passed.")
        return 0
    finally:
        try:
            admin_call(client, 'me->set("combat_exp",0)')
            for key in ("max_qi", "max_jing", "max_neili", "qi", "eff_qi", "jing", "eff_jing", "neili"):
                admin_call(client, f'me->set("{key}",200)')
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
        print(f"Fourth chapter runtime matrix failed: {error}", file=sys.stderr)
        raise SystemExit(1)
