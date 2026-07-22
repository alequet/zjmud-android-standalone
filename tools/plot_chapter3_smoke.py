#!/usr/bin/env python3
"""Run the complete third-chapter route and resolution matrix."""

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


BOARDING = ("token", "stealth", "force")
RECORD_METHODS = ("fight", "water", "turn")
RESCUES = ("crates", "rope", "guard")
SETTLEMENTS = ("seal", "aid_first", "guarantor")
COMBAT_SCENARIOS = (
    (1_000, False, "newcomer"),
    (100_000, False, "midgame"),
    (1_000_000, False, "advanced"),
    (1_000, True, "newcomer_meridian"),
    (100_000, True, "midgame_meridian"),
    (1_000_000, True, "advanced_meridian"),
)


def reset_case(client: MudClient, experience: int, meridian: bool) -> None:
    admin_call(client, "me->remove_all_enemy(1)")
    admin_call(client, "me->remove_all_killer()")
    for key in ("max_qi", "max_jing", "max_neili"):
        admin_call(client, f'me->set("{key}",200)')
    for key in ("qi", "eff_qi", "jing", "eff_jing", "neili"):
        admin_call(client, f'me->set("{key}",200)')
    admin_call(client, 'me->move("/d/city/beimen")', 0.8)
    admin_call(client, 'me->delete("plot/silent_crossing_03")')
    admin_call(client, 'me->delete_temp("plot/silent_crossing_03")')
    admin_call(client, 'me->delete("plot/visitors_from_home_04")')
    admin_call(client, 'me->delete_temp("plot/visitors_from_home_04")')
    admin_call(client, 'me->set("registered",1)')
    admin_call(client, 'me->set("born","扬州人氏")')
    admin_call(client, 'me->set("startroom","/d/city/wumiao")')
    admin_call(client, f'me->set("combat_exp",{experience})')
    for key in ("qi", "eff_qi", "jing", "eff_jing", "neili", "food", "water"):
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
    admin_call(client, 'me->set("plot/returning_mark_02/version",1)')
    admin_call(client, 'me->set("plot/returning_mark_02/status","completed")')
    admin_call(client, 'me->set("plot/returning_mark_02/stage","chapter_close")')
    admin_call(client, 'me->set("plot/returning_mark_02/flags/full_token_recovered",1)')
    admin_call(client, 'me->set("plot/arc/hometown_letters_01/version",1)')
    admin_call(client, 'me->set("plot/arc/hometown_letters_01/status","active")')
    admin_call(client, 'me->set("plot/arc/hometown_letters_01/flags/green_reed_ferry_known",1)')
    admin_call(client, 'me->set("plot/arc/hometown_letters_01/current_chapter",3)')
    admin_call(client, 'me->set("plot/silent_crossing_03/version",1)')
    admin_call(client, 'me->set("plot/silent_crossing_03/status","available")')
    admin_call(client, 'me->set("plot/silent_crossing_03/stage","watch_green_reed_ferry")')
    require(
        client.command_until("call me->save()", "->save() ="),
        "->save() =",
        "third chapter fixture save",
    )
    require(
        client.command_until(
            'call me->query("plot/silent_crossing_03/status")',
            '= "available"$br#',
        ),
        '= "available"$br#',
        "third chapter reset synchronization",
    )
    require(
        client.command_until("plot silent_crossing_03", "可接取"),
        "可接取",
        "third chapter availability",
    )


def identify_boat(client: MudClient, first_case: bool) -> None:
    require(
        client.command_until(
            "plot act silent_crossing_03 watch_green_reed_ferry", "夜渡无声"
        ),
        "夜渡无声",
        "watch Green Reed Ferry",
    )
    if first_case:
        require(
            client.command("plot act silent_crossing_03 signal_17"),
            "早于回浪凭据",
            "wrong early-tide signal",
        )
        require(
            client.command("plot act silent_crossing_03 signal_9"),
            "不是凭据所记",
            "wrong cargo signal",
        )
        require(
            client.command("plot act silent_crossing_03 signal_hint"),
            "二潮、二十三号船",
            "signal hint",
        )
    require(
        client.command("plot act silent_crossing_03 signal_23", 0.9),
        "锁定真正的夜船",
        "correct night-boat signal",
    )


