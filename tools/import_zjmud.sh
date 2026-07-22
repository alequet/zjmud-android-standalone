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
readonly PLOT_SOURCE_ROOT="$REPO_ROOT/tools/mudlib/plot"
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
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/character_skill_retention.patch"
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/attribute_skill_retention.patch"
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/cultivation_success_boost.patch"
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/runtime_error_fixes.patch"
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/ai_players.patch"
rm -f "$WORK_ROOT/payload/inherit/char/challenger.c.orig"

wizard_character_utf8="$WORK_ROOT/wizard-character.utf8"
iconv -f GB18030 -t UTF-8 "$WORK_ROOT/payload/adm/npc/wizard.c" > "$wizard_character_utf8"
perl -0pi -e 's/注意：改变性格之后不符合目前学习条件的技能将被删除！/注意：改变性格之后与新性格冲突的武学将被删除！/g; s/洗点后所有不符合学习条件的技能都将删除/洗点后先天属性不符合要求的武学将被删除/g; s/或大于39//g; s/\|\|tmpstr>39\|\|tmpint>39\|\|tmpcon>39\|\|tmpdex>39//g; s{(attribute_skilld"->valid_for_attributes\(skills\[i\], me\).*?)已经不符合学习条件，自动删除}{$1不符合当前先天属性要求，自动删除}s; s{(attribute_skilld"->valid_for_attributes\(skills\[i\], me\).*?)洗点后已经不符合学习条件，自动删除}{$1洗点后不符合先天属性要求，自动删除}s; s/心狠手辣，宗师心法-九阴神功/光明磊落，宗师心法-九阴神功/g; s/光明磊落，宗师心法-南海玄功/心狠手辣，宗师心法-南海玄功/g' \
  "$wizard_character_utf8"
iconv -f UTF-8 -t GB18030 "$wizard_character_utf8" > "$WORK_ROOT/payload/adm/npc/wizard.c"

perform_utf8="$WORK_ROOT/perform.utf8"
iconv -f GB18030 -t UTF-8 "$WORK_ROOT/payload/cmds/skill/perform.c" > "$perform_utf8"
perl -0pi -e 's{\n\t\tif \(me->query_skill\("count", 1\)\)\n\t\t\treturn notify_fail\(".*?"\);\n}{}s' "$perform_utf8"
iconv -f UTF-8 -t GB18030 "$perform_utf8" > "$WORK_ROOT/payload/cmds/skill/perform.c"

install -m 0644 "$REPO_ROOT/tools/mudlib/fullsave.c" "$WORK_ROOT/payload/feature/fullsave.c"
iconv -f UTF-8 -t GB18030 "$REPO_ROOT/tools/mudlib/ai-playerd.c" \
  > "$WORK_ROOT/payload/adm/daemons/ai_playerd.c"
iconv -f UTF-8 -t GB18030 "$REPO_ROOT/tools/mudlib/aiplayer.c" \
  > "$WORK_ROOT/payload/cmds/adm/aiplayer.c"
iconv -f UTF-8 -t GB18030 "$REPO_ROOT/tools/mudlib/ai-travel.c" \
  > "$WORK_ROOT/payload/adm/daemons/ai_travel.c"
iconv -f UTF-8 -t GB18030 "$REPO_ROOT/tools/mudlib/help-wizard-commands" \
  > "$WORK_ROOT/payload/help/system/wizard_commands"
iconv -f UTF-8 -t GB18030 "$REPO_ROOT/tools/mudlib/character-skilld.c" \
  > "$WORK_ROOT/payload/adm/daemons/character_skilld.c"
iconv -f UTF-8 -t GB18030 "$REPO_ROOT/tools/mudlib/attribute-skilld.c" \
  > "$WORK_ROOT/payload/adm/daemons/attribute_skilld.c"
