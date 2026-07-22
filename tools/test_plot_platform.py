#!/usr/bin/env python3
"""Static contract checks for the chapter plot platform sources."""

from pathlib import Path
import re
import zipfile


ROOT = Path(__file__).resolve().parents[1]
PLOTD = ROOT / "tools/mudlib/plot/adm/daemons/plotd.c"
PLOT_COMBAT = ROOT / "tools/mudlib/plot/adm/daemons/plotcombat.c"
PLOT_COMMAND = ROOT / "tools/mudlib/plot/cmds/usr/plot.c"
ORIGIN = ROOT / "tools/mudlib/plot/adm/daemons/plot/origin_letter.c"
IMPORTER = ROOT / "tools/import_zjmud.sh"
CHAPTER2_SMOKE = ROOT / "tools/plot_chapter2_smoke.py"
CHAPTER2_RECOVERY = ROOT / "tools/plot_chapter2_recovery.py"
CHAPTER3_SMOKE = ROOT / "tools/plot_chapter3_smoke.py"
CHAPTER3_RECOVERY = ROOT / "tools/plot_chapter3_recovery.py"
CHAPTER4_SMOKE = ROOT / "tools/plot_chapter4_smoke.py"
CHAPTER4_RECOVERY = ROOT / "tools/plot_chapter4_recovery.py"
CHAPTER5_SMOKE = ROOT / "tools/plot_chapter5_smoke.py"
CHAPTER5_RECOVERY = ROOT / "tools/plot_chapter5_recovery.py"
NPC_LIFECYCLE = ROOT / "docs/ORIGINAL_PLOT_NPC_LIFECYCLE.md"
COMBAT_SPEC = ROOT / "docs/ORIGINAL_PLOT_COMBAT_SPEC.md"
RUNTIME_ZIP = ROOT / "app/src/main/assets/runtime/zjmud-runtime.zip"

