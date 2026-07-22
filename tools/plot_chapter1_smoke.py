#!/usr/bin/env python3
"""Run the complete first-chapter matrix over the loopback MUD port."""

from __future__ import annotations

import argparse
import re
import select
import socket
import sys
import time


ENCODING = "gb18030"
HANDSHAKE = "zjmDMaIpOvxdb"
CREATE_CHARACTER = "\x1b0000008"
LOGIN_SUCCEEDED = "\x1b0000007"
WORLD_READY = "\x1b002"
METHODS = ("persuade", "ransom", "battle")
CHOICES = ("hall", "yamen", "grain")
COMBAT_SCENARIOS = (
    (1_000, False, "newcomer"),
    (100_000, False, "midgame"),
    (1_000_000, True, "advanced_meridian"),
)
CALL_INT_RE = re.compile(r" = (-?\d+)\$br#")


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
            readable, _, _ = select.select(
                [self.sock], [], [], min(0.2, deadline - time.monotonic())
            )
            if not readable:
                continue
            data = self.sock.recv(65536)
            if not data:
                break
            chunks.append(data)
        return b"".join(chunks).decode(ENCODING, errors="replace")

    def command(self, command: str, wait: float = 0.65) -> str:
        self.send(command)
        return self.read_for(wait)

    def command_until(self, command: str, marker: str, timeout: float = 5.0) -> str:
        self.send(command)
        deadline = time.monotonic() + timeout
        output = ""
        while time.monotonic() < deadline:
            output += self.read_for(min(0.25, deadline - time.monotonic()))
            if marker in output:
                return output + self.read_for(0.15)
        return output


def require(output: str, marker: str, context: str) -> None:
    if marker not in output:
        raise RuntimeError(f"{context}: expected {marker!r}\n{output[-3000:]}")


def login(client: MudClient, account: str, password: str, name: str) -> None:
    client.read_for(1.0)
    client.send(HANDSHAKE)
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


def admin_call(client: MudClient, expression: str, wait: float = 0.5) -> str:
    output = client.command(f"call {expression}", wait)
    if "呼叫中发生了错误" in output or "你没有直接呼叫" in output:
        raise RuntimeError(f"administrator call failed: {expression}\n{output[-3000:]}")
    return output


def call_int(client: MudClient, expression: str) -> int:
    output = admin_call(client, expression)
    match = CALL_INT_RE.search(output)
    if not match:
        raise RuntimeError(f"unable to parse integer call: {expression}\n{output[-3000:]}")
    return int(match.group(1))


def verify_martial_configuration(
    client: MudClient,
    npc: str,
    *,
    background: str,
    style: str,
    martial_skill: str,
    force_skill: str,
    dodge_skill: str,
    prepared_style: str,
    prepared_skill: str,
    weapon: str,
    core_perform: str,
    min_primary: int,
    min_force: int,
    min_neili: int,
) -> None:
    admin_call(client, "me->remove_all_enemy(1)")
    admin_call(client, f"{npc}->remove_all_enemy(1)")
    expected_strings = (
        (f'{npc}->query("plot_combat/background")', background, "background"),
        (f'{npc}->query_skill_mapped("{style}")', martial_skill, "primary mapping"),
        (f'{npc}->query_skill_mapped("force")', force_skill, "force mapping"),
        (f'{npc}->query_skill_mapped("dodge")', dodge_skill, "dodge mapping"),
        (f'{npc}->query_skill_prepared("{prepared_style}")', prepared_skill, "prepared skill"),
        (f'{npc}->query("plot_combat/core_perform")', core_perform, "core perform"),
    )
    for expression, expected, label in expected_strings:
        require(admin_call(client, expression), f'"{expected}"', f"{npc} {label}")
    if call_int(client, f'{npc}->query_skill("{style}",1)') < min_primary:
        raise RuntimeError(f"{npc} primary skill is below {min_primary}")
    if call_int(client, f'{npc}->query_skill("force",1)') < min_force:
        raise RuntimeError(f"{npc} force is below {min_force}")
    if call_int(client, f'{npc}->query("neili")') < min_neili:
        raise RuntimeError(f"{npc} neili is below {min_neili}")
    if call_int(client, f'{npc}->query("chat_chance_combat")') <= 0:
        raise RuntimeError(f"{npc} has no automatic combat action chance")
    equipped = admin_call(client, f'{npc}->query_temp("weapon")')
    if weapon:
        if " = 0" in equipped:
            raise RuntimeError(f"{npc} did not equip {weapon}")
        require(equipped, weapon, f"{npc} weapon")
    elif " = 0" not in equipped:
        raise RuntimeError(f"{npc} unexpectedly equipped a weapon")


