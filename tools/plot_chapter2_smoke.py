#!/usr/bin/env python3
"""Run the complete second-chapter branch and resolution matrix."""

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


BRANCHES = ("hall", "yamen", "grain")
RESOLUTIONS = ("fight", "intercept", "persuade")
COMBAT_SCENARIOS = (
    (1_000, False, "newcomer"),
    (100_000, False, "midgame"),
    (1_000_000, False, "advanced"),
    (1_000, True, "newcomer_meridian"),
    (100_000, True, "midgame_meridian"),
    (1_000_000, True, "advanced_meridian"),
)


def reset_case(client: MudClient, branch: str, experience: int, meridian: bool) -> None:
    admin_call(client, 'me->move("/d/city/beimen")', 0.8)
    admin_call(client, 'me->delete("plot/returning_mark_02")')
    admin_call(client, 'me->delete_temp("plot/returning_mark_02")')
    admin_call(client, 'me->set("registered",1)')
    admin_call(client, 'me->set("born","扬州人氏")')
    admin_call(client, 'me->set("startroom","/d/city/wumiao")')
    admin_call(client, f'me->set("combat_exp",{experience})')
    for key in ("qi", "eff_qi", "jing", "eff_jing", "food", "water"):
        admin_call(client, f'me->set("{key}",200)')
    for key in ("meridian/ap", "meridian/dp", "gain/attack", "gain/defense"):
        admin_call(client, f'me->delete("{key}")')
    if meridian:
        admin_call(client, 'me->set("meridian/ap","unarmed")')
        admin_call(client, 'me->set("meridian/dp","dodge")')
        admin_call(client, 'me->set("gain/attack",180)')
        admin_call(client, 'me->set("gain/defense",180)')
    admin_call(client, 'me->set("plot/origin_letter_01/version",1)')
    admin_call(client, 'me->set("plot/origin_letter_01/status","completed")')
    admin_call(client, 'me->set("plot/origin_letter_01/stage","aftermath")')
    admin_call(client, f'me->set("plot/origin_letter_01/choices/ledger_custodian","{branch}")')
    for key in ("hometown_hall", "yamen", "grain_house"):
        value = 2 if ((branch == "hall" and key == "hometown_hall") or
                      (branch == "yamen" and key == "yamen") or
                      (branch == "grain" and key == "grain_house")) else 0
        admin_call(client, f'me->set("plot/origin_letter_01/relations/{key}",{value})')
    for key in ("flags", "choices", "relations"):
        admin_call(client, f'me->delete("plot/returning_mark_02/{key}")')
    admin_call(client, 'me->set("plot/returning_mark_02/version",1)')
    admin_call(client, 'me->set("plot/returning_mark_02/status","available")')
    admin_call(client, 'me->set("plot/returning_mark_02/stage","branch_hook")')
    admin_call(client, 'me->set("plot/arc/hometown_letters_01/version",1)')
    admin_call(client, 'me->set("plot/arc/hometown_letters_01/status","active")')
    admin_call(client, 'me->set("plot/arc/hometown_letters_01/current_chapter",2)')
    admin_call(client, 'me->set("plot/origin_letter_01/status","completed")')
    require(
        client.command_until("plot returning_mark_02", "可接取"),
        "可接取",
        f"chapter availability {branch}",
    )