iconv -f UTF-8 -t GB18030 "$REPO_ROOT/tools/mudlib/zuoyou-hubo.c" \
  > "$WORK_ROOT/payload/kungfu/skill/zuoyou-hubo.c"
for shop_item in xidian xingge; do
  iconv -f UTF-8 -t GB18030 "$REPO_ROOT/tools/mudlib/shop/$shop_item.c" \
    > "$WORK_ROOT/payload/clone/vip/$shop_item.c"
done
chmod 0644 \
  "$WORK_ROOT/payload/adm/daemons/ai_playerd.c" \
  "$WORK_ROOT/payload/adm/daemons/ai_travel.c" \
  "$WORK_ROOT/payload/cmds/adm/aiplayer.c" \
  "$WORK_ROOT/payload/help/system/wizard_commands" \
  "$WORK_ROOT/payload/adm/daemons/character_skilld.c" \
  "$WORK_ROOT/payload/adm/daemons/attribute_skilld.c" \
  "$WORK_ROOT/payload/kungfu/skill/zuoyou-hubo.c" \
  "$WORK_ROOT/payload/clone/vip/xidian.c" \
  "$WORK_ROOT/payload/clone/vip/xingge.c"

while IFS= read -r -d '' plot_source; do
  plot_relative="${plot_source#$PLOT_SOURCE_ROOT/}"
  plot_target="$WORK_ROOT/payload/$plot_relative"
  mkdir -p "$(dirname "$plot_target")"
  iconv -f UTF-8 -t GB18030 "$plot_source" > "$plot_target"
  chmod 0644 "$plot_target"
done < <(find "$PLOT_SOURCE_ROOT" -type f -name '*.c' -print0 | LC_ALL=C sort -z)

perl -0pi -e 's{(#define STORY_DIR\s+"/adm/daemons/story/"\n)}{$1#define PLOT_DIR        "/adm/daemons/plot/"\n}' \
  "$WORK_ROOT/payload/include/globals.h"
perl -0pi -e 's{(#define STORY_D\s+"/adm/daemons/storyd"\n)}{$1#define PLOT_D          "/adm/daemons/plotd"\n}' \
  "$WORK_ROOT/payload/include/globals.h"
perl -0pi -e 's{(#define PLOT_D\s+"/adm/daemons/plotd"\n)}{$1#define PLOT_COMBAT_D   "/adm/daemons/plotcombat"\n}' \
  "$WORK_ROOT/payload/include/globals.h"

for plot_menu in cmds/usr/mycmds.c cmds/usr/mycmds1.c; do
  plot_menu_utf8="$WORK_ROOT/$(basename "$plot_menu").utf8"
  iconv -f GB18030 -t UTF-8 "$WORK_ROOT/payload/$plot_menu" > "$plot_menu_utf8"
  perl -0pi -e 's{ZJSEP"b4:暂未"ZJBR"设定:look"}{ZJSEP"b4:剧情"ZJBR"日志:plot"}' \
    "$plot_menu_utf8"
  iconv -f UTF-8 -t GB18030 "$plot_menu_utf8" > "$WORK_ROOT/payload/$plot_menu"
done

post_officer_utf8="$WORK_ROOT/post_officer.utf8"
iconv -f GB18030 -t UTF-8 "$WORK_ROOT/payload/d/city/npc/post_officer.c" > "$post_officer_utf8"
perl -0pi -e 's/string give_quest\(\);/string give_quest();\nstring plot_letter();\nstring plot_mark();/' "$post_officer_utf8"
perl -0pi -e 's/"收信" : \(: receive_mail :\),/"收信" : (: receive_mail :),\n\t\t"乡书" : (: plot_letter :),\n\t\t"旧印" : (: plot_mark :),/' "$post_officer_utf8"
perl -0pi -e 's/void init\(\)\n\{/string plot_letter()\n{\n\treturn "\/adm\/daemons\/plot\/origin_letter"->npc_notice(this_player());\n}\nvoid init()\n\{/' "$post_officer_utf8"
perl -0pi -e 's/string plot_letter\(\)\n\{\n\treturn "\/adm\/daemons\/plot\/origin_letter"->npc_notice\(this_player\(\)\);\n\}/$&\nstring plot_mark()\n{\n\treturn "\/adm\/daemons\/plot\/returning_mark"->npc_notice(this_player());\n}/' "$post_officer_utf8"
iconv -f UTF-8 -t GB18030 "$post_officer_utf8" > "$WORK_ROOT/payload/d/city/npc/post_officer.c"

