#!/bin/sh
# 평가 원본과 보조 진단은 별도 프로세스에서 실행한다. 기본 결과는 터미널에만 남긴다.
set -eu
if [ "${LD_PRELOAD+x}" ]; then echo 'FAIL: 실행기에는 LD_PRELOAD를 적용하지 마세요.' >&2; exit 2; fi
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd -P)
cd "$root"
library="$root/libft_malloc.so"
probe=./test/bin/integration/fault_probe
mode=${1:-eval}
[ "$mode" != diag ] || probe=${FAULT_PROBE:-$probe}
case "$mode" in eval|binding|diag) ;; *) echo '사용법: run.sh eval [횟수] | binding | diag cold|repeat|phases|fullwrite' >&2; exit 2;; esac
[ "$#" -le 2 ] || { echo 'FAIL: 인자가 너무 많습니다.' >&2; exit 2; }
for command in timeout awk readelf sha256sum readlink mktemp; do
    command -v "$command" >/dev/null || { echo "FAIL: $command 명령이 없습니다." >&2; exit 2; }
done
[ -f "$library" ] && [ -x "$probe" ] || { echo 'FAIL: make test-build 또는 test-diagnostic-build가 필요합니다.' >&2; exit 2; }
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
# timeout과 셸은 주입하지 않고, env 이후의 time/workload만 주입한다.
inject() {
    timeout --signal=TERM --kill-after=2s 15s env -u MALLOC_DEBUG -u MALLOC_SCRIBBLE \
        -u LD_DEBUG -u LD_DEBUG_OUTPUT -u LD_BIND_NOW -u EXPECTED_MALLOC_LIBRARY \
        "LD_LIBRARY_PATH=$root" "LD_PRELOAD=$library" "$@" &
    child=$!
    if wait "$child"; then result=0; else result=$?; fi
    child=
    return "$result"
}
printf '라이브러리: %s\n' "$(readlink -f "$library")"
sha256sum "$library"
if [ "$mode" = diag ]; then
    section=${2:-repeat}
    case "$section" in
        cold|repeat|fullwrite) inject "EXPECTED_MALLOC_LIBRARY=$library" "$probe" "$section";;
        phases)
            failed=0
            for section in baseline free; do
                printf '\n보조 진단: %s\n' "$section"
                if inject "EXPECTED_MALLOC_LIBRARY=$library" "$probe" "$section"; then :; else failed=1; fi
            done
            exit "$failed";;
        *) echo 'FAIL: 진단 모드는 cold, repeat, phases 또는 fullwrite입니다.' >&2; exit 2;;
    esac
    exit 0
fi
[ -x /usr/bin/time ] || { echo 'FAIL: /usr/bin/time이 없습니다. 다른 측정으로 대체하지 않습니다.' >&2; exit 2; }
/usr/bin/time --version | awk '/GNU/ {found=1} END {exit !found}' || { echo 'FAIL: GNU time 확인 실패' >&2; exit 2; }
# 점수 측정과 별개로 실제 correction 및 time 심볼 바인딩을 확인한다.
inject "EXPECTED_MALLOC_LIBRARY=$library" "$probe" binding
for n in 0 1 2; do
    binary=./test/bin/eval/test$n
    readelf -d "$binary" | awk '/\(NEEDED\)/ {n++; if ($0 !~ /\[libc\.so\.6\]/) bad=1} END {exit n!=1 || bad}' \
        || { echo "FAIL: $binary libc 단독 링크 확인 실패" >&2; exit 2; }
    [ "$n" -gt 0 ] || continue
    inject LD_DEBUG=bindings /usr/bin/time -v "$binary" 2>"$temporary/binding"
    awk -v binary="$binary" -v library="$library" -v n="$n" '
        BEGIN {quote=sprintf("%c",39)}
        index($0," to " library " ") {
            malloc=index($0,"symbol `malloc" quote) || index($0,"symbol " quote "malloc" quote)
            free=index($0,"symbol `free" quote) || index($0,"symbol " quote "free" quote)
            if (index($0,"binding file " binary " ")) {m+=malloc; f+=free}
            if (index($0,"binding file /usr/bin/time ")) {tm+=malloc; tf+=free}
        }
        END {exit !(m && (n==1 || f) && tm && tf)}' "$temporary/binding" \
        || { cat "$temporary/binding" >&2; echo 'FAIL: correction/time 주입 확인 실패' >&2; exit 2; }
done
echo 'PASS: correction/time 실제 malloc/free 바인딩 확인 (별도 실행)'
[ "$mode" != binding ] || exit 0
repeats=${2:-1}
case "$repeats" in ''|*[!0-9]*) echo 'FAIL: 반복 횟수는 양의 정수여야 합니다.' >&2; exit 2;; esac
[ "$repeats" -gt 0 ] || { echo 'FAIL: 반복 횟수는 양의 정수여야 합니다.' >&2; exit 2; }
echo '평가: 반복 baseline hold free hold차이 free차이 실행 할당 free품질 free기능'
failed=0
iteration=1
while [ "$iteration" -le "$repeats" ]; do
    execution=1
    for n in 0 1 2; do
        if inject /usr/bin/time -v ./test/bin/eval/test$n 2>"$temporary/time"; then status=0; else status=$?; fi
        cat "$temporary/time"
        if [ "$status" -ne 0 ]; then echo "FAIL: test$n 실행 종료 코드=$status" >&2; execution=0; fi
        if faults=$(awk '
            /^[[:space:]]*Minor \(reclaiming a frame\) page faults:/ {
                count++; value=$0; sub(/^[^:]*:[[:space:]]*/,"",value); sub(/[[:space:]]*$/,"",value)
                if (value !~ /^[0-9]+$/) bad=1
            }
            END {if (count==1 && !bad) print value; else exit 1}' "$temporary/time"); then :
        else faults=NA; execution=0; echo "FAIL: test$n minor faults 파싱 실패" >&2; fi
        case "$n" in 0) baseline=$faults;; 1) hold=$faults;; 2) free=$faults;; esac
    done
    if awk -v i="$iteration" -v b="$baseline" -v h="$hold" -v f="$free" -v ok="$execution" '
        BEGIN {
            dh=(b!="NA" && h!="NA") ? h-b : "NA"; df=(b!="NA" && f!="NA") ? f-b : "NA"
            a=ok && dh>=255 && dh<=272; q=ok && df<=3; fn=ok && f<h
            printf "%d %s %s %s %s %s %s %s %s %s\n",i,b,h,f,dh,df,ok?"PASS":"FAIL",a?"PASS":"FAIL",q?"PASS":"FAIL",fn?"PASS":"FAIL"
            exit !(ok && a && q && fn)
        }'; then :; else failed=$((failed+1)); fi
    iteration=$((iteration+1))
done
printf '평가 결과: %s/%s 통과, %s 실패\n' "$((repeats-failed))" "$repeats" "$failed"
[ "$failed" -eq 0 ]