CHAPTERS = [
    ("origin_letter_01", "notice"),
    ("returning_mark_02", "branch_hook"),
    ("silent_crossing_03", "watch_green_reed_ferry"),
    ("visitors_from_home_04", "letters_arrive"),
    ("where_rivers_end_05", "enter_salt_storehouse"),
]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> None:
    plotd = PLOTD.read_text(encoding="utf-8")
    plot_combat = PLOT_COMBAT.read_text(encoding="utf-8")
    command = PLOT_COMMAND.read_text(encoding="utf-8")
    origin = ORIGIN.read_text(encoding="utf-8")
    importer = IMPORTER.read_text(encoding="utf-8")
    chapter2_smoke = CHAPTER2_SMOKE.read_text(encoding="utf-8")
    chapter2_recovery = CHAPTER2_RECOVERY.read_text(encoding="utf-8")
    chapter3_smoke = CHAPTER3_SMOKE.read_text(encoding="utf-8")
    chapter3_recovery = CHAPTER3_RECOVERY.read_text(encoding="utf-8")
    chapter4_smoke = CHAPTER4_SMOKE.read_text(encoding="utf-8")
    chapter4_recovery = CHAPTER4_RECOVERY.read_text(encoding="utf-8")
    chapter5_smoke = CHAPTER5_SMOKE.read_text(encoding="utf-8")
    chapter5_recovery = CHAPTER5_RECOVERY.read_text(encoding="utf-8")
    lifecycle = NPC_LIFECYCLE.read_text(encoding="utf-8")
    combat_spec = COMBAT_SPEC.read_text(encoding="utf-8")

    require('#define HOMETOWN_ARC "hometown_letters_01"' in plotd, "unstable arc id")
    require("#define PLOT_SCHEMA_VERSION 2" in plotd, "plot schema migration missing")
    for marker in ("compatible_player_state", "valid_stage", "valid_status",
                   "reset_chapter_state", 'return "剧情存档来自更新版本'):
        require(marker in plotd, f"plot state hardening missing: {marker}")
    require("hometown_letters_arc_01" not in plotd, "legacy arc id entered runtime source")
    require("open_hidden_hold" not in plotd, "legacy chapter-three convergence stage")

    positions = []
    for chapter, initial_stage in CHAPTERS:
        position = plotd.find(f'"{chapter}" : ([')
        require(position >= 0, f"missing chapter registration: {chapter}")
        positions.append(position)
    require(positions == sorted(positions), "chapter registry order changed")
    chapter_blocks = {}
    for index, (chapter, _) in enumerate(CHAPTERS):
        end = positions[index + 1] if index + 1 < len(positions) else plotd.find("]);", positions[index])
        chapter_blocks[chapter] = plotd[positions[index] : end]

    for chapter, initial_stage in CHAPTERS:
        block = chapter_blocks[chapter]
        require(
            f'"initial_stage" : "{initial_stage}"' in block,
            f"wrong initial stage: {chapter}",
        )
        require('"final_stage"' in block and '"stages"' in block,
                f"stage registry missing: {chapter}")
    for chapter in ("origin_letter_01", "returning_mark_02", "silent_crossing_03", "visitors_from_home_04", "where_rivers_end_05"):
        require('"implemented" : 1' in chapter_blocks[chapter],
                f"implemented chapter disabled: {chapter}")
    for api in ("begin_chapter", "advance_stage", "record_choice", "set_arc_choice", "complete_chapter"):
        require(re.search(rf"\bint {api}\(", plotd) is not None, f"missing write API: {api}")
    require(
        plotd.count("caller_is_controller(plot_id, previous_object())") >= 6,
        "write API authorization drift",
    )
    require("call_other(" not in command, "player command gained dynamic LPC dispatch")
    require("file_name(" not in command, "player command exposes object paths")
    for marker in ("handle_action", "render_log", "solve_puzzle", "choose_custodian", "create_instance",
                   "protect_evidence", "puzzle_hint", "claim_token", "notify_entry"):
        require(marker in origin, f"first chapter feature missing: {marker}")
    require('"protect_evidence"' in origin, "counteraction stage missing")
    require('"ledger_custodian"' in origin, "custodian choice missing")
    for marker in ("query_player_profile", "configure_enemy", "meridian_boost", "street_blade",
                   "ledger_guard", "ledger_master", "martial_backgrounds",
                   "phase_requirements", "phase_actions", "perform_enemy_action"):
        require(marker in plot_combat, f"plot combat feature missing: {marker}")
    require('"wuhu-duanmendao"' in plot_combat, "Pei Jiu blade skill missing")
    require('"/clone/weapon/gangdao"' in plot_combat, "Pei Jiu blade missing")
    for marker in ("wanderer_escort_blade", "shaolin_lay_hand", "baichuan_water_escort",
                   "yunlong_courier", "gaibang_old_branch", "skill_map", "skill_prepare"):
        require(marker in plot_combat, f"fixed martial background missing: {marker}")
    require(plot_combat.count('"martial_background" : "baichuan_water_escort"') == 2,
            "Luo Qiniang martial continuity drift")
    require('"martial_background" : "gaibang_old_branch"' in plot_combat and
            '"force_skill" : "huntian-qigong"' in plot_combat and
            '"weapon_skill" : "dagou-bang"' in plot_combat,
            "Wen Shouzhuo Gaibang background missing")
    require('"staff.chan"' in plot_combat and '"staff.ban"' in plot_combat and
            '"staff.wugou"' in plot_combat,
            "Wen Shouzhuo phase perform ladder missing")
    require("setup_family" not in plot_combat and
            "random(sizeof(martial_backgrounds" not in plot_combat,
            "plot combat backgrounds became random")
    require("enable_plot_combat_actions" in plot_combat and
            "plot_combat_action" in (ROOT / "tools/mudlib/plot/clone/plot/origin_letter/pei_jiu.c").read_text(encoding="utf-8"),
            "automatic plot combat callback missing")
    with zipfile.ZipFile(RUNTIME_ZIP) as runtime:
        runtime_files = set(runtime.namelist())
    for skill in ("wuzheng-xinfa", "feiyan-zoubi", "houquan", "hunyuan-yiqi",
                  "shaolin-shenfa", "fengyun-shou", "sanhua-zhang", "lingyuan-xinfa",
                  "yunlong-shengong", "yunlong-shenfa", "yunlong-bian", "yunlong-shou",
                  "huntian-qigong", "dagou-bang", "dragon-strike"):
        require(f"kungfu/skill/{skill}.c" in runtime_files,
                f"martial background skill missing from runtime: {skill}")
    for perform in ("wuhu-duanmendao/duan", "fengyun-shou/qinna",
                    "yunlong-bian/chan", "dagou-bang/chan", "dagou-bang/ban",
                    "dagou-bang/wugou"):
        require(f"kungfu/skill/{perform}.c" in runtime_files,
                f"plot core perform missing from runtime: {perform}")
    for weapon in ("gangdao", "changbian"):
        require(f"clone/weapon/{weapon}.c" in runtime_files,
                f"plot martial weapon missing from runtime: {weapon}")
    require("clone/plot/where_rivers_end/qimei_staff.c" in runtime_files,
            "Wen Shouzhuo staff weapon missing from runtime")
    for marker in ("固定武学背景", "闻守拙", "丐帮旧支", "核心绝招", "不得把人物随机换成另一门派",
                   "不是必须写进主线对白"):
        require(marker in combat_spec, f"combat specification missing: {marker}")
    require('#define MAX_BOSS_PHASE 2' in plot_combat, "Boss phase cap missing")
    require('"max_phase" : 0' in plot_combat, "ordinary enemy gained Boss phases")
    require("exp_ratio_applied" in plot_combat and "restore_enemy" in plot_combat,
            "adaptive combat diagnostics or recovery missing")
    require('method == "battle" ? "killed" : "released"' in origin, "Pei Jiu fate drift")
    require("record_pei_death" in origin and '"pei_battle_won"' in origin,
            "Pei Jiu pre-claim death recovery missing")
    returning = ROOT / "tools/mudlib/plot/adm/daemons/plot/returning_mark.c"
    returning_text = returning.read_text(encoding="utf-8")
    for marker in ("branch_hook", "compare_marks", "protect_meng_si", "reconstruct_route",
                   "record_qing_death", "record_meng_injury", "green_hood", "set_arc_relation",
                   "set_pending_choice", "close_door", "show_half_token", "restore_enemy"):
        require(marker in returning_text, f"second chapter feature missing: {marker}")
    require("full_token_recovered" in returning_text, "second chapter token recovery missing")
    require("/d/plot/returning_mark/old_storehouse" in returning_text,
            "second chapter custom instance missing")
    silent = ROOT / "tools/mudlib/plot/adm/daemons/plot/silent_crossing.c"
    silent_text = silent.read_text(encoding="utf-8")
    for marker in ("watch_green_reed_ferry", "board_boat", "reach_cargo_hold", "save_a_he",
                   "inspect_three_cargos", "stop_burning_records", "settle_the_boat",
                   "record_a_he_injury", "record_luo_defeat", "ferry_matron", "baichuan_known",
                   "secure_deck", "home_letters_surge_known"):
        require(marker in silent_text, f"third chapter feature missing: {marker}")
    for room in ("boat_deck", "foredeck", "stern", "narrow_passage", "cargo_hold", "bilge"):
        require(f"/d/plot/silent_crossing/{room}" in silent_text,
                f"third chapter room missing: {room}")
    require("return_hold" in silent_text,
            "third chapter custom instance missing")
    for marker in ("query_instance_room", "open_instance_room", "close_instance_rooms"):
        require(marker in plotd, f"multi-room plot instance API missing: {marker}")
    for marker in ("COMBAT_SCENARIOS", "verify_adaptive_ladder", "verify_player_death_recovery",
                   "close_door", "show_half_token", "duplicate chapter completion changed score"):
        require(marker in chapter2_smoke, f"second chapter runtime coverage missing: {marker}")
    for marker in ("force_stop_and_reconnect", "alive-enemy", "enemy-death", "mark-chamber",
                   "injured-condition", "completed-idempotency"):
        require(marker in chapter2_recovery, f"second chapter recovery coverage missing: {marker}")
    for marker in ("BOARDING", "RECORD_METHODS", "RESCUES", "SETTLEMENTS",
                   "verify_adaptive_ladder", "verify_player_death_recovery", "return_hold"):
        require(marker in chapter3_smoke, f"third chapter runtime coverage missing: {marker}")
    for marker in ("boat-deck", "a-he-rescue", "burning-records", "records-saved",
                   "injured-condition", "completed-idempotency"):
        require(marker in chapter3_recovery, f"third chapter recovery coverage missing: {marker}")

    visitors = ROOT / "tools/mudlib/plot/adm/daemons/plot/visitors_from_home.c"
    visitors_text = visitors.read_text(encoding="utf-8")
    for marker in ("letters_arrive", "meet_lu_xiaoshuan", "verify_letters",
                   "choose_immediate_action", "catch_hua_yaozi", "decode_invitation",
                   "prepare_for_meeting", "record_hua_defeat", "courier_scout",
                   "meeting_place_known", "real_lu_captive_known"):
        require(marker in visitors_text, f"fourth chapter feature missing: {marker}")
    for room in ("relay_court", "post_road"):
        require(f"/d/plot/visitors_from_home/{room}" in visitors_text,
                f"fourth chapter room missing: {room}")
    require('arg == "visitors_from_home_04"' in command and
            '"/adm/daemons/plot/visitors_from_home"->render_log' in command,
            "fourth chapter log rendering missing")
    require('"courier_scout"' in plot_combat and '"martial_background" : "yunlong_courier"' in plot_combat and
            '"weapon" : "/clone/weapon/changbian"' in plot_combat,
            "fourth chapter fixed courier profile missing")
    for marker in ("ACTIONS", "CATCH_METHODS", "ORIGIN_GROUPS", "verify_adaptive_ladder",
                   "verify_format", "decode_place_grain", "chapter five becomes available",
                   "verify_martial_configuration", "whip.chan"):
        require(marker in chapter4_smoke, f"fourth chapter runtime coverage missing: {marker}")
    for marker in ("force_stop_and_reconnect", "injured-visitor", "alive-courier",
                   "courier-defeated", "partial-decode", "evidence-prepared",
                   "completed-idempotency"):
        require(marker in chapter4_recovery, f"fourth chapter recovery coverage missing: {marker}")
    stone = (ROOT / "tools/mudlib/plot/clone/plot/visitors_from_home/stone_seven.c").read_text(encoding="utf-8")
    hua = (ROOT / "tools/mudlib/plot/clone/plot/visitors_from_home/hua_yaozi.c").read_text(encoding="utf-8")
    require('set("plot_lifecycle", "instance_essential")' in stone and
            "void unconcious()" in stone and "void die()" in stone and "::die()" not in stone,
            "Stone Seven lifecycle contract missing")
    require('set("plot_lifecycle", "instance_continuity_opponent")' in hua and
            "record_hua_defeat" in hua and "destruct(this_object())" in hua and "::die()" not in hua,
            "Hua Yaozi lifecycle contract missing")
    for marker in ("adm/daemons/plot/visitors_from_home.c",
                   "d/plot/visitors_from_home/relay_court.c",
                   "d/plot/visitors_from_home/post_road.c",
                   "clone/plot/visitors_from_home/stone_seven.c",
                   "clone/plot/visitors_from_home/hua_yaozi.c",
                   "clone/plot/visitors_from_home/salt_invitation.c"):
        require(marker in importer, f"fourth chapter import validation missing: {marker}")

    chapter5 = ROOT / "tools/mudlib/plot/adm/daemons/plot/where_rivers_end.c"
    chapter5_text = chapter5.read_text(encoding="utf-8")
    for marker in ("enter_salt_storehouse", "hear_the_assembly", "rescue_real_lu",
                   "break_single_control", "save_people_and_records", "separate_two_hands",
                   "confront_wen_shouzhuo", "choose_archive_disposition", "arc_aftermath",
                   "record_luo_defeat", "record_wen_defeat", "set_arc_choice",
                   "shen_guanlan_known", "three_mountains_token_known"):
        require(marker in chapter5_text, f"fifth chapter feature missing: {marker}")
    chapter5_rooms = (
        "salt_gate", "drain_tunnel", "water_gate", "inspection_yard", "outer_gallery",
        "assembly_floor", "dark_cell", "guard_walk", "burning_archive", "index_room", "seal_chamber",
    )
    for room in chapter5_rooms:
        require(f"/d/plot/where_rivers_end/{room}" in chapter5_text,
                f"fifth chapter room missing: {room}")
    require('arg == "where_rivers_end_05"' in command and
            '"/adm/daemons/plot/where_rivers_end"->render_log' in command,
            "fifth chapter log rendering missing")
    for marker in ("ENTRIES", "CONTROL_METHODS", "COMBAT_SCENARIOS", "verify_adaptive_ladder",
                   "save_loose_pages", "solve_hands_grain_silence", "force_wen_yield",
                   "verify_wen_phase", "staff.wugou",
                   "duplicate chapter-five completion changed score"):
        require(marker in chapter5_smoke, f"fifth chapter runtime coverage missing: {marker}")
    for marker in ("force_stop_and_reconnect", "entry-route", "partial-testimony", "injured-real-lu",
                   "alive-guard", "fire-people-safe", "partial-two-hands", "boss-next-phase",
                   "confirmed-ending", "completed-idempotency"):
        require(marker in chapter5_recovery, f"fifth chapter recovery coverage missing: {marker}")
    real_lu = (ROOT / "tools/mudlib/plot/clone/plot/where_rivers_end/real_lu_xiaoshuan.c").read_text(encoding="utf-8")
    chapter5_luo = (ROOT / "tools/mudlib/plot/clone/plot/where_rivers_end/luo_qiniang.c").read_text(encoding="utf-8")
    wen = (ROOT / "tools/mudlib/plot/clone/plot/where_rivers_end/wen_shouzhuo.c").read_text(encoding="utf-8")
    require('set("plot_lifecycle", "instance_essential")' in real_lu and
            "void unconcious()" in real_lu and "void die()" in real_lu and "::die()" not in real_lu,
            "real Lu lifecycle contract missing")
    require('set("plot_lifecycle", "instance_continuity_opponent")' in chapter5_luo and
            "record_luo_defeat" in chapter5_luo and "::die()" not in chapter5_luo,
            "chapter-five Luo lifecycle contract missing")
    require('set("plot_lifecycle", "instance_boss_nonlethal")' in wen and
            "record_wen_defeat" in wen and "::die()" not in wen,
            "Wen Shouzhuo lifecycle contract missing")
    for marker in ("adm/daemons/plot/where_rivers_end.c",
                   "d/plot/where_rivers_end/salt_gate.c",
                   "d/plot/where_rivers_end/seal_chamber.c",
                   "clone/plot/where_rivers_end/real_lu_xiaoshuan.c",
                   "clone/plot/where_rivers_end/luo_qiniang.c",
                   "clone/plot/where_rivers_end/wen_shouzhuo.c",
                   "clone/plot/where_rivers_end/hometown_memorial.c"):
        require(marker in importer, f"fifth chapter import validation missing: {marker}")

    npc_root = ROOT / "tools/mudlib/plot"
    world_npcs = (
        npc_root / "d/plot/origin_letter/npc/tan_youji.c",
        npc_root / "d/plot/origin_letter/npc/xu_sanniang.c",
        npc_root / "d/plot/origin_letter/npc/zhou_shouyi.c",
    )
    for npc in world_npcs:
        require('set("plot_lifecycle", "world_refresh")' in npc.read_text(encoding="utf-8"),
                f"world NPC lifecycle missing: {npc.name}")
    hall = (npc_root / "d/plot/origin_letter/hall.c").read_text(encoding="utf-8")
    grain = (npc_root / "d/plot/origin_letter/grain_house.c").read_text(encoding="utf-8")
    require('set("objects"' in hall and "/npc/xu_sanniang" in hall,
            "Xu Sanniang is not room-owned")
    require('set("objects"' in grain and "/npc/zhou_shouyi" in grain,
            "Zhou Shouyi is not room-owned")
    require('"/d/plot/origin_letter/npc/tan_youji" : 1' in importer,
            "Tan Youji is not injected into a world room objects table")
    for forbidden in ("tan_youji_dead", "xu_sanniang_dead", "zhou_shouyi_dead"):
        require(forbidden not in origin, f"world NPC gained permanent death state: {forbidden}")

    for relative in ("clone/plot/returning_mark/meng_si.c", "clone/plot/silent_crossing/a_he.c",
                     "clone/plot/where_rivers_end/real_lu_xiaoshuan.c"):
        protected = (npc_root / relative).read_text(encoding="utf-8")
        require("void unconcious()" in protected and "void die()" in protected,
                f"protected NPC misses a fatal path: {relative}")
        require("reincarnate();" in protected and "::die()" not in protected,
                f"protected NPC uses native death semantics: {relative}")
        require('set("plot_lifecycle", "instance_essential")' in protected,
                f"protected NPC lifecycle metadata missing: {relative}")

    luo = (npc_root / "clone/plot/silent_crossing/luo_qiniang.c").read_text(encoding="utf-8")
    require("record_luo_defeat" in luo and "destruct(this_object())" in luo,
            "Luo Qiniang nonlethal defeat missing")
    require("::die()" not in luo, "Luo Qiniang still creates native death semantics")
    for marker in ("plot_spawned/pei_jiu", "plot_spawned/qing_peng_ke",
                   "plot_spawned/liu_dashao", "plot_spawned/a_he", "plot_spawned/luo_qiniang"):
        require(marker in origin + returning_text + silent_text,
                f"instance spawn-once marker missing: {marker}")
    for marker in ("world_refresh", "instance_essential", "instance_continuity_opponent",
                   "instance_boss_nonlethal", "unconcious()", "同一实例", "所有原创剧情 NPC",
                   "石七", "花鹞子", "真陆小栓", "闻守拙"):
        require(marker in lifecycle, f"NPC lifecycle document missing: {marker}")
    for forbidden in ('set("family")', 'delete("family")', "assign_apprentice", "relife/", "QUEST_D->bonus"):
        require(forbidden not in origin, f"first chapter violates identity boundary: {forbidden}")

    require('readonly PLOT_SOURCE_ROOT="$REPO_ROOT/tools/mudlib/plot"' in importer, "import root missing")
    require("iconv -f UTF-8 -t GB18030 \"$plot_source\"" in importer, "encoding conversion missing")
    require("'/adm/daemons/plotd'" in importer, "plot daemon preload missing")
    require("'/adm/daemons/plotcombat'" in importer, "plot combat daemon preload missing")
    require("Plot daemon preload validation failed." in importer, "preload validation missing")
    require("Plot global macro validation failed." in importer, "macro validation missing")
    require("Plot stable identifier validation failed." in importer, "identifier validation missing")
    require("Plot chapter entry validation failed." in importer, "chapter entry validation missing")

    plot_admin = (ROOT / "tools/mudlib/plot/cmds/adm/plotadmin.c").read_text(encoding="utf-8")
    for marker in ("PLOT_AUDIT", "state_chars", "instance_refs", "sizeof(objects())"):
        require(marker in plot_admin, f"plot runtime audit missing: {marker}")

    print("Plot platform static checks passed.")


if __name__ == "__main__":
    main()