wumiao_utf8="$WORK_ROOT/wumiao.utf8"
iconv -f GB18030 -t UTF-8 "$WORK_ROOT/payload/d/city/wumiao.c" > "$wumiao_utf8"
  perl -0pi -e 's/"每日签到"\s*:\s*"day_sign",/"每日签到" : "day_sign",\n\t\t"乡书旧闻" : "plot act origin_letter_01 notice",\n\t\t"旧印新痕" : "plot act returning_mark_02 branch_hook",\n\t\t"故园来客" : "plot act visitors_from_home_04 begin",\n\t\t"百川归处" : "plot act where_rivers_end_05 begin",/' "$wumiao_utf8"
iconv -f UTF-8 -t GB18030 "$wumiao_utf8" > "$WORK_ROOT/payload/d/city/wumiao.c"

for entry_room in d/city/kedian.c d/city/wumiao.c d/city/postofficer.c; do
  entry_utf8="$WORK_ROOT/$(basename "$entry_room").entry.utf8"
  iconv -f GB18030 -t UTF-8 "$WORK_ROOT/payload/$entry_room" > "$entry_utf8"
  perl -0pi -e 's/void init\(\)\n\{/void init()\n{\n\tstring plot_notice;\n\tplot_notice = "\/adm\/daemons\/plot\/origin_letter"->notify_entry(this_player());\n\tif (plot_notice != "") tell_object(this_player(), plot_notice + "\\n");/' "$entry_utf8"
  iconv -f UTF-8 -t GB18030 "$entry_utf8" > "$WORK_ROOT/payload/$entry_room"
done

yamen_utf8="$WORK_ROOT/ymzhengting.utf8"
iconv -f GB18030 -t UTF-8 "$WORK_ROOT/payload/d/city/ymzhengting.c" > "$yamen_utf8"
perl -0pi -e 's|(set\("objects", \(\[\n)|$1\t"\/d\/plot\/origin_letter\/npc\/tan_youji" : 1,\n|' "$yamen_utf8"
iconv -f UTF-8 -t GB18030 "$yamen_utf8" > "$WORK_ROOT/payload/d/city/ymzhengting.c"

help_topics_utf8="$WORK_ROOT/help-topics.utf8"
iconv -f GB18030 -t UTF-8 "$WORK_ROOT/payload/help/tit/topics" \
  > "$help_topics_utf8"
perl -0pi -e 's|(游戏简介:help system/intro\$zj\#\n)|$1巫师命令:help system/wizard_commands\$zj\#\n|' \
  "$help_topics_utf8"
iconv -f UTF-8 -t GB18030 "$help_topics_utf8" \
  > "$WORK_ROOT/payload/help/tit/topics"

