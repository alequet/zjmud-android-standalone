#!/usr/bin/env bash
set -euo pipefail

readonly EXPECTED_SHA256="2eee2ab12f81a3a7a7f5824a552f85f9d287c6d3dbfb03c4b1e2c0bfc2578ba0"
readonly SOURCE_ZIP="${1:-$HOME/zjmud-main.zip}"
readonly REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ASSET_ROOT="$REPO_ROOT/app/src/main/assets"
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

readonly SOURCE_ROOT="$WORK_ROOT/extracted/zjmud-main"
for required in config.ini adm clone cmds d data include inherit kungfu web/www/index.html; do
  if [[ ! -e "$SOURCE_ROOT/$required" ]]; then
    echo "Missing required source path: $required" >&2
    exit 1
  fi
done

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

perl -ni -e 'print unless m{^/adm/daemons/(messaged|payd)\s*$}' "$WORK_ROOT/payload/adm/etc/preload"

patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/remove_anticheat.patch"
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/singleplayer_boosts.patch"
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/shaolin_unarmed_rewards.patch"
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/quest_fly.patch"
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/hide_room_paths.patch"
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/static_admins.patch"
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/full_character_save.patch"
install -m 0644 "$REPO_ROOT/tools/mudlib/fullsave.c" "$WORK_ROOT/payload/feature/fullsave.c"

perl -pi -e 's/cost = me->query_skill\(skl_id, 1\) \/ 2 \+ 100;/cost = (me->query_skill(skl_id, 1) \/ 2 + 100) * 20;/' \
  "$WORK_ROOT/payload/cmds/skill/derive.c"

if [[ "$(LC_ALL=C rg -c 'inherit "/feature/fullsave";' "$WORK_ROOT/payload/clone/user/user.c")" != "1" ||
      "$(LC_ALL=C rg -c 'save_full_character_state\(\);' "$WORK_ROOT/payload/clone/user/user.c")" != "1" ||
      "$(LC_ALL=C rg -c 'prepare_full_character_restore\(\);' "$WORK_ROOT/payload/clone/user/user.c")" != "1" ||
      "$(LC_ALL=C rg -c 'query_full_save_room\(\)' "$WORK_ROOT/payload/adm/daemons/logind.c")" != "1" ]]; then
  echo "Full character persistence patch validation failed." >&2
  exit 1
fi

if [[ "$(LC_ALL=C rg -F -c 'cost = (me->query_skill(skl_id, 1) / 2 + 100) * 20;' \
        "$WORK_ROOT/payload/cmds/skill/derive.c")" != "1" ]]; then
  echo "Jiqu absorption multiplier patch validation failed." >&2
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
perl -0pi -e 's/\n\tdocument\.oncontextmenu = function\(\) \{\n\t\treturn false;\n\t\}//' \
  "$ASSET_ROOT/web/main.js" "$ASSET_ROOT/web/js/main.js"

if LC_ALL=C rg -q 'document\.oncontextmenu' \
     "$ASSET_ROOT/web/main.js" "$ASSET_ROOT/web/js/main.js"; then
  echo "Web text-selection context menu is still disabled." >&2
  exit 1
fi

install -m 0644 "$WORK_ROOT/zjmud-runtime.zip" "$ASSET_ROOT/runtime/zjmud-runtime.zip"

runtime_sha256="$(shasum -a 256 "$ASSET_ROOT/runtime/zjmud-runtime.zip" | awk '{print $1}')"
runtime_bytes="$(stat -f '%z' "$ASSET_ROOT/runtime/zjmud-runtime.zip")"
printf 'source_sha256=%s\nruntime_sha256=%s\nruntime_bytes=%s\n' \
  "$actual_sha256" "$runtime_sha256" "$runtime_bytes" \
  > "$ASSET_ROOT/runtime/manifest.properties"
cat <<EOF
Imported zjmud assets
source_sha256=$actual_sha256
runtime_sha256=$runtime_sha256
runtime_bytes=$runtime_bytes
EOF