def reach_old_storehouse(client: MudClient, branch: str, meridian: bool) -> dict[str, int]:
    require(
        client.command_until("plot act returning_mark_02 branch_hook", "旧印新痕"),
        "旧印新痕",
        f"branch hook {branch}",
    )
    other = "yamen" if branch == "hall" else "hall"
    require(
        client.command(f"plot act returning_mark_02 inspect_{other}"),
        "卷宗" if other == "yamen" else "会馆",
        f"second record {branch}",
    )
    require(
        admin_call(client, 'me->query("plot/arc/hometown_letters_01/relations/hometown_hall")'),
        "= 2" if branch == "hall" else "= 0",
        f"arc relation mapping {branch}",
    )
    admin_call(client, 'me->move("/d/city/beimen")')
    require(
        client.command("plot act returning_mark_02 find_old_storehouse", 0.9),
        "推开旧仓沉重的木门",
        f"old storehouse {branch}",
    )
    require(
        admin_call(client, 'qing peng->query("plot_combat/profile")'),
        '"green_hood"',
        f"unarmed adaptive profile {branch}",
    )
    verify_martial_configuration(
        client,
        "qing peng",
        background="shaolin_lay_hand",
        style="hand",
        martial_skill="fengyun-shou",
        force_skill="hunyuan-yiqi",
        dodge_skill="shaolin-shenfa",
        prepared_style="hand",
        prepared_skill="fengyun-shou",
        weapon="",
        core_perform="hand.qinna",
        min_primary=120,
        min_force=100,
        min_neili=150,
    )
    expected_boost = 2 if meridian else 0
    actual_boost = call_int(client, 'qing peng->query("plot_combat/meridian_boost")')
    if actual_boost != expected_boost:
        raise RuntimeError(f"meridian boost mismatch for {branch}")
    if call_int(client, 'qing peng->query_temp("weapon")') != 0:
        raise RuntimeError(f"Qing Peng Ke unexpectedly has a weapon for {branch}")
    require(
        admin_call(client, 'qing peng->query("attitude")'),
        '"peaceful"',
        "Qing Peng Ke peaceful entry",
    )
    max_neili = call_int(client, 'qing peng->query("max_neili")')
    neili = call_int(client, 'qing peng->query("neili")')
    if max_neili <= 0 or neili != max_neili:
        raise RuntimeError(f"Qing Peng Ke did not enter at full neili for {branch}")
    return {
        "enemy_exp": call_int(client, 'qing peng->query("combat_exp")'),
        "effective_level": call_int(client, 'qing peng->query("plot_combat/effective_level")'),
        "skill": call_int(client, 'qing peng->query("plot_combat/skill_level")'),
        "qi": call_int(client, 'qing peng->query("max_qi")'),
        "max_neili": max_neili,
        "neili": neili,
        "boost": actual_boost,
    }


def verify_escape_reentry(client: MudClient) -> None:
    require(
        client.command("plot act returning_mark_02 leave", 1.2),
        "你离开旧仓",
        "warehouse escape",
    )
    require(
        client.command("plot act returning_mark_02 enter", 0.9),
        "推开旧仓沉重的木门",
        "warehouse reentry",
    )
    require(
        admin_call(client, 'qing peng->query("attitude")'),
        '"peaceful"',
        "enemy restored after escape",
    )


def verify_player_death_recovery(client: MudClient) -> None:
    require(client.command("plot act returning_mark_02 fight_qing", 1.0), "青篷客翻过箱垛", "death combat start")
    if call_int(client, "qing peng->is_fighting()") != 1:
        raise RuntimeError("Qing Peng Ke did not enter real combat before player death")
    admin_call(client, "me->die()", 1.2)
    require(
        admin_call(client, 'me->query("plot/returning_mark_02/stage")'),
        '"protect_meng_si"',
        "plot stage survived player death",
    )
    admin_call(client, "me->reincarnate()")
    for key in ("qi", "eff_qi", "jing", "eff_jing"):
        admin_call(client, f'me->set("{key}",200)')
    admin_call(client, 'me->move("/d/city/beimen")')
    require(
        client.command("plot act returning_mark_02 enter", 0.9),
        "推开旧仓沉重的木门",
        "warehouse recovery after player death",
    )
    require(
        admin_call(client, 'qing peng->query("attitude")'),
        '"peaceful"',
        "enemy reset after player death",
    )