def board(client: MudClient, method: str, first_case: bool) -> None:
    if method == "token" and first_case:
        admin_call(client, 'me->delete("plot/returning_mark_02/flags/full_token_recovered")')
        require(
            client.command("plot act silent_crossing_03 board_token"),
            "没有完整回浪凭据",
            "token boarding prerequisite",
        )
        admin_call(client, 'me->set("plot/returning_mark_02/flags/full_token_recovered",1)')
    marker = {
        "token": "放下了跳板",
        "stealth": "不检查轻功门槛",
        "force": "后续冲突压力更高",
    }[method]
    require(
        client.command(f"plot act silent_crossing_03 board_{method}", 1.0),
        marker,
        f"board by {method}",
    )
    if method == "token":
        require(
            admin_call(client, 'liu dashao->query("plot_owner")'),
            '"codexaiqa"',
            "night-boat owner isolation",
        )
    if method == "force":
        require(
            admin_call(client, 'me->query("plot/silent_crossing_03/flags/boarding_pressure")'),
            "= 1",
            "force boarding pressure",
        )
        require(
            client.command("plot act silent_crossing_03 reach_cargo_hold"),
            "缆绳封住窄梯",
            "force boarding deck blockade",
        )
        require(
            client.command("plot act silent_crossing_03 secure_deck"),
            "夺下系缆柱",
            "clear force-boarding deck blockade",
        )
    require(
        client.command("plot act silent_crossing_03 reach_cargo_hold", 1.0),
        "三条登船路线都汇入",
        "boarding route passage convergence",
    )
    require(
        client.command("plot act silent_crossing_03 reach_cargo_hold", 1.0),
        "阿禾被绳索和药箱压在舱角",
        "boarding convergence",
    )


def verify_leave_reentry(client: MudClient) -> None:
    require(
        client.command("plot act silent_crossing_03 leave", 1.0),
        "离开夜船",
        "leave night boat",
    )
    require(
        client.command("plot act silent_crossing_03 enter", 1.0),
        "钻入货舱",
        "re-enter cargo hold",
    )
    if call_int(client, 'a he->query("qi")') <= 0:
        raise RuntimeError("A He did not recover on cargo-hold rebuild")


def rescue(client: MudClient, method: str, injury_mode: str) -> None:
    if injury_mode:
        admin_call(client, f"a he->{injury_mode}()", 0.8)
        require(
            admin_call(client, 'me->query("plot/silent_crossing_03/flags/a_he_condition")'),
            '"injured"',
            "A He protected injury state",
        )
        if call_int(client, 'a he->query("qi")') <= 0:
            raise RuntimeError("A He died instead of entering protected injury state")
        return
    marker = {
        "crates": "依次卸掉上层粮袋",
        "rope": "固定主缆",
        "guard": "撞开还想护货不救人的押运者",
    }[method]
    require(
        client.command(f"plot act silent_crossing_03 rescue_{method}", 0.9),
        marker,
        f"rescue A He by {method}",
    )


def inspect_cargos(client: MudClient) -> dict[str, int]:
    require(
        client.command("plot act silent_crossing_03 inspect_grain"),
        "义仓旧号",
        "inspect relabelled grain",
    )
    require(
        client.command("plot act silent_crossing_03 inspect_medicine"),
        "改写生死状态",
        "inspect medicine tags",
    )
    require(
        client.command("plot act silent_crossing_03 inspect_relabelled", 0.9),
        "火折已经逼近舱灯",
        "inspect unregistered relief grain",
    )
    require(
        admin_call(client, 'luo qiniang->query("plot_combat/profile")'),
        '"ferry_matron"',
        "Luo Qiniang adaptive profile",
    )
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
    if " = 0" in admin_call(client, 'luo qiniang->query_temp("weapon")'):
        raise RuntimeError("Luo Qiniang did not receive her adaptive weapon")
    require(
        admin_call(client, 'luo qiniang->query("attitude")'),
        '"peaceful"',
        "Luo Qiniang peaceful entry",
    )
    max_neili = call_int(client, 'luo qiniang->query("max_neili")')
    neili = call_int(client, 'luo qiniang->query("neili")')
    if max_neili <= 0 or neili != max_neili:
        raise RuntimeError("Luo Qiniang did not enter at full neili")
    return {
        "enemy_exp": call_int(client, 'luo qiniang->query("combat_exp")'),
        "effective_level": call_int(
            client, 'luo qiniang->query("plot_combat/effective_level")'
        ),
        "skill": call_int(client, 'luo qiniang->query("plot_combat/skill_level")'),
        "qi": call_int(client, 'luo qiniang->query("max_qi")'),
        "boost": call_int(client, 'luo qiniang->query("plot_combat/meridian_boost")'),
    }


