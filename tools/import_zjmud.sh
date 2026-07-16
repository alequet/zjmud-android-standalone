#!/usr/bin/env bash
set -euo pipefail

readonly UPSTREAM_ZJMUD_COMMIT="c56a166380d74858d7b4f0ba2817478ccea6b83d"
readonly EXPECTED_SHA256="2eee2ab12f81a3a7a7f5824a552f85f9d287c6d3dbfb03c4b1e2c0bfc2578ba0"
readonly EXPECTED_ZJMUD_PATCH_SHA256="e1f5157a544ae8523b78d2b8bd62a4d4f4bc1b0800e5e8f8c35a9604812c3cbe"
readonly EXPECTED_PATCHED_WEB_SHA256="d5ce16fb65a1beb9d39989244b832e4d776ff0db193f75074e8b69dad77b121c"
readonly SOURCE_ZIP="${1:-$HOME/zjmud-main.zip}"
readonly REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ASSET_ROOT="$REPO_ROOT/app/src/main/assets"
readonly ZJMUD_PATCH="$REPO_ROOT/tools/zjmud_patch/web_frontend.patch"
readonly WORK_ROOT="$(mktemp -d "${TMPDIR:-/tmp}/zjmud-import.XXXXXX")"

cleanup() {
  rm -rf "$WORK_ROOT"
}
trap cleanup EXIT

actual_sha256="$(shasum -a 256 "$SOURCE_ZIP" | awk '{print $1}')"
if [[ "$actual_sha256" != "$EXPECTED_SHA256" ]]; then
  echo "Unexpected source archive SHA-256: $actual_sha256" >&2
  exit 1
fi

mkdir -p "$WORK_ROOT/extracted" "$WORK_ROOT/payload" "$WORK_ROOT/web"
bsdtar -xf "$SOURCE_ZIP" -C "$WORK_ROOT/extracted"

