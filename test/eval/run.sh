#!/bin/sh
# Check workload bindings and measure page faults in separate processes.
set -eu
if [ "${LD_PRELOAD+x}" ]; then echo 'FAIL: Do not apply LD_PRELOAD to the test runner.' >&2; exit 2; fi
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd -P)
cd "$root"
library="$root/libft_malloc.so"
mode=${1:-eval}
case "$mode" in eval|binding|correction) ;; *) echo 'Usage: run.sh eval [repeats] | binding | correction [extra binaries...]' >&2; exit 2;; esac
if [ "$mode" != correction ] && [ "$#" -gt 2 ]; then echo 'FAIL: Too many arguments.' >&2; exit 2; fi
for command in timeout awk readelf sha256sum readlink mktemp cmp; do
    command -v "$command" >/dev/null || { echo "FAIL: Required command not found: $command." >&2; exit 2; }
done
[ -f "$library" ] || { echo 'FAIL: Run make test-build first.' >&2; exit 2; }
temporary=$(mktemp -d "${TMPDIR:-/tmp}/malloc-tests.XXXXXX")
child=
cleanup() {
    if [ -n "$child" ]; then kill -TERM "$child" 2>/dev/null || :; wait "$child" 2>/dev/null || :; fi
    rm -rf "$temporary"
}
trap cleanup 0
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 142' ALRM
ulimit -c 0
export LC_ALL=C
# Inject the allocator into time/workloads after env; keep timeout and the shell uninjected.
inject() {
    timeout --signal=TERM --kill-after=2s 15s env -u MALLOC_DEBUG -u MALLOC_SCRIBBLE \
        -u LD_DEBUG -u LD_DEBUG_OUTPUT -u LD_BIND_NOW -u EXPECTED_MALLOC_LIBRARY \
        "LD_LIBRARY_PATH=$root" "LD_PRELOAD=$library" "$@" &
    child=$!
    if wait "$child"; then result=0; else result=$?; fi
    child=
    return "$result"
}
printf 'Library: %s\n' "$(readlink -f "$library")"
sha256sum "$library"
[ -x /usr/bin/time ] || { echo 'FAIL: /usr/bin/time is required for evaluation.' >&2; exit 2; }
/usr/bin/time --version | awk '/GNU/ {found=1} END {exit !found}' || { echo 'FAIL: GNU time verification failed.' >&2; exit 2; }
# Verify correction/time symbol bindings separately from scoring.
for n in 0 1 2 3 4 5; do
    binary=./test/bin/eval/test$n
    readelf -d "$binary" | awk -v test="$n" '
        /\(NEEDED\)/ {
            n++
            if ($0 ~ /\[libft_malloc\.so\]/) allocator++
            else if ($0 !~ /\[libc\.so\.6\]/) bad=1
        }
        END {exit bad || (test==5 ? n!=2 || allocator!=1 : n!=1 || allocator!=0)}' \
        || { echo "FAIL: $binary has unexpected direct library dependencies." >&2; exit 2; }
    inject LD_DEBUG=bindings /usr/bin/time -v "$binary" >"$temporary/binding-output" 2>"$temporary/binding"
    awk -v binary="$binary" -v library="$library" -v n="$n" '
        BEGIN {quote=sprintf("%c",39)}
        index($0," to " library " ") {
            malloc=index($0,"symbol `malloc" quote) || index($0,"symbol " quote "malloc" quote)
            free=index($0,"symbol `free" quote) || index($0,"symbol " quote "free" quote)
            realloc=index($0,"symbol `realloc" quote) || index($0,"symbol " quote "realloc" quote)
            show=index($0,"symbol `show_alloc_mem" quote) || index($0,"symbol " quote "show_alloc_mem" quote)
            if (index($0,"binding file " binary " ")) {m+=malloc; f+=free; r+=realloc; s+=show}
            if (index($0,"binding file /usr/bin/time ")) {tm+=malloc; tf+=free}
        }
        END {
            workload=(n==0 || m) && (n!=2 && n!=4 || f) && (n!=3 && n!=4 || r) && (n!=5 || s)
            exit !(workload && tm && tf)
        }' "$temporary/binding" \
        || { cat "$temporary/binding" >&2; echo 'FAIL: Allocator injection into correction/time could not be verified.' >&2; exit 2; }
    echo "PASS: test$n direct dependencies and current allocator bindings verified."