def verify_player_death_recovery(client: MudClient) -> None:
    require(
        client.command("plot act silent_crossing_03 fight_luo", 1.0),
        "罗七娘拔出短刀",
        "start Luo combat before player death",
    )
    if call_int(client, "luo qiniang->is_fighting()") != 1:
        raise RuntimeError("Luo Qiniang did not enter real combat before player death")
    admin_call(client, "me->die()", 1.2)
    require(
        admin_call(client, 'me->query("plot/silent_crossing_03/stage")'),
        '"stop_burning_records"',
        "third chapter stage survived player death",
    )
    admin_call(client, "me->reincarnate()")
    for key in ("qi", "eff_qi", "jing", "eff_jing"):
        admin_call(client, f'me->set("{key}",200)')
    admin_call(client, 'me->move("/d/city/beimen")')
    require(
        client.command("plot act silent_crossing_03 enter", 1.0),
        "钻入货舱",
        "cargo-hold recovery after player death",
    )
    require(
        admin_call(client, 'luo qiniang->query("attitude")'),
        '"peaceful"',
        "Luo Qiniang reset after player death",
    )


def resolve_records(client: MudClient, method: str, death_case: bool) -> None:
    if method == "fight":
        if death_case:
            verify_player_death_recovery(client)
        require(
            client.command("plot act silent_crossing_03 fight_luo", 1.0),
            "罗七娘拔出短刀",
            "fight Luo Qiniang",
        )
        if call_int(client, "luo qiniang->is_fighting()") != 1:
            raise RuntimeError("Luo Qiniang did not enter real combat")
        trigger_core_perform(client, "luo qiniang", "blade.duan")
        admin_call(client, "luo qiniang->die()", 0.8)
        require(
            admin_call(client, 'me->query("plot/silent_crossing_03/flags/luo_fate")'),
            '"defeated"',
            "Luo Qiniang continuity fate",
        )
    elif method == "water":
        require(
            client.command("plot act silent_crossing_03 douse_records"),
            "先从舱底汲水",
            "water prerequisite",
        )
        require(
            client.command("plot act silent_crossing_03 draw_water"),
            "汲起一桶河水",
            "draw cargo-hold water",
        )
        require(
            client.command("plot act silent_crossing_03 return_hold"),
            "水桶已经放到火折与存根之间",
            "return from bilge with water",
        )
        require(
            client.command("plot act silent_crossing_03 douse_records", 0.9),
            "核心存根和一份附签被保住",
            "douse burning records",
        )
    else:
        require(
            client.command("plot act silent_crossing_03 turn_shipworkers"),
            "先把换名药签摊开",
            "shipworker prerequisite",
        )
        require(
            client.command("plot act silent_crossing_03 show_relabelled_tag"),
            "阿禾当场认出家人的指印",
            "show relabelled medicine tag",
        )
        require(
            client.command("plot act silent_crossing_03 turn_shipworkers", 0.9),
            "交出全部三份存根",
            "turn shipworkers",
        )
    require(
        admin_call(client, 'me->query("plot/silent_crossing_03/stage")'),
        '"settle_the_boat"',
        f"record resolution stage {method}",
    )


def settle_and_complete(
    client: MudClient, method: str, reward_recovery: bool
) -> None:
    marker = {
        "seal": "封存改签义仓粮",
        "aid_first": "先卸救命粮",
        "guarantor": "关系最稳的一方",
    }[method]
    require(
        client.command(f"plot act silent_crossing_03 settle_{method}", 0.9),
        marker,
        f"settle night boat by {method}",
    )
    score_before = call_int(client, 'me->query("score")')
    require(
        client.command("plot act silent_crossing_03 chapter_close", 1.0),
        "第三章「夜渡无声」完成",
        "third chapter completion",
    )
    if call_int(client, 'me->query("score")') != score_before + 15:
        raise RuntimeError("third chapter score mismatch")
    require(
        admin_call(client, 'me->query("plot/arc/hometown_letters_01/flags/baichuan_known")'),
        "= 1",
        "Baichuan arc flag",
    )
    require(
        admin_call(client, 'me->query("plot/arc/hometown_letters_01/current_chapter")'),
        "= 4",
        "arc chapter advance",
    )
    require(
        admin_call(client, 'me->query("plot/arc/hometown_letters_01/flags/home_letters_surge_known")'),
        "= 1",
        "fourth chapter letter hook",
    )
    client.command("plot act silent_crossing_03 chapter_close", 0.8)
    if call_int(client, 'me->query("score")') != score_before + 15:
        raise RuntimeError("duplicate third chapter completion changed score")
    require(
        client.command("plot visitors_from_home_04", 0.8),
        "可接取",
        "fourth chapter becomes available",
    )
    if reward_recovery:
        admin_call(client, 'relabelled stub->move("/d/city/beimen")')
        require(
            client.command("plot act silent_crossing_03 claim_stub"),
            "补出一份改签存根",
            "third chapter reward recovery",
        )
        require(
            client.command("plot act silent_crossing_03 claim_stub"),
            "还在你身上",
            "third chapter duplicate reward recovery",
        )