def resolve(client: MudClient, resolution: str) -> None:
    if resolution == "fight":
        require(client.command("plot act returning_mark_02 fight_qing"), "青篷客翻过箱垛", "fight start")
        if call_int(client, "qing peng->is_fighting()") != 1:
            raise RuntimeError("Qing Peng Ke did not enter real combat")
        trigger_core_perform(client, "qing peng", "hand.qinna")
        admin_call(client, "qing peng->die()", 0.8)
        require(client.command("plot act returning_mark_02 fight_qing", 0.9), "潮印案台", "fight recovery")
    elif resolution == "intercept":
        require(
            client.command("plot act returning_mark_02 intercept_qing"),
            "仓门仍然敞着",
            "intercept requires closed door",
        )
        require(client.command("plot act returning_mark_02 close_door"), "落下生锈的门闩", "close door")
        require(
            client.command("plot act returning_mark_02 intercept_qing"),
            "潮印案台",
            "intercept resolution",
        )
    else:
        require(
            client.command("plot act returning_mark_02 persuade_qing"),
            "口说无凭",
            "persuasion requires half token",
        )
        require(client.command("plot act returning_mark_02 show_half_token"), "半枚回浪印", "show half token")
        require(
            client.command("plot act returning_mark_02 persuade_qing"),
            "潮印案台",
            "persuasion resolution",
        )
    require(
        admin_call(client, 'me->query("plot/returning_mark_02/flags/full_token_recovered")'),
        "= 1",
        f"full token recovered {resolution}",
    )


def solve_route(client: MudClient, wrong_first: bool, reward_recovery: bool) -> None:
    if wrong_first:
        client.command("plot act returning_mark_02 route_tide_4")
        client.command("plot act returning_mark_02 route_ship_17")
        client.command("plot act returning_mark_02 route_warehouse_4")
        require(
            client.command("plot act returning_mark_02 confirm_route"),
            "双半印在这里断开",
            "wrong route feedback",
        )
    client.command("plot act returning_mark_02 route_tide_2")
    client.command("plot act returning_mark_02 route_ship_23")
    client.command("plot act returning_mark_02 route_warehouse_7")
    require(
        client.command("plot act returning_mark_02 confirm_route"),
        "青芦渡",
        "correct route",
    )
    score_before = call_int(client, 'me->query("score")')
    require(
        client.command("plot act returning_mark_02 chapter_close", 0.9),
        "第二章「旧印新痕」完成",
        "chapter completion",
    )
    require(
        admin_call(client, 'me->query("plot/returning_mark_02/status")'),
        '"completed"',
        "completed status",
    )
    require(
        admin_call(client, 'me->query("plot/arc/hometown_letters_01/current_chapter")'),
        "= 3",
        "arc chapter advance",
    )
    score_after = call_int(client, 'me->query("score")')
    if score_after != score_before + 12:
        raise RuntimeError(f"chapter score mismatch: before={score_before} after={score_after}")
    client.command("plot act returning_mark_02 chapter_close", 0.8)
    if call_int(client, 'me->query("score")') != score_after:
        raise RuntimeError("duplicate chapter completion changed score")
    require(
        admin_call(client, 'me->query("plot/returning_mark_02/flags/reward_claimed")'),
        "= 1",
        "reward claimed flag",
    )
    if reward_recovery:
        admin_call(client, 'wave mark rubbing->move("/d/city/beimen")')
        require(
            client.command("plot act returning_mark_02 claim_rubbing"),
            "补出一张回浪印拓样",
            "rubbing recovery",
        )
        require(
            client.command("plot act returning_mark_02 claim_rubbing"),
            "还在你身上",
            "duplicate rubbing recovery",
        )