if [[ "$(LC_ALL=C rg -F -c 'help system/wizard_commands' \
        "$WORK_ROOT/payload/help/tit/topics")" != "1" ]] ||
   [[ "$(LC_ALL=C rg -F -c 'call me->set("character","' \
        "$WORK_ROOT/payload/help/system/wizard_commands")" != "1" ]] ||
   [[ "$(LC_ALL=C rg -F -c 'aiplayer validate' \
        "$WORK_ROOT/payload/help/system/wizard_commands")" != "1" ]]; then
  echo "Wizard help documentation validation failed." >&2
  exit 1
fi
mkdir -p "$WORK_ROOT/payload/d/standalone"
iconv -f UTF-8 -t GB18030 "$REPO_ROOT/tools/mudlib/ai-test-room.c" \
  > "$WORK_ROOT/payload/d/standalone/ai_test.c"
iconv -f UTF-8 -t GB18030 "$REPO_ROOT/tools/mudlib/ai-test-retreat-room.c" \
  > "$WORK_ROOT/payload/d/standalone/ai_test_retreat.c"
iconv -f UTF-8 -t GB18030 "$REPO_ROOT/tools/mudlib/ai-test-blocked-room.c" \
  > "$WORK_ROOT/payload/d/standalone/ai_test_blocked.c"
iconv -f UTF-8 -t GB18030 "$REPO_ROOT/tools/mudlib/ai-test-safe-room.c" \
  > "$WORK_ROOT/payload/d/standalone/ai_test_safe.c"
iconv -f UTF-8 -t GB18030 "$REPO_ROOT/tools/mudlib/ai-test-dummy.c" \
  > "$WORK_ROOT/payload/clone/npc/ai_test_dummy.c"
chmod 0644 "$WORK_ROOT/payload/d/standalone/ai_test.c" \
  "$WORK_ROOT/payload/d/standalone/ai_test_retreat.c" \
  "$WORK_ROOT/payload/d/standalone/ai_test_blocked.c" \
  "$WORK_ROOT/payload/d/standalone/ai_test_safe.c" \
  "$WORK_ROOT/payload/clone/npc/ai_test_dummy.c"
printf '\n%s\n' '/adm/daemons/ai_playerd' >> "$WORK_ROOT/payload/adm/etc/preload"
printf '%s\n' '/adm/daemons/ai_travel' >> "$WORK_ROOT/payload/adm/etc/preload"
printf '%s\n' '/adm/daemons/plotcombat' >> "$WORK_ROOT/payload/adm/etc/preload"
printf '%s\n' '/adm/daemons/plotd' >> "$WORK_ROOT/payload/adm/etc/preload"
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
      "$(LC_ALL=C rg -F -c 'int inject_ai_death_state()' \
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
      "$(LC_ALL=C rg -c '^/adm/daemons/ai_travel$' \
        "$WORK_ROOT/payload/adm/etc/preload")" != "1" ||
      "$(LC_ALL=C rg -c '^/adm/daemons/securityd$' \
        "$WORK_ROOT/payload/adm/etc/preload")" != "1" ||
      "$(LC_ALL=C sed -n '1p' "$WORK_ROOT/payload/adm/etc/preload")" != \
        "/adm/daemons/securityd" ||
      "$(LC_ALL=C rg -F -c 'Persistent autonomous player actors' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'Frozen bounded region graph and approved real-world schedule for AI v2.4' \
        "$WORK_ROOT/payload/adm/daemons/ai_travel.c")" != "1" ||
      "$(LC_ALL=C rg -F -c '#define AI_TRAVEL_SCHEMA_VERSION 2' \
        "$WORK_ROOT/payload/adm/daemons/ai_travel.c")" != "1" ||
      "$(LC_ALL=C rg -F -c '#define AI_TRAVEL_CAPABILITY_VERSION "v2.4"' \
        "$WORK_ROOT/payload/adm/daemons/ai_travel.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'mapping plan_route' \
        "$WORK_ROOT/payload/adm/daemons/ai_travel.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'mapping selftest' \
        "$WORK_ROOT/payload/adm/daemons/ai_travel.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'mapping run_registered_travel' \
        "$WORK_ROOT/payload/adm/daemons/ai_travel.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'mapping run_schedule_period' \
        "$WORK_ROOT/payload/adm/daemons/ai_travel.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'int prepare_auto_schedule_test' \
        "$WORK_ROOT/payload/adm/daemons/ai_travel.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'int should_hold_for_schedule' \
        "$WORK_ROOT/payload/adm/daemons/ai_travel.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'mapping recover_travel' \
        "$WORK_ROOT/payload/adm/daemons/ai_travel.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'int prepare_recovery_test' \
        "$WORK_ROOT/payload/adm/daemons/ai_travel.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'private mapping migrate_travel_state' \
        "$WORK_ROOT/payload/adm/daemons/ai_travel.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'private mapping resume_travel' \
        "$WORK_ROOT/payload/adm/daemons/ai_travel.c")" != "2" ||
      "$(LC_ALL=C rg -F -c 'protected void heart_beat' \
        "$WORK_ROOT/payload/adm/daemons/ai_travel.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'AI_TRAVEL_ACTOR "ai_qingfeng"' \
        "$WORK_ROOT/payload/adm/daemons/ai_travel.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'private string find_route_step' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "2" ||
      "$(LC_ALL=C rg -F -c 'mapping query_player_status' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'object query_ai_player' \
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
      "$(LC_ALL=C rg -F -c 'private void manage_combat' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "2" ||
      "$(LC_ALL=C rg -F -c 'private int attempt_combat_retreat' \
        "$WORK_ROOT/payload/adm/daemons/ai_playerd.c")" != "2" ||
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

for plot_file in \
  adm/daemons/plotcombat.c \
  adm/daemons/plotd.c \
  adm/daemons/plot/origin_letter.c \
  adm/daemons/plot/returning_mark.c \
  adm/daemons/plot/silent_crossing.c \
  adm/daemons/plot/visitors_from_home.c \
  adm/daemons/plot/where_rivers_end.c \
  cmds/usr/plot.c \
  cmds/adm/plotadmin.c \
  d/plot/origin_letter/warehouse.c \
  d/plot/origin_letter/hall.c \
  d/plot/origin_letter/grain_house.c \
  d/plot/returning_mark/old_storehouse.c \
  d/plot/returning_mark/mark_chamber.c \
  d/plot/silent_crossing/boat_deck.c \
  d/plot/silent_crossing/foredeck.c \
  d/plot/silent_crossing/stern.c \
  d/plot/silent_crossing/narrow_passage.c \
  d/plot/silent_crossing/cargo_hold.c \
  d/plot/silent_crossing/bilge.c \
  d/plot/visitors_from_home/relay_court.c \
  d/plot/visitors_from_home/post_road.c \
  d/plot/where_rivers_end/instance_room.c \
  d/plot/where_rivers_end/salt_gate.c \
  d/plot/where_rivers_end/drain_tunnel.c \
  d/plot/where_rivers_end/water_gate.c \
  d/plot/where_rivers_end/inspection_yard.c \
  d/plot/where_rivers_end/outer_gallery.c \
  d/plot/where_rivers_end/assembly_floor.c \
  d/plot/where_rivers_end/dark_cell.c \
  d/plot/where_rivers_end/guard_walk.c \
  d/plot/where_rivers_end/burning_archive.c \
  d/plot/where_rivers_end/index_room.c \
  d/plot/where_rivers_end/seal_chamber.c \
  d/plot/origin_letter/npc/xu_sanniang.c \
  d/plot/origin_letter/npc/zhou_shouyi.c \
  d/plot/origin_letter/npc/tan_youji.c \
  clone/plot/origin_letter/pei_jiu.c \
  clone/plot/origin_letter/old_post_token.c \
  clone/plot/returning_mark/meng_si.c \
  clone/plot/returning_mark/qing_peng_ke.c \
  clone/plot/returning_mark/wave_mark_rubbing.c \
  clone/plot/silent_crossing/liu_dashao.c \
  clone/plot/silent_crossing/a_he.c \
  clone/plot/silent_crossing/luo_qiniang.c \
  clone/plot/silent_crossing/relabelled_stub.c \
  clone/plot/visitors_from_home/stone_seven.c \
  clone/plot/visitors_from_home/hua_yaozi.c \
  clone/plot/visitors_from_home/salt_invitation.c \
  clone/plot/where_rivers_end/real_lu_xiaoshuan.c \
  clone/plot/where_rivers_end/luo_qiniang.c \
  clone/plot/where_rivers_end/wen_shouzhuo.c \
  clone/plot/where_rivers_end/qimei_staff.c \
  clone/plot/where_rivers_end/hometown_memorial.c; do
  if [[ ! -f "$WORK_ROOT/payload/$plot_file" ]]; then
    echo "Missing plot platform file: $plot_file" >&2
    exit 1
  fi
done

if [[ "$(LC_ALL=C rg -c '^/adm/daemons/plotd$' \
        "$WORK_ROOT/payload/adm/etc/preload")" != "1" ]]; then
  echo "Plot daemon preload validation failed." >&2
  exit 1
fi
if [[ "$(LC_ALL=C rg -c '^/adm/daemons/plotcombat$' \
        "$WORK_ROOT/payload/adm/etc/preload")" != "1" ]]; then
  echo "Plot combat daemon preload validation failed." >&2
  exit 1
fi
if [[ "$(LC_ALL=C rg -c '^#define PLOT_DIR[[:space:]]' \
        "$WORK_ROOT/payload/include/globals.h")" != "1" ||
      "$(LC_ALL=C rg -c '^#define PLOT_D[[:space:]]' \
        "$WORK_ROOT/payload/include/globals.h")" != "1" ||
      "$(LC_ALL=C rg -c '^#define PLOT_COMBAT_D[[:space:]]' \
        "$WORK_ROOT/payload/include/globals.h")" != "1" ]]; then
  echo "Plot global macro validation failed." >&2
  exit 1
fi
if [[ "$(LC_ALL=C rg -F -c 'PLOT_SELFTEST ok=%d schema=%d arc=%s errors=%O' \
        "$WORK_ROOT/payload/cmds/adm/plotadmin.c")" != "1" ]]; then
  echo "Plot administrator command validation failed." >&2
  exit 1
fi
if [[ "$(LC_ALL=C rg -F -c 'PLOT_COMBAT_SELFTEST ok=%d profiles=%d errors=%O' \
        "$WORK_ROOT/payload/cmds/adm/plotadmin.c")" != "1" ]]; then
  echo "Plot combat administrator validation failed." >&2
  exit 1
fi
if [[ "$(iconv -f GB18030 -t UTF-8 "$WORK_ROOT/payload/d/city/npc/post_officer.c" | \
        LC_ALL=C rg -F -c '"乡书" : (: plot_letter :)')" != "1" ||
      "$(iconv -f GB18030 -t UTF-8 "$WORK_ROOT/payload/d/city/npc/post_officer.c" | \
        LC_ALL=C rg -F -c '"旧印" : (: plot_mark :)')" != "1" ||
      "$(iconv -f GB18030 -t UTF-8 "$WORK_ROOT/payload/d/city/wumiao.c" | \
        LC_ALL=C rg -F -c '"乡书旧闻" : "plot act origin_letter_01 notice"')" != "1" ||
      "$(iconv -f GB18030 -t UTF-8 "$WORK_ROOT/payload/d/city/wumiao.c" | \
        LC_ALL=C rg -F -c '"旧印新痕" : "plot act returning_mark_02 branch_hook"')" != "1" ||
      "$(iconv -f GB18030 -t UTF-8 "$WORK_ROOT/payload/d/city/wumiao.c" | \
        LC_ALL=C rg -F -c '"故园来客" : "plot act visitors_from_home_04 begin"')" != "1" ||
      "$(iconv -f GB18030 -t UTF-8 "$WORK_ROOT/payload/d/city/wumiao.c" | \
        LC_ALL=C rg -F -c '"百川归处" : "plot act where_rivers_end_05 begin"')" != "1" ||
      "$(iconv -f GB18030 -t UTF-8 "$WORK_ROOT/payload/d/city/ymzhengting.c" | \
        LC_ALL=C rg -F -c '"/d/plot/origin_letter/npc/tan_youji" : 1')" != "1" ]]; then
  echo "Plot chapter entry validation failed." >&2
  exit 1
fi
if [[ "$(LC_ALL=C rg -F -c 'hometown_letters_01' \
        "$WORK_ROOT/payload/adm/daemons/plotd.c")" != "1" ]] ||
   LC_ALL=C rg -F -q 'hometown_letters_arc_01' \
     "$WORK_ROOT/payload/adm/daemons/plotd.c"; then
  echo "Plot stable identifier validation failed." >&2
  exit 1
fi

for plot_menu in cmds/usr/mycmds.c cmds/usr/mycmds1.c; do
  if [[ "$(iconv -f GB18030 -t UTF-8 "$WORK_ROOT/payload/$plot_menu" | \
          LC_ALL=C rg -F -c 'b4:剧情"ZJBR"日志:plot')" != "1" ]]; then
    echo "Plot menu validation failed: $plot_menu" >&2
    exit 1
  fi
done

if [[ ! -f "$WORK_ROOT/payload/d/standalone/ai_test.c" ||
      ! -f "$WORK_ROOT/payload/d/standalone/ai_test_retreat.c" ||
      ! -f "$WORK_ROOT/payload/d/standalone/ai_test_blocked.c" ||
      ! -f "$WORK_ROOT/payload/d/standalone/ai_test_safe.c" ||
      ! -f "$WORK_ROOT/payload/clone/npc/ai_test_dummy.c" ||
      "$(LC_ALL=C rg -F -c 'set("ai_test_room", 1);' \
        "$WORK_ROOT/payload/d/standalone/ai_test.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'set("ai_test_dummy", 1);' \
        "$WORK_ROOT/payload/clone/npc/ai_test_dummy.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'configure_for_ai' \
        "$WORK_ROOT/payload/clone/npc/ai_test_dummy.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'ai_test_safe' \
        "$WORK_ROOT/payload/d/standalone/ai_test_retreat.c")" != "1" ||
      "$(LC_ALL=C rg -F -c 'ai_test_block_exit' \
        "$WORK_ROOT/payload/d/standalone/ai_test_blocked.c")" != "1" ]]; then
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

for shop_item in xidian xingge; do
  shop_item_file="$WORK_ROOT/payload/clone/vip/$shop_item.c"
  if [[ ! -f "$shop_item_file" ]] ||
     [[ "$(LC_ALL=C rg -F -c 'set("yuanbao", 1);' "$shop_item_file")" != "1" ]] ||
     [[ "$(LC_ALL=C rg -F -c 'set("only_do_effect", 1);' "$shop_item_file")" != "1" ]]; then
    echo "Reset item shop contract validation failed: $shop_item" >&2
    exit 1
  fi
done
if [[ "$(LC_ALL=C rg -F -c 'add_action("do_reset", "xidian_dan");' \
      "$WORK_ROOT/payload/clone/vip/xidian.c")" != "1" ]] ||
   [[ "$(LC_ALL=C rg -F -c 'me->set("str", temp_str);' \
      "$WORK_ROOT/payload/clone/vip/xidian.c")" != "1" ]] ||
   [[ "$(LC_ALL=C rg -F -c 'add_action("do_reset", "xingge_dan");' \
      "$WORK_ROOT/payload/clone/vip/xingge.c")" != "1" ]] ||
   [[ "$(LC_ALL=C rg -F -c 'me->set("character", character);' \
      "$WORK_ROOT/payload/clone/vip/xingge.c")" != "1" ]] ||
   LC_ALL=C rg -q 'reset_(att|cha)_times' \
      "$WORK_ROOT/payload/clone/vip/xidian.c" \
      "$WORK_ROOT/payload/clone/vip/xingge.c"; then
  echo "Reset item direct-operation validation failed." >&2
  exit 1
fi

if LC_ALL=C rg -q 'temp_(str|int|con|dex) > 39' \
     "$WORK_ROOT/payload/clone/vip/xidian.c" ||
   iconv -f GB18030 -t UTF-8 "$WORK_ROOT/payload/adm/npc/wizard.c" |
     LC_ALL=C rg -q 'tmp(str|int|con|dex)>39|大于39'; then
  echo "Reset attribute upper-cap removal validation failed." >&2
  exit 1
fi

if iconv -f GB18030 -t UTF-8 "$WORK_ROOT/payload/kungfu/skill/zuoyou-hubo.c" |
     LC_ALL=C rg -q 'query\("int"\)|query_int\(' ||
   LC_ALL=C rg -F -q '"zuoyou-hubo"' \
     "$WORK_ROOT/payload/adm/daemons/attribute_skilld.c"; then
  echo "Zuoyou-hubo intelligence-limit removal validation failed." >&2
  exit 1
fi

if iconv -f GB18030 -t UTF-8 "$WORK_ROOT/payload/cmds/skill/perform.c" |
     LC_ALL=C rg -q 'query_skill\("count", 1\)|杂学太多'; then
  echo "Zuoyou-hubo count-limit removal validation failed." >&2
  exit 1
fi

if [[ "$(LC_ALL=C rg -F -c '"/adm/daemons/character_skilld"->valid_for_character' \
      "$WORK_ROOT/payload/adm/npc/wizard.c")" != "1" ]] ||
   [[ "$(LC_ALL=C rg -F -c '"/adm/daemons/attribute_skilld"->valid_for_attributes' \
      "$WORK_ROOT/payload/adm/npc/wizard.c")" != "1" ]] ||
   [[ "$(LC_ALL=C rg -F -c '"/adm/daemons/character_skilld"->valid_for_character' \
      "$WORK_ROOT/payload/clone/vip/xingge.c")" != "1" ]] ||
   LC_ALL=C rg -F -q 'SKILL_D(skills[i])->valid_learn(me)' \
      "$WORK_ROOT/payload/adm/npc/wizard.c" \
      "$WORK_ROOT/payload/clone/vip/xingge.c" \
      "$WORK_ROOT/payload/clone/vip/xidian.c"; then
  echo "Character skill retention integration validation failed." >&2
  exit 1
fi
if [[ "$(LC_ALL=C rg -F -c '"/adm/daemons/attribute_skilld"->valid_for_attributes' \
      "$WORK_ROOT/payload/clone/vip/xidian.c")" != "1" ]] ||
   [[ "$(LC_ALL=C rg -F -c 'int valid_for_attributes(string skill, object me)' \
      "$WORK_ROOT/payload/adm/daemons/attribute_skilld.c")" != "1" ]]; then
  echo "Attribute skill retention integration validation failed." >&2
  exit 1
fi
if [[ "$(iconv -f GB18030 -t UTF-8 "$WORK_ROOT/payload/adm/npc/wizard.c" | \
      LC_ALL=C rg -F -c '光明磊落，宗师心法-九阴神功')" != "1" ]] ||
   [[ "$(iconv -f GB18030 -t UTF-8 "$WORK_ROOT/payload/adm/npc/wizard.c" | \
      LC_ALL=C rg -F -c '心狠手辣，宗师心法-南海玄功')" != "1" ]]; then
  echo "Wizard character menu validation failed." >&2
  exit 1
fi
for character_skill in \
  jiuyin-shengong bluesea-force never-defeated kuihua-mogong lonely-sword \
  huagong-dafa poison xixing-dafa; do
  if [[ "$(LC_ALL=C rg -F -c "\"$character_skill\"" \
        "$WORK_ROOT/payload/adm/daemons/character_skilld.c")" != "1" ]]; then
    echo "Missing character skill retention rule: $character_skill" >&2
    exit 1
  fi
done

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
