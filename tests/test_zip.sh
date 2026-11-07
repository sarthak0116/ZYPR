#!/usr/bin/env bash
# Integration tests: extract real ZIP archives with the built `unzip` binary
# and compare against the original payloads / system unzip output.
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
ZYPR_BIN="$REPO_ROOT/unzip"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

failures=0

check() { # check <label> <condition...>
    local label="$1"; shift
    if "$@"; then
        echo "PASS: $label"
    else
        echo "FAIL: $label"
        failures=$((failures + 1))
    fi
}

# --- build the payload tree -------------------------------------------------
PAYLOAD="$WORK/payload"
mkdir -p "$PAYLOAD/dir1"
: > "$PAYLOAD/empty.txt"
printf 'hello zypr\n' > "$PAYLOAD/hello.txt"
python3 -c "import random,sys; r=random.Random(8); sys.stdout.buffer.write(bytes(r.getrandbits(8) for _ in range(65536)))" > "$PAYLOAD/random.bin"
python3 -c "import sys; sys.stdout.write('repeat me. ' * 200000)" > "$PAYLOAD/big.txt"
printf 'nested file contents\n' > "$PAYLOAD/dir1/nested.txt"

cd "$WORK"

# --- case 1: system zip, DEFLATE (default, -9) -------------------------------
(cd "$PAYLOAD" && zip -q -r -9 "$WORK/deflate.zip" .)
mkdir out-deflate && (cd out-deflate && "$ZYPR_BIN" "$WORK/deflate.zip" > /dev/null 2>&1)
check "extract system zip (deflate) matches payload" diff -r "$PAYLOAD" out-deflate

# --- case 2: system zip, stored (-0) -----------------------------------------
(cd "$PAYLOAD" && zip -q -r -0 "$WORK/stored.zip" .)
mkdir out-stored && (cd out-stored && "$ZYPR_BIN" "$WORK/stored.zip" > /dev/null 2>&1)
check "extract system zip (stored) matches payload" diff -r "$PAYLOAD" out-stored

# --- case 3: zip created by Python's zipfile (different writer) --------------
python3 - "$WORK/py.zip" "$PAYLOAD" <<'PYGEN'
import os, sys, zipfile
target, payload = sys.argv[1], sys.argv[2]
with zipfile.ZipFile(target, "w", zipfile.ZIP_DEFLATED) as z:
    for root, _dirs, files in os.walk(payload):
        for name in files:
            full = os.path.join(root, name)
            z.write(full, os.path.relpath(full, payload))
PYGEN
mkdir out-py && (cd out-py && "$ZYPR_BIN" "$WORK/py.zip" > /dev/null 2>&1)
check "extract python zipfile archive matches payload" diff -r "$PAYLOAD" out-py

# --- case 4: bundled demo zip matches system unzip ---------------------------
mkdir out-demo ref-demo
(cd out-demo && "$ZYPR_BIN" "$REPO_ROOT/demo/wq.zip" > /dev/null 2>&1)
(cd ref-demo && unzip -q -o "$REPO_ROOT/demo/wq.zip" && rm -rf __MACOSX)
check "demo/wq.zip output matches system unzip" diff -r ref-demo out-demo

# --- case 5: non-ZIP input fails cleanly -------------------------------------
printf 'this is not a zip file at all\n' > "$WORK/notzip.bin"
(cd "$WORK" && "$ZYPR_BIN" "$WORK/notzip.bin" > /dev/null 2>&1)
rc=$?
if [ "$rc" -ne 0 ] && [ "$rc" -lt 126 ]; then
    echo "PASS: non-ZIP input rejected (exit $rc)"
else
    echo "FAIL: non-ZIP input exit code $rc"
    failures=$((failures + 1))
fi

# --- case 6: malicious paths are skipped (zip-slip guard) --------------------
EVIL="$WORK/evilwork"
mkdir -p "$EVIL/out"
python3 - "$WORK/evil.zip" "$WORK" <<'PYEVIL'
import sys, zipfile
target, work = sys.argv[1], sys.argv[2]
with zipfile.ZipFile(target, "w", zipfile.ZIP_DEFLATED) as z:
    z.writestr("../evil_escape.txt", "traversal payload")
    z.writestr(work + "/abs_escape.txt", "absolute payload")
    z.writestr("safe/ok.txt", "safe payload")
PYEVIL
(cd "$EVIL/out" && "$ZYPR_BIN" "$WORK/evil.zip" > /dev/null 2>&1)
slip=0
if [ -e "$EVIL/evil_escape.txt" ]; then
    echo "FAIL: ../ traversal wrote outside cwd"
    slip=1
fi
if [ -e "$WORK/abs_escape.txt" ]; then
    echo "FAIL: absolute path wrote outside cwd"
    slip=1
fi
if [ ! -f "$EVIL/out/safe/ok.txt" ]; then
    echo "FAIL: safe entry not extracted"
    slip=1
fi
if [ "$slip" -eq 0 ]; then
    echo "PASS: traversal and absolute paths skipped, safe entry extracted"
else
    failures=$((failures + 1))
fi

if [ "$failures" -gt 0 ]; then
    echo "$failures integration test(s) FAILED"
    exit 1
fi
echo "all integration tests passed"