def run_case(
    client: MudClient,
    boarding: str,
    records: str,
    index: int,
    experience: int,
    meridian: bool,
) -> dict[str, int]:
    reset_case(client, experience, meridian)
    identify_boat(client, index == 0)
    board(client, boarding, index == 0)
    if index == 0:
        verify_leave_reentry(client)
    injury_mode = "unconcious" if index == 0 else ("die" if index == 1 else "")
    rescue(client, RESCUES[index % len(RESCUES)], injury_mode=injury_mode)
    metrics = inspect_cargos(client)
    resolve_records(client, records, death_case=index == 0)
    settle_and_complete(
        client, SETTLEMENTS[index % len(SETTLEMENTS)], reward_recovery=index == 0
    )
    return metrics


def verify_adaptive_ladder(metrics: dict[tuple[int, bool], dict[str, int]]) -> None:
    for meridian in (False, True):
        rows = [metrics[(exp, meridian)] for exp in (1_000, 100_000, 1_000_000)]
        for key in ("enemy_exp", "effective_level", "skill", "qi"):
            values = [row[key] for row in rows]
            if values != sorted(values) or len(set(values)) != len(values):
                raise RuntimeError(f"third chapter adaptive ladder failed: {key}={values}")
    for experience in (1_000, 100_000, 1_000_000):
        plain = metrics[(experience, False)]
        boosted = metrics[(experience, True)]
        if plain["boost"] != 0 or boosted["boost"] != 2:
            raise RuntimeError(f"third chapter meridian boost mismatch at {experience}")
        if boosted["skill"] <= plain["skill"] or boosted["qi"] <= plain["qi"]:
            raise RuntimeError(f"third chapter meridian scaling failed at {experience}")
    for experience, meridian, scenario in COMBAT_SCENARIOS:
        row = metrics[(experience, meridian)]
        print(
            "adaptive "
            f"scenario={scenario} player_exp={experience} enemy_exp={row['enemy_exp']} "
            f"level={row['effective_level']} skill={row['skill']} qi={row['qi']}",
            flush=True,
        )


def clear_test_stubs(client: MudClient) -> None:
    admin_call(client, 'me->move("/d/city/postofficer")')
    for _ in range(30):
        output = client.command('call relabelled stub->move("/d/city/wumiao")', 0.2)
        if "找不到" in output or "没有这样" in output or "不是一个物件" in output:
            break


def main() -> int:
    client = MudClient("127.0.0.1", 3000)
    try:
        login(client, "codexaiqa", "codex-ai-qa", "春风明月")
        clear_test_stubs(client)
        selftest = client.command("plotadmin selftest", 1.0)
        require(selftest, "PLOT_SELFTEST ok=1", "plot platform selftest")
        require(selftest, "PLOT_COMBAT_SELFTEST ok=1", "plot combat selftest")
        index = 0
        metrics: dict[tuple[int, bool], dict[str, int]] = {}
        for boarding in BOARDING:
            for records in RECORD_METHODS:
                experience, meridian, scenario = COMBAT_SCENARIOS[index % len(COMBAT_SCENARIOS)]
                result = run_case(client, boarding, records, index, experience, meridian)
                metrics[(experience, meridian)] = result
                print(
                    f"passed boarding={boarding} records={records} "
                    f"rescue={RESCUES[index % 3]} settlement={SETTLEMENTS[index % 3]} "
                    f"combat={scenario}",
                    flush=True,
                )
                index += 1
        verify_adaptive_ladder(metrics)
        print("Third chapter runtime matrix passed.")
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
        print(f"Third chapter runtime matrix failed: {error}", file=sys.stderr)
        raise SystemExit(1)