source_roots=("$WORK_ROOT"/extracted/*)
if [[ "${#source_roots[@]}" != "1" || ! -d "${source_roots[0]}" ]]; then
  echo "Expected exactly one source directory in the zjmud archive." >&2
  exit 1
fi
readonly SOURCE_ROOT="${source_roots[0]}"
for required in config.ini adm clone cmds d data include inherit kungfu web/www/index.html; do
  if [[ ! -e "$SOURCE_ROOT/$required" ]]; then
    echo "Missing required source path: $required" >&2
    exit 1
  fi
done

zjmud_patch_sha256="$(shasum -a 256 "$ZJMUD_PATCH" | awk '{print $1}')"
if [[ "$zjmud_patch_sha256" != "$EXPECTED_ZJMUD_PATCH_SHA256" ]]; then
  echo "Unexpected zjmud patch SHA-256: $zjmud_patch_sha256" >&2
  exit 1
fi

# The fixed local snapshot already contains the upstream Web patch. Validate
# that relationship without applying the patch a second time.
patched_web_sha256="$(shasum -a 256 "$SOURCE_ROOT/web/www/main.js" | awk '{print $1}')"
if [[ "$patched_web_sha256" != "$EXPECTED_PATCHED_WEB_SHA256" ]]; then
  echo "Local zjmud snapshot does not contain the expected Web patch result: $patched_web_sha256" >&2
  exit 1
fi

rsync -a \
  --exclude='.DS_Store' \
  --exclude='*.rar' \
  --exclude='fluffos64/' \
  --exclude='start-64.bat' \
  --exclude='tangmen.exe.stackdump' \
  --exclude='web/' \
  --exclude='d/beijing/map/' \
  --exclude='d/beijing/liandan_lin - 副本.c' \
  "$SOURCE_ROOT/" "$WORK_ROOT/payload/"

perl -ni -e 'print unless m{^/adm/daemons/(messaged|payd|securityd)\s*$}' \
  "$WORK_ROOT/payload/adm/etc/preload"
perl -0pi -e 's{\A}{/adm/daemons/securityd\n}' "$WORK_ROOT/payload/adm/etc/preload"

patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/remove_anticheat.patch"
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/singleplayer_boosts.patch"
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/shaolin_unarmed_rewards.patch"
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/quest_fly.patch"
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/quest_commands.patch"
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/hide_room_paths.patch"
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/static_admins.patch"
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/full_character_save.patch"
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/challenger_runtime.patch"
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/challenger_level.patch"
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/wizard_reward_repair.patch"
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/cultivation_success_boost.patch"
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/runtime_error_fixes.patch"
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/ai_players.patch"
rm -f "$WORK_ROOT/payload/inherit/char/challenger.c.orig"
install -m 0644 "$REPO_ROOT/tools/mudlib/fullsave.c" "$WORK_ROOT/payload/feature/fullsave.c"
iconv -f UTF-8 -t GB18030 "$REPO_ROOT/tools/mudlib/ai-playerd.c" \
  > "$WORK_ROOT/payload/adm/daemons/ai_playerd.c"
iconv -f UTF-8 -t GB18030 "$REPO_ROOT/tools/mudlib/aiplayer.c" \
  > "$WORK_ROOT/payload/cmds/adm/aiplayer.c"
chmod 0644 \
  "$WORK_ROOT/payload/adm/daemons/ai_playerd.c" \
  "$WORK_ROOT/payload/cmds/adm/aiplayer.c"
mkdir -p "$WORK_ROOT/payload/d/standalone"
iconv -f UTF-8 -t GB18030 "$REPO_ROOT/tools/mudlib/ai-test-room.c" \
  > "$WORK_ROOT/payload/d/standalone/ai_test.c"
iconv -f UTF-8 -t GB18030 "$REPO_ROOT/tools/mudlib/ai-test-dummy.c" \
  > "$WORK_ROOT/payload/clone/npc/ai_test_dummy.c"
chmod 0644 "$WORK_ROOT/payload/d/standalone/ai_test.c" \
  "$WORK_ROOT/payload/clone/npc/ai_test_dummy.c"
printf '\n%s\n' '/adm/daemons/ai_playerd' >> "$WORK_ROOT/payload/adm/etc/preload"
install -m 0644 "$REPO_ROOT/tools/mudlib/offline-gchannel.c" \
  "$WORK_ROOT/payload/adm/daemons/network/services/gchannel.c"
install -m 0644 "$REPO_ROOT/tools/mudlib/offline-messaged.c" \
  "$WORK_ROOT/payload/adm/daemons/network/messaged.c"
install -m 0644 "$REPO_ROOT/tools/mudlib/offline-dns-master.c" \
  "$WORK_ROOT/payload/adm/daemons/network/dns_master.c"
install -m 0644 "$REPO_ROOT/tools/mudlib/offline-versiond.c" \
  "$WORK_ROOT/payload/adm/daemons/versiond.c"
install -m 0644 "$REPO_ROOT/tools/mudlib/offline-cmwhod.c" \
  "$WORK_ROOT/payload/adm/daemons/network/cmwhod.c"
install -m 0644 "$REPO_ROOT/tools/mudlib/offline-payd.c" \
  "$WORK_ROOT/payload/adm/daemons/payd.c"
install -m 0644 "$REPO_ROOT/tools/mudlib/offline-telnet.c" \
  "$WORK_ROOT/payload/shadow/telnet.c"
for network_service in "$WORK_ROOT"/payload/adm/daemons/network/services/*.c; do
  install -m 0644 "$REPO_ROOT/tools/mudlib/offline-network-service.c" \
    "$network_service"
done
install -m 0644 "$REPO_ROOT/tools/mudlib/offline-finger-service.c" \
  "$WORK_ROOT/payload/adm/daemons/network/fs.c"

perl -pi -e 's/cost = me->query_skill\(skl_id, 1\) \/ 2 \+ 100;/cost = (me->query_skill(skl_id, 1) \/ 2 + 100) * 20;/' \
  "$WORK_ROOT/payload/cmds/skill/derive.c"

if [[ "$(LC_ALL=C rg -c 'inherit "/feature/fullsave";' "$WORK_ROOT/payload/clone/user/user.c")" != "1" ||
      "$(LC_ALL=C rg -c 'save_full_character_state\(\);' "$WORK_ROOT/payload/clone/user/user.c")" != "1" ||
      "$(LC_ALL=C rg -c 'prepare_full_character_restore\(\);' "$WORK_ROOT/payload/clone/user/user.c")" != "1" ||
      "$(LC_ALL=C rg -c 'query_full_save_room\(\)' "$WORK_ROOT/payload/adm/daemons/logind.c")" != "1" ]]; then
  echo "Full character persistence patch validation failed." >&2
  exit 1
fi

if [[ "$(LC_ALL=C rg -F -c 'int     is_ai_player()' \
        "$WORK_ROOT/payload/clone/user/user.c")" != "1" ||
      "$(LC_ALL=C rg -F -c '"/adm/daemons/ai_playerd"->hear_say' \
        "$WORK_ROOT/payload/clone/user/user.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'int prepare_ai_runtime()' \
        "$WORK_ROOT/payload/clone/user/user.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'int prepare_ai_uid_export()' \
        "$WORK_ROOT/payload/clone/user/user.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'int prepare_ai_login()' \
        "$WORK_ROOT/payload/clone/user/login.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'login->prepare_ai_login()' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'user->prepare_ai_uid_export()' \
        "$WORK_ROOT/payload/adm/daemons/logind.c")" != "1" ||
      "$(LC_ALL=C rg -U -c 'if \(ai_login\)\n\s+\{\n\s+if \(! export_uid\(user\) \|\| ! export_uid\(ob\)\)' \
        "$WORK_ROOT/payload/adm/daemons/logind.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'save_error = catch(saved = login->save())' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'ob->activate_ai_login()' \
        "$WORK_ROOT/payload/adm/daemons/logind.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'catch(NAME_D->map_name(me->query("name"), id));' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'strsrch(myinfo[0], "ai_") == 0' \
        "$WORK_ROOT/payload/adm/daemons/logind.c")" != "1" ||
      "$(LC_ALL=C rg -F -c -- '->hear_ask(ob, me, topic);' \
        "$WORK_ROOT/payload/cmds/std/ask.c")" != "1" ||
      "$(LC_ALL=C rg -c '^/adm/daemons/ai_playerd$' \
        "$WORK_ROOT/payload/adm/etc/preload")" != "1" ||
      "$(LC_ALL=C rg -c '^/adm/daemons/securityd$' \
        "$WORK_ROOT/payload/adm/etc/preload")" != "1" ||
      "$(LC_ALL=C sed -n '1p' "$WORK_ROOT/payload/adm/etc/preload")" != \
        "/adm/daemons/securityd" ||
      "$(LC_ALL=C rg -F -c 'Persistent autonomous player actors' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'private string find_route_step' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "2" ||
      "$(LC_ALL=C rg -F -c 'mapping query_player_status' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'mapping query_metrics' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'void hear_ask' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'string *validate_profiles' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'mapping selftest_player' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'case "eat":' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'case "buy":' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'int start_combat_scenario' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'mapping query_scenario_status' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'private void clear_scenario_events' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "2" ||
      "$(LC_ALL=C rg -F -c 'int start_supply_activity' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'adapter_postcondition_failures' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "2" ||
      "$(LC_ALL=C rg -F -c 'private int run_patrol_activity' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "2" ||
      "$(LC_ALL=C rg -F -c 'private int run_social_activity' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "2" ||
      "$(LC_ALL=C rg -F -c 'private int rest_safely' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "2" ||
      "$(LC_ALL=C rg -F -c 'string contextual_greeting' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "2" ||
      "$(LC_ALL=C rg -F -c 'int start_profile_activity' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "1" ||
      "$(LC_ALL=C rg -F -c '#define AI_ACTIVITY_SCHEMA_VERSION 2' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'mapping query_recovery_status' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'private void reconcile_social_activities' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "2" ||
      "$(LC_ALL=C rg -F -c 'private int checkpoint_activity' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "2" ||
      "$(LC_ALL=C rg -F -c 'behavior patrol_stops=%d rests=%d social_meetings=%d' \
        "$WORK_ROOT/payload/cmds/adm/aiplayer.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'AI_RECOVERY id=%s schema=%d active=%d' \
        "$WORK_ROOT/payload/cmds/adm/aiplayer.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'status|pause|resume|reload|save' \
        "$WORK_ROOT/payload/cmds/adm/aiplayer.c")" != "2" ]]; then
  echo "AI player runtime validation failed." >&2
  exit 1
fi

if [[ "$(LC_ALL=C rg -F -c 'AI_STABILITY objects=%d players=%d profiles=%d paused=%d' \
        "$WORK_ROOT/payload/cmds/adm/aiplayer.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'AI_STABILITY_PLAYER id=%s loaded=%d errors=%d action_failures=%d' \
        "$WORK_ROOT/payload/cmds/adm/aiplayer.c")" != "1" ]]; then
  echo "AI stability diagnostics validation failed." >&2
  exit 1
fi

if [[ ! -f "$WORK_ROOT/payload/d/standalone/ai_test.c" ||
      ! -f "$WORK_ROOT/payload/clone/npc/ai_test_dummy.c" ||
      "$(LC_ALL=C rg -F -c 'set("ai_test_room", 1);' \
        "$WORK_ROOT/payload/d/standalone/ai_test.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'set("ai_test_dummy", 1);' \
        "$WORK_ROOT/payload/clone/npc/ai_test_dummy.c")" != "1" ]]; then
  echo "AI isolated scenario assets validation failed." >&2
  exit 1
fi

ai_profile_count="$(LC_ALL=C rg -o '"ai_[a-z]+" : \(\[' \
  "$WORK_ROOT/payload/adm/daemons/ai_playerd.c" | wc -l | tr -d ' ')"
if [[ "$ai_profile_count" != "5" ]]; then
  echo "Unexpected AI player profile count: $ai_profile_count" >&2
  exit 1
fi

ai_behavior_profile_count="$(LC_ALL=C rg -F -c '"talk_chance" :' \
  "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")"
ai_schedule_profile_count="$(LC_ALL=C rg -F -c '"schedule" : ([' \
  "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")"
ai_activity_profile_count="$(LC_ALL=C rg -F -c '"activities" : ({' \
  "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")"
if [[ "$ai_behavior_profile_count" != "5" ||
      "$ai_schedule_profile_count" != "5" ||
      "$ai_activity_profile_count" != "5" ]] ||
   ! LC_ALL=C rg -F -q 'inspect <id>' \
     "$WORK_ROOT/payload/cmds/adm/aiplayer.c" ||
   ! LC_ALL=C rg -F -q 'metrics [id]' \
     "$WORK_ROOT/payload/cmds/adm/aiplayer.c" ||
   ! LC_ALL=C rg -F -q 'events <id>' \
     "$WORK_ROOT/payload/cmds/adm/aiplayer.c" ||
   ! LC_ALL=C rg -F -q 'selftest [id]' \
     "$WORK_ROOT/payload/cmds/adm/aiplayer.c" ||
   ! LC_ALL=C rg -F -q 'activity supplies <id>' \
     "$WORK_ROOT/payload/cmds/adm/aiplayer.c"; then
  echo "AI player behavior profile validation failed." >&2
  exit 1
fi

for ai_route in \
  d/city/guangchang d/city/dongmen d/city/kedian d/city/zuixianlou d/city/xiaohuayuan \
  d/city/yaopu_neishi d/wudang/jiejianyan d/wudang/slxl3 \
  d/wudang/guangchang d/wudang/zixiaogate d/shaolin/shanmen \
  d/shaolin/shijie8 d/shaolin/guangchang1 d/shaolin/shijie10 \
  d/hangzhou/road10 d/hangzhou/kedian d/hangzhou/baoshishan \
  d/hangzhou/lingyinsi; do
  if [[ ! -f "$WORK_ROOT/payload/$ai_route.c" ]]; then
    echo "Missing AI player route room: $ai_route" >&2
    exit 1
  fi
done

if [[ "$(LC_ALL=C rg -F -c 'cost = (me->query_skill(skl_id, 1) / 2 + 100) * 20;' \
        "$WORK_ROOT/payload/cmds/skill/derive.c")" != "1" ]]; then
  echo "Jiqu absorption multiplier patch validation failed." >&2
  exit 1
fi

if [[ "$(LC_ALL=C rg -U -c '^\s*if \(objectp\(user\)\)\n\s+destruct\(user\);' \
        "$WORK_ROOT/payload/adm/daemons/logind.c")" != "1" ]]; then
  echo "New-character login cleanup patch validation failed." >&2
  exit 1
fi

cultivation_success_boost_count="$(LC_ALL=C rg -F -l \
  'random(4000) < me->query("con")' \
  "$WORK_ROOT/payload/cmds/skill/breakup.c" \
  "$WORK_ROOT/payload/cmds/skill/animaout.c" | wc -l | tr -d ' ')"
if [[ "$cultivation_success_boost_count" != "2" ]] ||
   LC_ALL=C rg -q 'random\(40000\) < me->query\("con"\)' \
     "$WORK_ROOT/payload/cmds/skill/breakup.c" \
     "$WORK_ROOT/payload/cmds/skill/animaout.c"; then
  echo "Cultivation success-rate boost validation failed." >&2
  exit 1
fi

for yitian_npc in aer asan; do
  if [[ "$(LC_ALL=C rg -U -c 'destruct\(this_object\(\)\);\n\s+return;' \
        "$WORK_ROOT/payload/d/tulong/yitian/npc/$yitian_npc.c")" != "1" ]]; then
    echo "Yitian duel death-flow patch validation failed: $yitian_npc" >&2
    exit 1
  fi
done

if [[ "$(LC_ALL=C rg -F -c 'replace_string(msg, "\n", NOR "\n")' \
      "$WORK_ROOT/payload/kungfu/skill/qiankun-danuoyi.c")" != "4" ]] ||
   LC_ALL=C rg -q 'msg = HI[GY] \+ msg \+ NOR;' \
     "$WORK_ROOT/payload/kungfu/skill/qiankun-danuoyi.c"; then
  echo "Qiankun Danuoyi status-packet framing patch validation failed." >&2
  exit 1
fi

if [[ "$(LC_ALL=C rg -U -c 'destruct\(ob\);\n\s+return;' \
      "$WORK_ROOT/payload/clone/user/comb_ob.c")" != "1" ]] ||
   [[ "$(LC_ALL=C rg -U -c 'if \(objectp\(ob\)\)\n\s+ob->delete_temp\("fight_team"\);' \
      "$WORK_ROOT/payload/clone/user/comb_ob.c")" != "2" ]]; then
  echo "Combined-combat object lifetime patch validation failed." >&2
  exit 1
fi

if [[ "$(LC_ALL=C rg -F -c 'destruct(ob);' \
      "$WORK_ROOT/payload/cmds/arch/register.c")" != "1" ]] ||
   [[ "$(LC_ALL=C rg -U -c 'if \(! body\)\n\s+return 0;' \
      "$WORK_ROOT/payload/cmds/arch/register.c")" != "1" ]]; then
  echo "Registration object lifetime patch validation failed." >&2
  exit 1
fi

if [[ "$(LC_ALL=C rg -F -c \
      'HIG "VIP Jiuyang reward book restored." NOR "\n"' \
      "$WORK_ROOT/payload/adm/npc/wizard.c")" != "1" ]] ||
   LC_ALL=C rg -q 'VIP Jiuyang reward book restored\\n" NOR' \
     "$WORK_ROOT/payload/adm/npc/wizard.c"; then
  echo "Jiuyang repair status-packet framing patch validation failed." >&2
  exit 1
fi

for quest_command in cmds/usr/fly.c cmds/imm/goto.c cmds/std/kill.c; do
  if [[ "$(LC_ALL=C rg -F -c 'if (arg == "quest")' \
        "$WORK_ROOT/payload/$quest_command")" != "1" ]] ||
     [[ "$(LC_ALL=C rg -F -c 'quest = me->query("quest");' \
        "$WORK_ROOT/payload/$quest_command")" != "1" ]] ||
     [[ "$(LC_ALL=C rg -F -c 'quest = me->query("bquest");' \
        "$WORK_ROOT/payload/$quest_command")" != "1" ]] ||
     [[ "$(LC_ALL=C rg -F -c '! stringp(quest["id"])' \
        "$WORK_ROOT/payload/$quest_command")" != "1" ]]; then
    echo "Quest target command patch validation failed: $quest_command" >&2
    exit 1
  fi
done

quest_argument_count="$(LC_ALL=C rg -F -o 'arg = quest["id"];' \
  "$WORK_ROOT/payload/cmds/imm/goto.c" \
  "$WORK_ROOT/payload/cmds/std/kill.c" | wc -l | tr -d ' ')"
if [[ "$quest_argument_count" != "2" ]] ||
   [[ "$(LC_ALL=C rg -F -c 'goto_inventory = 0;' \
      "$WORK_ROOT/payload/cmds/imm/goto.c")" != "2" ]] ||
   [[ "$(LC_ALL=C rg -F -c 'target = find_living(quest["id"]);' \
      "$WORK_ROOT/payload/cmds/imm/goto.c")" != "1" ]]; then
  echo "Quest goto/kill argument patch validation failed." >&2
  exit 1
fi

perl -ni -e 'print unless /"(?:intermud|intermud_emote|intermud_channel|filter)"\s*:/' \
  "$WORK_ROOT/payload/adm/daemons/channeld.c"
perl -0pi -e 's/(\s+if\( sscanf\(target, "%s\@%s", target, mud\)==2 \) \{)/\n    if (sscanf(target, "%*s\@%*s") == 2)\n        return notify_fail("Inter-MUD messaging is unavailable in standalone mode.\\n");\n$1/' \
  "$WORK_ROOT/payload/cmds/std/tell.c"
perl -0pi -e 's/(\n\tif \(sscanf\(target, "%s\@%s", target, mud\) == 2\))/\n\tif (sscanf(target, "%*s\@%*s") == 2)\n\t\treturn notify_fail("Inter-MUD messaging is unavailable in standalone mode.\\n");\n$1/' \
  "$WORK_ROOT/payload/cmds/std/reply.c"
perl -0pi -e 's/(\n\tif \(! me\) me = this_player\(\);)/\n\tif (sscanf(name, "%*s\@%*s") == 2)\n\t\treturn "Inter-MUD lookup is unavailable in standalone mode.\\n";\n$1/' \
  "$WORK_ROOT/payload/adm/daemons/fingerd.c"
perl -0pi -e 's/(\n\tif \(! stringp\(info\[MESSAGE\]\)\))/\n\tif (sscanf(info[MSGTO], "%*s\@%*s") == 2)\n\t\treturn MESSAGE_D->error_msg("Inter-MUD messaging is unavailable in standalone mode.\\n");\n$1/' \
  "$WORK_ROOT/payload/cmds/chat/tell.c"
perl -0pi -e 's/(\n\tmsg = FINGER_D->finger_user\(info\[ARG\], me\);)/\n\tif (sscanf(info[ARG], "%*s\@%*s") == 2)\n\t\treturn MESSAGE_D->error_msg("Inter-MUD lookup is unavailable in standalone mode.\\n");\n$1/' \
  "$WORK_ROOT/payload/cmds/chat/finger.c"
perl -0pi -e 's/(\n\tseteuid\(getuid\(\)\);)/\n\treturn notify_fail("External telnet is unavailable in standalone mode.\\n");\n$1/' \
  "$WORK_ROOT/payload/cmds/adm/telnet.c"
perl -pi -e 's/MESSAGE_D->tell_object\(/MESSAGE_D->tell_user(/' \
  "$WORK_ROOT/payload/clone/user/chatter.c"
perl -0pi -e 's#(ob = new\("/clone/book/jiuyang-copy"\);\n)(\s*)ob->move\(me, 1\);#$1$2if (ob->move(me, 1))\n$2\tme->set("zjvip/vipgift/jiuyang_book_repaired", 1);#g' \
  "$WORK_ROOT/payload/adm/npc/wizard.c"
perl -pi -e 's#/clone/book/jiuyang-copy#/clone/book/jiuyang-book#g; s#/clone/shizhe/fushougao#/clone/gift/fushougao#g; s#/clone/shizhe/(?:xiaohuan|lingzhi)#/clone/gift/lingzhi#g' \
  "$WORK_ROOT/payload/adm/npc/wizard.c"

if LC_ALL=C rg -q '"intermud"\s*:|"intermud_emote"\s*:|"intermud_channel"\s*:|"filter"\s*:' \
     "$WORK_ROOT/payload/adm/daemons/channeld.c"; then
  echo "Offline channel patch validation failed." >&2
  exit 1
fi

if LC_ALL=C rg -q 'DNS_MASTER|send_udp|socket_' \
     "$WORK_ROOT/payload/adm/daemons/network/services/gchannel.c"; then
  echo "Offline gchannel replacement validation failed." >&2
  exit 1
fi

for offline_daemon in \
  "$WORK_ROOT/payload/adm/daemons/network/dns_master.c" \
  "$WORK_ROOT/payload/adm/daemons/network/messaged.c" \
  "$WORK_ROOT/payload/adm/daemons/network/cmwhod.c" \
  "$WORK_ROOT/payload/adm/daemons/payd.c" \
  "$WORK_ROOT/payload/adm/daemons/versiond.c" \
  "$WORK_ROOT/payload/shadow/telnet.c"; do
  if LC_ALL=C rg -q 'DNS_MASTER|GTELL|\bsocket_(create|bind|listen|accept|connect|write|close|release|acquire|status|error)\s*\(|\bresolve\s*\(' \
       "$offline_daemon"; then
    echo "Offline daemon replacement still references network APIs: $offline_daemon" >&2
    exit 1
  fi
done

if [[ "$(LC_ALL=C rg -c '^varargs void tell_user\(' \
        "$WORK_ROOT/payload/adm/daemons/network/messaged.c")" != "1" ||
      "$(LC_ALL=C rg -c '^int (send_msg_to|error_msg)\(' \
        "$WORK_ROOT/payload/adm/daemons/network/messaged.c")" != "2" ||
      "$(LC_ALL=C rg -F -c 'MESSAGE_D->tell_user(' \
        "$WORK_ROOT/payload/clone/user/chatter.c")" != "18" ||
      "$(LC_ALL=C rg -F -o 'MESSAGE_D->tell_object(' \
        "$WORK_ROOT/payload/clone/user/chatter.c" | wc -l | tr -d ' ')" != "0" ||
      "$(LC_ALL=C rg -F -c 'int is_release_server() { return 1; }' \
        "$WORK_ROOT/payload/adm/daemons/versiond.c")" != "1" ]]; then
  echo "Offline daemon API validation failed." >&2
  exit 1
fi

offline_guard_count="$(LC_ALL=C rg -l \
  'unavailable in standalone mode' \
  "$WORK_ROOT/payload/cmds/std/tell.c" \
  "$WORK_ROOT/payload/cmds/std/reply.c" \
  "$WORK_ROOT/payload/adm/daemons/fingerd.c" \
  "$WORK_ROOT/payload/cmds/chat/tell.c" \
  "$WORK_ROOT/payload/cmds/chat/finger.c" \
  "$WORK_ROOT/payload/cmds/adm/telnet.c" | wc -l | tr -d ' ')"
if [[ "$offline_guard_count" != "6" ]]; then
  echo "Offline network command guard validation failed." >&2
  exit 1
fi

if LC_ALL=C rg -q '/clone/book/jiuyang-copy|/clone/shizhe/(fushougao|xiaohuan|lingzhi)' \
     "$WORK_ROOT/payload/adm/npc/wizard.c" ||
   [[ "$(LC_ALL=C rg -F -c 'repair_jiuyang_book(this_player());' \
        "$WORK_ROOT/payload/adm/npc/wizard.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'void repair_jiuyang_book(object me)' \
        "$WORK_ROOT/payload/adm/npc/wizard.c")" != "2" ||
      "$(LC_ALL=C rg -F -c 'me->query_skill("jiuyang-shengong", 1) != 50' \
        "$WORK_ROOT/payload/adm/npc/wizard.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'me->set("zjvip/vipgift/jiuyang_book_repaired", 1);' \
        "$WORK_ROOT/payload/adm/npc/wizard.c")" != "4" ||
      "$(LC_ALL=C rg -F -c 'new("/clone/book/jiuyang-book")' \
        "$WORK_ROOT/payload/adm/npc/wizard.c")" != "3" ||
      "$(LC_ALL=C rg -F -c 'new("/clone/gift/fushougao")' \
        "$WORK_ROOT/payload/adm/npc/wizard.c")" != "4" ||
      "$(LC_ALL=C rg -F -c 'new("/clone/gift/lingzhi")' \
        "$WORK_ROOT/payload/adm/npc/wizard.c")" != "3" ]]; then
  echo "Wizard reward object path validation failed." >&2
  exit 1
fi

while IFS= read -r reward_path; do
  if [[ ! -f "$WORK_ROOT/payload${reward_path}.c" ]]; then
    echo "Wizard reward object is missing: $reward_path" >&2
    exit 1
  fi
done < <(LC_ALL=C rg -o 'new\("/clone/[^"]+"\)' \
  "$WORK_ROOT/payload/adm/npc/wizard.c" | sed -E 's/^new\("(.*)"\)$/\1/' | sort -u)

network_efun_pattern='\b(socket_create|socket_bind|socket_listen|socket_accept|socket_connect|socket_write|socket_close|socket_release|socket_acquire|socket_status|socket_address|socket_error|resolve|external_start|db_connect)\s*\('
if LC_ALL=C rg -q "$network_efun_pattern" "$WORK_ROOT/payload" --glob '*.c'; then
  echo "Payload still contains a callable network efun." >&2
  LC_ALL=C rg -n "$network_efun_pattern" "$WORK_ROOT/payload" --glob '*.c' >&2
  exit 1
fi

removed_network_target_pattern='\b(FTP_D|INETD|NETMAIL_D|TELNET_D|MAIL_SERVER|NAME_SERVER|USERID_D|UDP_MASTER|INTER_CHAN_D|TS_D|RWHO_D)\s*->|"/adm/daemons/network/(inetd|telnetd|netmail|ms|name_server|userid|pingd|udp_master|inter_chan|ts|rwho)"\s*->'
if LC_ALL=C rg -q "$removed_network_target_pattern" "$WORK_ROOT/payload" --glob '*.c'; then
  echo "Payload still calls a removed network daemon." >&2
  LC_ALL=C rg -n "$removed_network_target_pattern" "$WORK_ROOT/payload" --glob '*.c' >&2
  exit 1
fi

offline_daemon_marker_count="$(LC_ALL=C rg -l '^// Offline replacement' \
  "$WORK_ROOT/payload/adm/daemons/network/dns_master.c" \
  "$WORK_ROOT/payload/adm/daemons/network/messaged.c" \
  "$WORK_ROOT/payload/adm/daemons/network/cmwhod.c" \
  "$WORK_ROOT/payload/adm/daemons/network/services/gchannel.c" \
  "$WORK_ROOT/payload/adm/daemons/payd.c" \
  "$WORK_ROOT/payload/adm/daemons/versiond.c" \
  "$WORK_ROOT/payload/shadow/telnet.c" | wc -l | tr -d ' ')"
if [[ "$offline_daemon_marker_count" != "7" ]]; then
  echo "Offline daemon replacement marker validation failed." >&2
  exit 1
fi

offline_service_count="$(LC_ALL=C rg -l '^// Offline replacement' \
  "$WORK_ROOT"/payload/adm/daemons/network/services/*.c | wc -l | tr -d ' ')"
if [[ "$offline_service_count" != "22" ||
      "$(LC_ALL=C rg -c '^// Offline replacement' \
        "$WORK_ROOT/payload/adm/daemons/network/fs.c")" != "1" ]]; then
  echo "Offline network service replacement validation failed." >&2
  exit 1
fi

family_reward_multiplier_count="$(LC_ALL=C rg -c \
  '^\s*(exp|pot|mar|shen|score|weiwang) \*= 100;$' \
  "$WORK_ROOT/payload/adm/daemons/questd.c")"
family_skill_chance_count="$(LC_ALL=C rg -F -c \
  'random(10000) < 100' "$WORK_ROOT/payload/adm/daemons/questd.c")"
if [[ "$family_reward_multiplier_count" != "6" ||
      "$(LC_ALL=C rg -F -c 'who->add("gongxian", (flag || b["family_reward"]) ? 100 : 1);' \
        "$WORK_ROOT/payload/adm/daemons/questd.c")" != "1" ||
      "$family_skill_chance_count" != "2" ]]; then
  echo "Family quest reward boost validation failed." >&2
  exit 1
fi

# Old MudOS accepted numeric values in string concatenation in more places than
# this driver. Preserve the GB18030 message bytes and format only the numbers.
perl -pi -e 's/\+ boss\[killer\[i\]\] \+/+ sprintf("%d", boss[killer[i]]) +/' \
  "$WORK_ROOT/payload/inherit/char/challenger.c"
perl -pi -e 's{\+boss\[killer\[i\]\]/(10|20|100)\+}{+sprintf("%d", boss[killer[i]]/$1)+}g' \
  "$WORK_ROOT/payload/inherit/char/challenger.c"

if [[ "$(LC_ALL=C rg -F -c 'sprintf("%d", boss[killer[i]])' \
        "$WORK_ROOT/payload/inherit/char/challenger.c")" != "1" ||
      "$(LC_ALL=C rg -o 'sprintf\("%d", boss\[killer\[i\]\]/(10|20|100)\)' \
        "$WORK_ROOT/payload/inherit/char/challenger.c" | wc -l | tr -d ' ')" != "4" ||
      "$(LC_ALL=C rg -F -c 'random(total_damage)' \
        "$WORK_ROOT/payload/inherit/char/challenger.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'call_out("destruct_me", 0);' \
        "$WORK_ROOT/payload/inherit/char/challenger.c")" != "2" ||
      "$(LC_ALL=C rg -F -o 'call_out("destruct",' \
        "$WORK_ROOT/payload/inherit/char/challenger.c" | wc -l | tr -d ' ')" != "0" ]]; then
  echo "Challenger runtime compatibility patch validation failed." >&2
  exit 1
fi

if [[ -e "$WORK_ROOT/payload/inherit/char/challenger.c.orig" ]]; then
  echo "Challenger patch left an unexpected backup file." >&2
  exit 1
fi

if [[ "$(LC_ALL=C rg -F -c 'lv = 220;' \
      "$WORK_ROOT/payload/adm/daemons/boss/challenge.c")" != "1" ]] ||
   LC_ALL=C rg -F -q 'lv = 120;' \
     "$WORK_ROOT/payload/adm/daemons/boss/challenge.c"; then
  echo "Challenger level patch validation failed." >&2
  exit 1
fi

perl -pi -e 's/me->improve_skill\("finger", 1 \+ random\(me->query\("str"\)\*2\)\);/me->improve_skill("finger", (1 + random(me->query("str")*2)) * 500);/' "$WORK_ROOT/payload/d/shaolin/beilin3.c"
perl -pi -e 's/me->improve_skill\("claw", 1 \+ random\(me->query\("str"\)\*2\)\);/me->improve_skill("claw", (1 + random(me->query("str")*2)) * 500);/' "$WORK_ROOT/payload/d/shaolin/beilin3.c"
perl -pi -e 's/me->improve_skill\("strike", 1 \+ me->query\("str"\)\*2\);/me->improve_skill("strike", (1 + me->query("str")*2) * 500);/' "$WORK_ROOT/payload/d/shaolin/beilin3.c"
perl -pi -e 's/me->improve_skill\("cuff", 1 \+ random\(me->query\("str"\)\*2\)\);/me->improve_skill("cuff", (1 + random(me->query("str")*2)) * 500);/' "$WORK_ROOT/payload/d/shaolin/beilin3.c"
perl -pi -e 's/me->improve_skill\("hand", 1 \+ random\(me->query\("int"\)\*2\)\);/me->improve_skill("hand", (1 + random(me->query("int")*2)) * 500);/' "$WORK_ROOT/payload/d/shaolin/beilin3.c"
perl -pi -e 's/me->improve_skill\(skill\[i-1\], 5 \+ random\(30\)\);/me->improve_skill(skill[i-1], (5 + random(30)) * 500);/' "$WORK_ROOT/payload/clone/book/xisuijing.c"

if [[ "$(LC_ALL=C rg -c 'improve_skill.*\* 500\);' "$WORK_ROOT/payload/d/shaolin/beilin3.c")" != "6" ||
      "$(LC_ALL=C rg -c 'improve_skill.*\* 500\);' "$WORK_ROOT/payload/clone/book/xisuijing.c")" != "1" ]]; then
  echo "Shaolin unarmed reward patch validation failed." >&2
  exit 1
fi

perl -pi -e 's/\+environment\(ob\)->query\("short"\)\+NOR/\+ZJURL("cmds:fly quest")+environment(ob)->query("short")+NOR/' \
  "$WORK_ROOT/payload/clone/vip/zbagua.c" \
  "$WORK_ROOT/payload/clone/vip/zbagua1.c" \
  "$WORK_ROOT/payload/clone/gift/bagua.c"
perl -pi -e 's/\+environment\(ob\)->query\("short"\)\+"/\+ZJURL("cmds:fly quest")+environment(ob)->query("short")+NOR+"/' \
  "$WORK_ROOT/payload/clone/shizhe/bagua.c"
install -m 0644 "$REPO_ROOT/tools/mudlib/death-block.c" "$WORK_ROOT/payload/d/death/block.c"
install -m 0644 "$REPO_ROOT/tools/mudlib/death-shenxun.c" "$WORK_ROOT/payload/d/death/shenxun.c"

rsync -a --exclude='.DS_Store' "$SOURCE_ROOT/web/www/" "$WORK_ROOT/web/"

non_ascii_path="$(find "$WORK_ROOT/payload" -type f -print | LC_ALL=C rg '[^ -~]' || true)"
if [[ -n "$non_ascii_path" ]]; then
  echo "Payload contains an unexpected non-ASCII path:" >&2
  echo "$non_ascii_path" >&2
  exit 1
fi

find "$WORK_ROOT/payload" -exec touch -h -t 202101010000 {} +

mkdir -p "$ASSET_ROOT/runtime" "$ASSET_ROOT/web"
(
  cd "$WORK_ROOT/payload"
  find . -mindepth 1 -print | LC_ALL=C sort | zip -X -q "$WORK_ROOT/zjmud-runtime.zip" -@
)

rsync -a --delete "$WORK_ROOT/web/" "$ASSET_ROOT/web/"
install -m 0644 "$REPO_ROOT/tools/web/local-transport.js" "$ASSET_ROOT/web/local-transport.js"

perl -0pi -e 's#\s*<script src="/socket\.io/socket\.io\.js" charset="gb2312"></script>#\n  <script src="local-transport.js"></script>#' "$ASSET_ROOT/web/index.html"
perl -0pi -e 's#\s*<script src="http://cdn\.bootcss\.com/blueimp-md5/1\.1\.0/js/md5\.min\.js"></script>##' "$ASSET_ROOT/web/index.html"
perl -0pi -e 's/var sock = io\.connect\(\);/var sock = localTransport.connect();/' "$ASSET_ROOT/web/main.js"
perl -ni -e 'print unless /value="游戏充值"|value="游戏主页"/' "$ASSET_ROOT/web/main.js"
patch -s -p1 -d "$ASSET_ROOT/web" < "$REPO_ROOT/tools/web/main.android.patch"
perl -0pi -e 's/hudongob\.dialog\("close"\);/closeHudongDialog();/g' \
  "$ASSET_ROOT/web/main.js"
perl -0pi -e 's#function cmds\(str\) \{#function closeHudongDialog() {\n\tif (hudongob.hasClass("ui-dialog-content")) {\n\t\thudongob.dialog("close");\n\t}\n}\n\nfunction cmds(str) {#' \
  "$ASSET_ROOT/web/main.js"
perl -0pi -e 's#/\*\nfunction logincheck\(id,pass\) \{.*?\}; \*/\n##s' \
  "$ASSET_ROOT/web/main.js"
perl -0pi -e 's#/\* function registercheck\(\) \{.*?\n\} \*/\n##s' \
  "$ASSET_ROOT/web/main.js"
perl -0pi -e 's#function paym\(\)\n\{.*?\n\}#function paym()\n{\n}#s' \
  "$ASSET_ROOT/web/main.js"
perl -0pi -e 's#function main_login\(\)\n\{.*?\n\}#function main_login()\n{\n}#s' \
  "$ASSET_ROOT/web/main.js"
install -m 0644 "$ASSET_ROOT/web/main.js" "$ASSET_ROOT/web/js/main.js"
perl -0pi -e 's/\n\tdocument\.oncontextmenu = function\(\) \{\n\t\treturn false;\n\t\}//' \
  "$ASSET_ROOT/web/main.js" "$ASSET_ROOT/web/js/main.js"

if LC_ALL=C rg -q 'document\.oncontextmenu' \
     "$ASSET_ROOT/web/main.js" "$ASSET_ROOT/web/js/main.js"; then
  echo "Web text-selection context menu is still disabled." >&2
  exit 1
fi

if [[ "$(LC_ALL=C rg -F -c 'function closeHudongDialog()' \
        "$ASSET_ROOT/web/main.js")" != "1" ||
      "$(LC_ALL=C rg -F -c 'hudongob.dialog("close");' \
        "$ASSET_ROOT/web/main.js")" != "1" ]]; then
  echo "Web dialog-close guard validation failed." >&2
  exit 1
fi

if LC_ALL=C rg -q '92mud\.com|\$\.ajax\s*\(|window\.open\s*\(|io\.connect\s*\(' \
     "$ASSET_ROOT/web/main.js" "$ASSET_ROOT/web/js/main.js"; then
  echo "Web client still contains executable external-network code." >&2
  exit 1
fi

if [[ "$(LC_ALL=C rg -F -c "sock.emit('stream', myid + '║' + mypass" \
        "$ASSET_ROOT/web/main.js")" != "1" ||
      "$(LC_ALL=C rg -F -c 'logincheck(myid, mypass);' \
        "$ASSET_ROOT/web/main.js")" != "1" ||
      "$(LC_ALL=C rg -F -c "sock.emit('stream',mysex.val()+'║001║'" \
        "$ASSET_ROOT/web/main.js")" != "1" ]]; then
  echo "Web local login or registration flow validation failed." >&2
  exit 1
fi

install -m 0644 "$WORK_ROOT/zjmud-runtime.zip" "$ASSET_ROOT/runtime/zjmud-runtime.zip"

runtime_sha256="$(shasum -a 256 "$ASSET_ROOT/runtime/zjmud-runtime.zip" | awk '{print $1}')"
runtime_bytes="$(stat -f '%z' "$ASSET_ROOT/runtime/zjmud-runtime.zip")"
printf 'source_kind=%s\nupstream_commit=%s\nsource_sha256=%s\nzjmud_patch_sha256=%s\npatched_web_sha256=%s\nruntime_sha256=%s\nruntime_bytes=%s\n' \
  "local_patched_snapshot" "$UPSTREAM_ZJMUD_COMMIT" "$actual_sha256" \
  "$zjmud_patch_sha256" "$patched_web_sha256" \
  "$runtime_sha256" "$runtime_bytes" \
  > "$ASSET_ROOT/runtime/manifest.properties"
cat <<EOF
Imported zjmud assets
source_kind=local_patched_snapshot
upstream_commit=$UPSTREAM_ZJMUD_COMMIT
source_sha256=$actual_sha256
zjmud_patch_sha256=$zjmud_patch_sha256
patched_web_sha256=$patched_web_sha256
runtime_sha256=$runtime_sha256
runtime_bytes=$runtime_bytes
EOF