def run_case(
    client: MudClient,
    branch: str,
    resolution: str,
    case_index: int,
    experience: int,
    meridian: bool,
) -> dict[str, int]:
    reset_case(client, branch, experience, meridian)
    metrics = reach_old_storehouse(client, branch, meridian)
    if case_index == 0:
        verify_escape_reentry(client)
        admin_call(client, "meng si->unconcious()", 0.6)
        require(
            admin_call(client, 'me->query("plot/returning_mark_02/flags/meng_injured")'),
            "= 1",
            "Meng Si injury persisted",
        )
        if call_int(client, 'meng si->query("qi")') <= 0:
            raise RuntimeError("Meng Si died instead of entering protected injury state")
        admin_call(client, "meng si->die()", 0.6)
        if call_int(client, 'meng si->query("qi")') <= 0:
            raise RuntimeError("Meng Si die path did not remain protected")
        verify_player_death_recovery(client)
    resolve(client, resolution)
    solve_route(client, wrong_first=case_index == 0, reward_recovery=case_index == 0)
    return metrics


def verify_adaptive_ladder(metrics: dict[tuple[int, bool], dict[str, int]]) -> None:
    for meridian in (False, True):
        rows = [metrics[(experience, meridian)] for experience in (1_000, 100_000, 1_000_000)]
        for key in ("enemy_exp", "effective_level", "skill", "qi"):
            values = [row[key] for row in rows]
            if values != sorted(values) or len(set(values)) != len(values):
                raise RuntimeError(f"adaptive ladder is not strictly increasing: {key}={values}")
    for experience in (1_000, 100_000, 1_000_000):
        plain = metrics[(experience, False)]
        boosted = metrics[(experience, True)]
        if boosted["boost"] != 2 or plain["boost"] != 0:
            raise RuntimeError(f"meridian boost mismatch at {experience}")
        if boosted["skill"] <= plain["skill"] or boosted["qi"] <= plain["qi"]:
            raise RuntimeError(f"meridian scaling did not strengthen enemy at {experience}")
    for experience, meridian, scenario in COMBAT_SCENARIOS:
        row = metrics[(experience, meridian)]
        print(
            "adaptive "
            f"scenario={scenario} player_exp={experience} enemy_exp={row['enemy_exp']} "
            f"level={row['effective_level']} skill={row['skill']} qi={row['qi']}",
            flush=True,
        )


def clear_test_rubbings(client: MudClient) -> None:
    admin_call(client, 'me->move("/d/city/postofficer")')
    for _ in range(30):
        output = client.command('call wave mark rubbing->move("/d/city/wumiao")', 0.2)
        if "找不到" in output or "没有这样" in output or "不是一个物件" in output:
            break


def main() -> int:
    client = MudClient("127.0.0.1", 3000)
    try:
        login(client, "codexaiqa", "codex-ai-qa", "春风明月")
        clear_test_rubbings(client)
        selftest = client.command("plotadmin selftest")
        require(selftest, "PLOT_SELFTEST ok=1", "plot platform selftest")
        require(selftest, "PLOT_COMBAT_SELFTEST ok=1", "plot combat selftest")
        index = 0
        metrics: dict[tuple[int, bool], dict[str, int]] = {}
        for branch in BRANCHES:
            for resolution in RESOLUTIONS:
                experience, meridian, scenario = COMBAT_SCENARIOS[index % len(COMBAT_SCENARIOS)]
                result = run_case(client, branch, resolution, index, experience, meridian)
                metrics[(experience, meridian)] = result
                print(f"passed branch={branch} resolution={resolution} combat={scenario}")
                index += 1
        verify_adaptive_ladder(metrics)
        print("Second chapter runtime matrix passed.")
        return 0
    finally:
        try:
            admin_call(client, 'me->set("born","扬州人氏")')
            admin_call(client, 'me->set("startroom","/d/city/wumiao")')
            admin_call(client, 'me->set("combat_exp",0)')
            for key in ("meridian/ap", "meridian/dp", "gain/attack", "gain/defense"):
                admin_call(client, f'me->delete("{key}")')
        except (OSError, RuntimeError):
            pass
        client.close()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError) as error:
        print(f"Second chapter runtime matrix failed: {error}", file=sys.stderr)
        raise SystemExit(1)
