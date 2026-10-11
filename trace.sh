#!/bin/sh

(
    unset LD_PRELOAD LD_BIND_NOW LD_DEBUG LD_DEBUG_OUTPUT
    export LC_ALL=C

    evidence_dir=$(mktemp -d /tmp/malloc-pair.XXXXXX)
    printf '기록 위치: %s\n' "$evidence_dir"
    sha256sum ./libft_malloc.so > "$evidence_dir/library.sha256"

    for n in 0 1 2; do
        perf record \
            -e minor-faults:u -c 1 -d \
            --call-graph dwarf,8192 \
            -o "$evidence_dir/test$n.data" \
            -- env -u MALLOC_DEBUG -u MALLOC_SCRIBBLE \
            LD_LIBRARY_PATH="$PWD" \
            LD_PRELOAD="$PWD/libft_malloc.so" \
            /usr/bin/time -v \
            -o "$evidence_dir/test$n.time" \
            "./test/bin/eval/test$n" \
            > "$evidence_dir/test$n.stdout" \
            2> "$evidence_dir/test$n.record-errors"

        printf '%s\n' "$?" > "$evidence_dir/test$n.status"

        perf script -i "$evidence_dir/test$n.data" \
            --header-only \
            > "$evidence_dir/test$n.header" \
            2> "$evidence_dir/test$n.header-errors"

        perf script -i "$evidence_dir/test$n.data" \
            --show-task-events --show-mmap-events \
            > "$evidence_dir/test$n.script" \
            2> "$evidence_dir/test$n.decode-errors"

        perf script -D -i "$evidence_dir/test$n.data" \
            > "$evidence_dir/test$n.raw" \
            2> "$evidence_dir/test$n.raw-errors"
    done

    awk '
        /Minor \(reclaiming a frame\) page faults:/ {
            minor[FILENAME] = $NF
            fields[FILENAME]++
        }
        /Exit status:/ {
            status[FILENAME] = $NF
            exits[FILENAME]++
        }
        END {
            for (i = 1; i <= 3; i++) {
                f = ARGV[i]
                if (fields[f] != 1 || exits[f] != 1 || status[f] != 0) {
                    print "INVALID: time 필드 또는 종료 코드:", f
                    exit 1
                }
            }
            b = minor[ARGV[1]] + 0
            h = minor[ARGV[2]] + 0
            f = minor[ARGV[3]] + 0
            printf "baseline=%d hold=%d free=%d\n", b, h, f
            printf "hold 차이=%d free 차이=%d\n", h-b, f-b
            printf "free 품질=%s\n", f-b <= 3 ? "PASS" : "FAIL"
        }
    ' "$evidence_dir/test0.time" \
      "$evidence_dir/test1.time" \
      "$evidence_dir/test2.time"

    printf '\n손실·throttle 기록 검사:\n'
    rg -n 'PERF_RECORD_(LOST|LOST_SAMPLES|THROTTLE|UNTHROTTLE)' \
        "$evidence_dir"/test*.raw

    printf '\nperf 종료 코드:\n'
    for n in 0 1 2; do
        printf 'test%s: ' "$n"
        cat "$evidence_dir/test$n.status"
    done
)