def trigger_core_perform(client: MudClient, npc: str, core_perform: str) -> None:
    for key in ("max_qi", "eff_qi", "qi", "max_jing", "eff_jing", "jing"):
        admin_call(client, f'me->set("{key}",100000)')
    result = call_int(client, f"{npc}->plot_core_perform()")
    if result != 1:
        raise RuntimeError(f"{npc} failed to execute core perform {core_perform}")


def reset_chapter(
    client: MudClient, compatibility_origin: bool, experience: int, meridian: bool
) -> None:
    admin_call(client, 'me->delete("plot/origin_letter_01")')
    admin_call(client, 'me->delete_temp("plot/origin_letter_01")')
    admin_call(client, 'me->set("registered",1)')
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
    if compatibility_origin:
        admin_call(client, 'me->delete("born")')
    else:
        admin_call(client, 'me->set("born","扬州人氏")')


def acquire_ledger(client: MudClient, method: str) -> None:
    if method == "persuade":
        require(
            client.command("plot act origin_letter_01 persuade"),
            "你取回了义仓簿",
            "persuasion route",
        )
        require(
            admin_call(client, 'me->query("plot/origin_letter_01/flags/pei_jiu_fate")'),
            '"released"',
            "persuasion fate",
        )
        return
    if method == "ransom":
        admin_call(client, 'pei jiu->add_money("silver",100)')
        require(client.command("get silver from pei jiu"), "搜出", "seed ransom money")
        require(
            client.command("plot act origin_letter_01 ransom"),
            "你取回了义仓簿",
            "ransom route",
        )
        require(
            admin_call(client, 'me->query("plot/origin_letter_01/flags/pei_jiu_fate")'),
            '"released"',
            "ransom fate",
        )
        return
    require(client.command("plot act origin_letter_01 fight"), "裴九横刀", "battle start")
    trigger_core_perform(client, "pei jiu", "blade.duan")
    admin_call(client, "pei jiu->die()", 0.8)
    require(
        admin_call(client, 'me->query("plot/origin_letter_01/flags/pei_battle_won")'),
        "= 1",
        "battle victory persisted before claim",
    )
    require(
        client.command("plot act origin_letter_01 leave", 1.2),
        "你离开废仓",
        "leave warehouse after battle before claim",
    )
    require(
        client.command("plot act origin_letter_01 enter", 0.9),
        "推开一扇半朽的仓门",
        "rebuild warehouse after battle before claim",
    )
    require(
        client.command("plot act origin_letter_01 claim_battle"),
        "你取回了义仓簿",
        "battle route after instance rebuild",
    )
    require(
        admin_call(client, 'me->query("plot/origin_letter_01/flags/pei_jiu_fate")'),
        '"killed"',
        "battle death fate",
    )