done
echo 'PASS: Actual correction/time malloc/free bindings verified (separate run).'
[ "$mode" != binding ] || exit 0
if [ "$mode" = correction ]; then
    failed=0
    for n in 0 1 2 3 4 5; do
        binary=./test/bin/eval/test$n
        if inject "$binary" >"$temporary/output" 2>"$temporary/errors"; then status=0; else status=$?; fi
        echo "Execute: $binary (exit status $status)"
        cat "$temporary/output" "$temporary/errors"
        valid=1
        case "$n" in
            0|1|2) [ ! -s "$temporary/output" ] || valid=0;;
            3|4)
                printf 'Bonjours\n' >"$temporary/expected"
                [ "$n" != 3 ] || printf 'Bonjours\n' >>"$temporary/expected"
                cmp -s "$temporary/expected" "$temporary/output" || valid=0;;
            5)
                awk '
                    function hex(s, value, j, digit) {
                        value=0
                        for (j=3; j<=length(s); j++) {
                            digit=index("0123456789abcdef",tolower(substr(s,j,1)))-1
                            if (digit<0) {bad=1; return 0}
                            value=value*16+digit
                        }
                        return value
                    }
                    BEGIN {
                        expected[1024]=1; expected[32768]=1; expected[1048576]=1
                        expected[16777216]=1; expected[134217728]=1
                    }
                    /^===== Show Allocated Memory =====$/ {banner++; next}
                    /^Page size : [0-9]+ bytes$/ {page++; if ($4<=0) bad=1; next}
                    /^(TINY|SMALL|LARGE) : 0x[[:xdigit:]]+$/ {
                        addr=hex($3); if (addr<=previous || addr<end) bad=1
                        previous=addr; heaps++; next
                    }
                    /^0x[[:xdigit:]]+ - 0x[[:xdigit:]]+ : [0-9]+ bytes$/ {
                        addr=hex($1); end=hex($3); size=$5
                        if (!heaps || addr<=previous || end-addr!=size || !expected[size]) bad=1
                        previous=addr; expected[size]--; sum+=size; blocks++; next
                    }
                    /^Total : [0-9]+ bytes$/ {total++; reported=$3; next}
                    /^=================================$/ {footer++; next}
                    {bad=1}
                    END {
                        for (size in expected) if (expected[size]!=0) bad=1
                        exit bad || banner!=1 || page!=1 || footer!=1 || heaps!=5 || blocks!=5 || total!=1 || reported!=sum
                    }' "$temporary/output" || valid=0;;
        esac
        [ ! -s "$temporary/errors" ] || valid=0
        if [ "$status" -eq 0 ] && [ "$valid" -eq 1 ]; then
            echo "PASS: test$n execution and expected output verified."
        else
            echo "FAIL: test$n execution or expected output is incorrect."
            failed=$((failed+1))
        fi
    done
    shift
    for binary in "$@"; do
        if inject "./$binary"; then echo "PASS: Extra correction $binary."
        else echo "FAIL: Extra correction $binary."; failed=$((failed+1)); fi
    done
    printf 'Correction result: 6 main workloads checked; %s failed (including extras).\n' "$failed"
    [ "$failed" -eq 0 ]
    exit
fi
repeats=${2:-${REPEATS:-1}}
case "$repeats" in ''|*[!0-9]*) echo 'FAIL: Repeats must be a positive integer.' >&2; exit 2;; esac
[ "$repeats" -gt 0 ] || { echo 'FAIL: Repeats must be a positive integer.' >&2; exit 2; }
echo 'Evaluation: iteration baseline hold free hold_delta free_delta execution allocation free_quality free_function'
failed=0
iteration=1
while [ "$iteration" -le "$repeats" ]; do
    execution=1
    for n in 0 1 2; do
        if inject /usr/bin/time -v ./test/bin/eval/test$n 2>"$temporary/time"; then status=0; else status=$?; fi
        cat "$temporary/time"
        if [ "$status" -ne 0 ]; then echo "FAIL: test$n exited with status $status." >&2; execution=0; fi
        if faults=$(awk '
            /^[[:space:]]*Minor \(reclaiming a frame\) page faults:/ {
                count++; value=$0; sub(/^[^:]*:[[:space:]]*/,"",value); sub(/[[:space:]]*$/,"",value)
                if (value !~ /^[0-9]+$/) bad=1
            }
            END {if (count==1 && !bad) print value; else exit 1}' "$temporary/time"); then :
        else faults=NA; execution=0; echo "FAIL: Could not parse minor page faults for test$n." >&2; fi
        case "$n" in 0) baseline=$faults;; 1) hold=$faults;; 2) free=$faults;; esac
    done
    if awk -v i="$iteration" -v b="$baseline" -v h="$hold" -v f="$free" -v ok="$execution" '
        BEGIN {
            dh=(b!="NA" && h!="NA") ? h-b : "NA"; df=(b!="NA" && f!="NA") ? f-b : "NA"
            a=ok && dh>=255 && dh<=272; q=ok && df<=3; fn=ok && f<h
            printf "%d %s %s %s %s %s %s %s %s %s\n",i,b,h,f,dh,df,ok?"PASS":"FAIL",a?"PASS":"FAIL",ok?(q?"PASS":"WARNING"):"N/A",fn?"PASS":"FAIL"
            if (ok && !q) printf "WARNING: Free quality evaluation criteria not met (free_delta=%s; required <= 3).\n",df
            exit !(ok && a && fn)
        }'; then :; else failed=$((failed+1)); fi
    iteration=$((iteration+1))
done
printf 'Evaluation result: %s/%s passed; %s failed\n' "$((repeats-failed))" "$repeats" "$failed"
[ "$failed" -eq 0 ]
