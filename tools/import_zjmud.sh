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
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/quest_fly.patch"
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/hide_room_paths.patch"
patch -s -p1 -d "$WORK_ROOT/payload" < "$REPO_ROOT/tools/mudlib/static_admins.patch"

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