def run_case(
    client: MudClient,
    method: str,
    choice: str,
    first_case: bool,
    experience: int,
    meridian: bool,
) -> None:
    reset_chapter(client, compatibility_origin=first_case, experience=experience, meridian=meridian)
    entry = admin_call(client, 'me->move("/d/city/wumiao")', 0.8)
    if first_case:
        require(entry, "杜宽托人送来一封同乡旧信", "first-arrival notice")
    admin_call(client, 'me->move("/d/city/postofficer")')
    accepted = client.command("plot act origin_letter_01 accept")
    require(accepted, "杜宽从箱底翻出一封旧信", "chapter acceptance")
    if first_case:
        require(accepted, "籍贯无考", "historical character compatibility")
    require(
        client.command("plot act origin_letter_01 begin_inquiry"),
        "三方说法",
        "investigation start",
    )
    client.command("plot act origin_letter_01 go_hall")
    require(
        client.command("plot act origin_letter_01 hear_hall"),
        "二十三户乡民",
        "hall clue",
    )
    client.command("plot act origin_letter_01 go_yamen")
    require(client.command("ask tan about 乡书"), "二十三枚签收手印", "Tan Youji clue")
    admin_call(client, 'me->move("/d/city/beimen")')
    require(
        client.command("plot act origin_letter_01 trace", 0.9),
        "推开一扇半朽的仓门",
        "warehouse instance",
    )
    require(
        admin_call(client, 'pei jiu->query("plot_combat/profile")'),
        '"street_blade"',
        "adaptive enemy profile",
    )
    verify_martial_configuration(
        client,
        "pei jiu",
        background="wanderer_escort_blade",
        style="blade",
        martial_skill="wuhu-duanmendao",
        force_skill="wuzheng-xinfa",
        dodge_skill="feiyan-zoubi",
        prepared_style="unarmed",
        prepared_skill="houquan",
        weapon="/clone/weapon/gangdao",
        core_perform="blade.duan",
        min_primary=120,
        min_force=80,
        min_neili=200,
    )
    expected_boost = 2 if meridian else 0
    actual_boost = call_int(client, 'pei jiu->query("plot_combat/meridian_boost")')
    if actual_boost != expected_boost:
        raise RuntimeError(
            f"meridian boost mismatch: expected={expected_boost} actual={actual_boost}"
        )
    acquire_ledger(client, method)
    if first_case:
        require(
            client.command("plot act origin_letter_01 hint_number"),
            "提示一",
            "numeric puzzle hint",
        )
        require(
            client.command("plot act origin_letter_01 hint_column"),
            "提示二",
            "binding puzzle hint",
        )
        require(
            client.command("plot act origin_letter_01 puzzle_grain"),
            "无法解释",
            "specific wrong-answer feedback",
        )
    require(
        client.command("plot act origin_letter_01 puzzle_names"),
        "花鹞子的人要抢走半张名单",
        "counteraction entry",
    )
    counter = "guard_names" if choice != "grain" else "quench_fire"
    require(
        client.command(f"plot act origin_letter_01 {counter}"),
        "证据没有散失",
        "counteraction convergence",
    )
    require(
        client.command(f"plot act origin_letter_01 choose_{choice}"),
        "确认后不可改写",
        "custodian confirmation prompt",
    )
    require(
        client.command(f"plot act origin_letter_01 confirm_{choice}"),
        "决定已经写入剧情日志",
        "custodian confirmation",
    )
    duplicate = client.command(f"plot act origin_letter_01 confirm_{choice}")
    require(duplicate, "江湖回响", "duplicate confirmation remains idempotent")
    require(
        client.command("plot act origin_letter_01 aftermath", 0.9),
        "第一章「一纸乡书」完成",
        "chapter completion",
    )
    log = client.command("plot origin_letter_01")
    for marker in ("取得账簿：", "账簿去向："):
        require(log, marker, "Chinese chapter review")
    if "persuade" in log or "battle" in log or "ransom" in log:
        raise RuntimeError(f"internal acquisition id leaked into log\n{log[-3000:]}")
    client.command("call old post token->move(\"/d/city/beimen\")", 0.5)
    require(
        client.command("plot act origin_letter_01 claim_token"),
        "补出一块旧驿木牌",
        "keepsake recovery",
    )
    require(
        client.command("plot act origin_letter_01 aftermath"),
        "取得账簿：",
        "duplicate completion",
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=3000)
    parser.add_argument("--account", default="codexaiqa")
    parser.add_argument("--password", default="codex-ai-qa")
    parser.add_argument("--name", default="春风明月")
    args = parser.parse_args()

    client = MudClient(args.host, args.port)
    try:
        login(client, args.account, args.password, args.name)
        selftest = client.command("plotadmin selftest")
        require(selftest, "PLOT_SELFTEST ok=1", "plot platform selftest")
        require(selftest, "PLOT_COMBAT_SELFTEST ok=1", "plot combat selftest")
        first_case = True
        case_index = 0
        for method in METHODS:
            for choice in CHOICES:
                experience, meridian, scenario = COMBAT_SCENARIOS[
                    case_index % len(COMBAT_SCENARIOS)
                ]
                run_case(
                    client, method, choice, first_case, experience, meridian
                )
                first_case = False
                case_index += 1
                print(
                    f"passed method={method} choice={choice} combat={scenario}"
                )
        print("First chapter runtime matrix passed.")
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
        print(f"First chapter runtime matrix failed: {error}", file=sys.stderr)
        raise SystemExit(1)
