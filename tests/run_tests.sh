#!/bin/sh
# Run each case in its own process so crashes do not stop the suite.
ulimit -c 0 || exit 1
binary=${1:-./tests/edge_cases}
count=$("$binary") || exit 1
case "$count" in
    ''|*[!0-9]*) echo 'Invalid test count' >&2; exit 1 ;;
esac
index=0
failures=0
while [ "$index" -lt "$count" ]; do
    "$binary" "$index"
    status=$?
    if [ "$status" -ne 0 ]; then
        failures=$((failures + 1))
        if [ "$status" -gt 128 ]; then
            echo "FAIL case $index (signal $((status - 128)))"
        fi
    fi
    index=$((index + 1))
done
echo "$((count - failures))/$count passed"
echo 'UPCOMING FEATURE: thread safety, debug/scribble, show_alloc_mem_ex, defragmentation'
[ "$failures" -eq 0 ]
