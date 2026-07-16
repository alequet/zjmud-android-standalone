#!/usr/bin/env bash
set -euo pipefail

readonly PACKAGE="com.zjmud.android"
readonly ACTIVITY="$PACKAGE/.MainActivity"
readonly DEFAULT_APK="app/build/outputs/apk/debug/app-debug.apk"
readonly EXPECTED_AI_IDS=(
  ai_qingfeng
  ai_songlan
  ai_wantang
  ai_yanqiu
  ai_zhiyuan
)

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
apk="$repo_root/$DEFAULT_APK"
serial=""
wait_seconds=60
build=1
confirmed=0

usage() {
  cat <<'EOF'
Usage: tools/test_ai_player_startup.sh --yes [options]

Destructively clears com.zjmud.android data, verifies the five AI login and
player saves on first boot, restarts the app, then verifies recovery.

Options:
  --yes             Confirm destructive application-data clearing.
  --no-build        Reuse an existing debug APK.
  --apk PATH        Install a specific APK.
  --serial SERIAL   Select an adb device.
  --wait SECONDS    Startup timeout per phase (default: 60).
EOF
}

while (($#)); do
  case "$1" in
    --yes) confirmed=1 ;;
    --no-build) build=0 ;;
    --apk)
      shift
      apk="${1:?--apk requires a path}"
      [[ "$apk" = /* ]] || apk="$repo_root/$apk"
      ;;
    --serial)
      shift
      serial="${1:?--serial requires a value}"
      ;;
    --wait)
      shift
      wait_seconds="${1:?--wait requires seconds}"
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
  shift
done

if ((confirmed != 1)); then
  echo "Refusing to clear $PACKAGE data without --yes." >&2
  exit 2
fi
if ! [[ "$wait_seconds" =~ ^[1-9][0-9]*$ ]]; then
  echo "--wait must be a positive integer." >&2
  exit 2
fi

adb_cmd=(adb)
if [[ -n "$serial" ]]; then
  adb_cmd+=(-s "$serial")
fi

run_adb() {
  "${adb_cmd[@]}" "$@"
}

run_as() {
  run_adb exec-out run-as "$PACKAGE" "$@"
}

count_saves() {
  local kind="$1"
  run_as find "files/runtime/current/data/$kind/a" -maxdepth 1 -type f -name 'ai_*.o' \
    | wc -l | tr -d '[:space:]'
}

list_saves() {
  local kind="$1"
  run_as find "files/runtime/current/data/$kind/a" -maxdepth 1 -type f -name 'ai_*.o' -printf '%f\n' \
    | sort
}

wait_for_saves() {
  local deadline=$((SECONDS + wait_seconds))
  local login_count user_count

  while ((SECONDS < deadline)); do
    login_count="$(count_saves login)"
    user_count="$(count_saves user)"
    if [[ "$login_count" == "5" && "$user_count" == "5" ]]; then
      return 0
    fi
    sleep 2
  done
  echo "Timed out waiting for AI saves: login=${login_count:-0} user=${user_count:-0}" >&2
  return 1
}

verify_names() {
  local kind="$1"
  local expected actual

  expected="$(printf '%s.o\n' "${EXPECTED_AI_IDS[@]}" | sort)"
  actual="$(list_saves "$kind" | tr -d '\r')"
  if [[ "$actual" != "$expected" ]]; then
    echo "Unexpected $kind AI saves:" >&2
    printf '%s\n' "$actual" >&2
    return 1
  fi
}

verify_logs() {
  local log_file matches

  log_file="$(mktemp "${TMPDIR:-/tmp}/zjmud-ai-startup.XXXXXX.log")"
  run_as cat files/runtime/current/android-driver.log > "$log_file"
  run_as cat files/runtime/current/log/debug.log >> "$log_file" 2>/dev/null || true
  matches="$(LC_ALL=C rg -a -n \
    'Denied write permission in save_object|Can.t load objects when no effective user|Unable to (save initial login|prepare login uid|export AI account uid|activate AI account uid|initialize runtime uid)|AI_(INIT|SECURITY)_DEBUG|ai-save-debug' \
    "$log_file" || true)"
  rm -f "$log_file"
  if [[ -n "$matches" ]]; then
    echo "AI startup errors found in runtime logs:" >&2
    printf '%s\n' "$matches" >&2
    return 1
  fi
}

cd "$repo_root"
if ((build == 1)); then
  ./gradlew assembleDebug
fi
if [[ ! -f "$apk" ]]; then
  echo "APK not found: $apk" >&2
  exit 1
fi

run_adb get-state >/dev/null
run_adb install -r "$apk"
run_adb shell pm clear "$PACKAGE" >/dev/null
run_adb shell pm grant "$PACKAGE" android.permission.POST_NOTIFICATIONS 2>/dev/null || true
run_adb shell am start -n "$ACTIVITY" >/dev/null

wait_for_saves
verify_names login
verify_names user
verify_logs
echo "First boot: 5 login saves and 5 player saves verified."

run_adb shell am force-stop "$PACKAGE"
run_adb shell am start -n "$ACTIVITY" >/dev/null
wait_for_saves
verify_names login
verify_names user
verify_logs
echo "Restart recovery: 5 login saves and 5 player saves verified."

echo "AI startup regression passed."
