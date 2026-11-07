#!/usr/bin/env bash
# Build ZYPR and run the full test suite (unit + integration).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT"

echo "== build =="
make

echo "== generate fixtures =="
python3 tests/gen_fixtures.py

echo "== build unit test driver =="
gcc -Wall -Wextra -o tests/test_inflate tests/test_inflate.c src/inflate.c -Iinclude

echo "== inflate unit tests =="
failures=0
for expected in tests/fixtures/*.expected; do
    name="$(basename "$expected" .expected)"
    if ! ./tests/test_inflate "tests/fixtures/$name.deflate" "$expected"; then
        failures=$((failures + 1))
    fi
done
if ! ./tests/test_inflate --robust tests/fixtures/truncated.deflate; then
    failures=$((failures + 1))
fi

echo "== zip integration tests =="
if ! bash tests/test_zip.sh; then
    failures=$((failures + 1))
fi

if [ "$failures" -gt 0 ]; then
    echo "RESULT: $failures test group(s) FAILED"
    exit 1
fi
echo "RESULT: all tests passed"
