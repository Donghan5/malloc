#!/usr/bin/env bash
# Fixtures are temporary and never participate in allocator measurements.
set -euo pipefail
export NO_COLOR=1
source "$(dirname -- "${BASH_SOURCE[0]}")/runner.sh"
FIXTURE=$(mktemp -d "${TMPDIR:-/tmp}/malloc-runner-fixtures-XXXXXXXX")
trap 'rm -rf -- "$FIXTURE"' EXIT
PAGE=$(getconf PAGESIZE)
checks=0
pass() { checks=$((checks+1)); printf 'PASS: %s\n' "$1"; }
reject() { if "$@"; then echo 'FAIL: Expected rejection' >&2; exit 1; else local code=$?; [[ $code == 1 ]] || { echo "FAIL: Unexpected error $code" >&2; exit 1; }; fi; }

WORK=$FIXTURE; RECORDS="$WORK/records.jsonl"; : > "$RECORDS"
RUN_ENV=(); RUN_TIMEOUT=5
run_command sh -c 'printf success'
[[ $LAST_EXIT == 0 && $(cat "$LAST_OUT") == success ]]
run_command sh -c 'exit 7'
[[ $LAST_EXIT == 7 ]]
jq -se 'length==2 and .[1].exit==7 and (.[0].command|length)==3' "$RECORDS" >/dev/null
pass 'Child PASS/FAIL and command evidence'
RUN_TIMEOUT=.05
run_command sh -c 'sleep 5'
[[ $LAST_EXIT == 124 ]]
RUN_TIMEOUT=30
pass 'Timeout returns 124'

cat > "$FIXTURE/time" <<'TIME'
 Major (requiring I/O) page faults: 0
 Minor (reclaiming a frame) page faults: 71
 Maximum resident set size (kbytes): 1200
TIME
parse_time "$FIXTURE/time" | jq -e '.major==0 and .minor==71 and .rss_kib==1200' >/dev/null
reject parse_time /dev/null
cp "$FIXTURE/time" "$FIXTURE/time-duplicate"
printf 'Minor (reclaiming a frame) page faults: 72\n' >> "$FIXTURE/time-duplicate"
reject parse_time "$FIXTURE/time-duplicate"
sed 's/faults: 71/faults: broken/' "$FIXTURE/time" > "$FIXTURE/time-invalid"
reject parse_time "$FIXTURE/time-invalid"
pass 'Time parser rejects missing, duplicate and invalid fields'

ROWS="$FIXTURE/rows.jsonl"; LOG="$FIXTURE/evidence.json"; COMMAND=test0
: > "$ROWS"
add_row E01 'Preliminary tests' 'Fixture only' 'UNVERIFIABLE — manual review missing'
add_row E02 'Library compilation' 'Fixture only' PASS
render_rows test0 "$ROWS" > "$FIXTURE/output"
render_summary test0 "$ROWS" > "$FIXTURE/summary"
grep -q 'Mandatory summary' "$FIXTURE/summary"
[[ $(grep -c 'Result: PASS' "$FIXTURE/summary") == 1 ]]
reject grep -q E01 "$FIXTURE/output"
reject status_exit "$ROWS"
render_rows test1 "$ROWS" | grep -q 'Result: UNVERIFIABLE'
pass 'test0 hides E01 without bypassing its failure'
: > "$ROWS"
add_row E04 'Malloc test' 'Fixture only' FAIL 'Parser failed' 'One valid time field' 'Missing field'
render_rows test1 "$ROWS" > "$FIXTURE/output"
for label in Expected Actual Reproduce 'Log file'; do grep -q "$label:" "$FIXTURE/output"; done
reject status_exit "$ROWS"
pass 'FAIL UI includes expected, actual, reproduction and log'
: > "$ROWS"
add_row pending 'Selected tests' 'Prerequisite stop' 'NOT RUN — prerequisites'
reject status_exit "$ROWS"
add_row B01 'Competitive access' 'Fixture only' 'EXCLUDED — mandatory requirements not met'
reject status_exit "$ROWS"
tail -n 1 "$ROWS" > "$FIXTURE/excluded"
status_exit "$FIXTURE/excluded"
pass 'Prerequisite stop fails; normal bonus exclusion does not'

for entry in '-1 0' '0 5' '20 5' '21 4' '50 4' '51 3' '90 3' '91 2' '180 2' '181 1'; do
    read -r value expected <<< "$entry"
    [[ $(allocation_score "$value") == "$expected" ]]
done
[[ $(( (341-70)-(321-60) )) == 10 ]]
pass 'Baseline arithmetic and every official score boundary'
[[ $(measurement_plan test0 | cut -d' ' -f1 | tr '\n' ' ') == 'S0 U0 ' ]]
[[ $(measurement_plan test1 | cut -d' ' -f1 | tr '\n' ' ') == 'S0 S1 U0 U1 ' ]]
[[ $(measurement_plan test2 | cut -d' ' -f1 | tr '\n' ' ') == 'U0 U1 U2 ' ]]
pass 'Independent target measurement plans'

sizes=(1024 32768 1048576 16777216 134217728)
address=$((0x10000000)); sum=0
: > "$FIXTURE/show"
for size in "${sizes[@]}"; do
    if ((size<=4*PAGE/128)); then group=TINY
    elif ((size<=32*PAGE/128)); then group=SMALL
    else group=LARGE; fi
    printf '%s : 0x%x\n0x%x - 0x%x : %d bytes\n' "$group" "$address" "$((address+80))" "$((address+80+size))" "$size" >> "$FIXTURE/show"
    address=$((address+size+4096)); sum=$((sum+size))
done
printf 'Total : %d bytes\n' "$sum" >> "$FIXTURE/show"
validate_show "$FIXTURE/show" "${sizes[*]}"
sed 's/Total : [0-9]*/Total : 0/' "$FIXTURE/show" > "$FIXTURE/show-invalid"
reject validate_show "$FIXTURE/show-invalid" "${sizes[*]}"
reject validate_show "$FIXTURE/show" "${sizes[*]} 16"
printf 'noise\n' >> "$FIXTURE/show-invalid"
reject validate_show "$FIXTURE/show-invalid" "${sizes[*]}"
pass 'Five-allocation output parser and malformed output rejection'

cat > "$FIXTURE/known" <<'SHOW'
EXPECT 0x100050 32
===== Show Allocated Memory =====
Page size : 4096 bytes
TINY : 0x100000
0x100050 - 0x100070 : 32 bytes
Total : 32 bytes
=================================
SHOW
validate_show "$FIXTURE/known"
sed 's/0x100070/0x100071/' "$FIXTURE/known" > "$FIXTURE/known-invalid"
reject validate_show "$FIXTURE/known-invalid"
# Check exact integer address ordering beyond IEEE-754 integer precision.
sed 's/0x100/0x200000000001/g' "$FIXTURE/known" > "$FIXTURE/high-address"
validate_show "$FIXTURE/high-address"
pass 'Known-address/range parsing uses exact shell integer arithmetic'

mkdir "$FIXTURE/tools"
cat > "$FIXTURE/tools/uname" <<'UNAME'
#!/bin/sh
printf 'Other\n'
UNAME
chmod +x "$FIXTURE/tools/uname"
set +e
PATH="$FIXTURE/tools:$PATH" bash "$ROOT/test/eval/runner.sh" test0 > "$FIXTURE/unsupported"
code=$?
set -e
[[ $code == 2 ]]
grep -q 'Unsupported environment' "$FIXTURE/unsupported"
pass 'Non-Linux environment is rejected before building'

for file in "$ROOT"/test/eval/*.sh; do
    bash -n "$file"
done
# The active eval directory must contain shell scripts only.
[[ -z $(find "$ROOT/test/eval" -maxdepth 1 -name '*.py' -print) ]]
pass 'Shell syntax and no active Python scripts; English Mandatory UI'
# Exercise exact UI order, no visible identifiers and score/exit semantics.
: > "$ROWS"
printf '{"valid":true,"confirmed":true,"score":3,"extra":70}\n' > "$WORK/allocation.json"
add_row E04 'Fixture only' 'Fixture only' PASS 'system delta=260, project delta=330, extra=70'
render_rows test1 "$ROWS" > "$FIXTURE/ui"
awk '/^Description:/ {d=NR} /^Log:/ {l=NR} /^Result:/ {r=NR} END {exit !(d<l && l<r)}' "$FIXTURE/ui"
grep -q 'Score: 3/5' "$FIXTURE/ui"
reject grep -Eq 'E0[1-9]|E10|^[0-9]+[.)]' "$FIXTURE/ui"
status_exit "$ROWS"
render_summary test1 "$ROWS" > "$FIXTURE/summary"
reject grep -q E04 "$FIXTURE/summary"
grep -q 'Score: 3/5' "$FIXTURE/summary"
reject env LC_ALL=C.utf8 grep -Pq '[\x{ac00}-\x{d7a3}]' "$FIXTURE/ui"
pass 'English description/log/result order, hidden IDs and valid partial score exit 0'
: > "$ROWS"
add_row E05 'Fixture only' 'Fixture only' 'UNVERIFIABLE — Single-zone membership unavailable' 'No observation'
render_rows test1 "$ROWS" > "$FIXTURE/ui"
grep -q 'Result: UNVERIFIABLE — Single-zone membership unavailable' "$FIXTURE/ui"
reject status_exit "$ROWS"
: > "$ROWS"
add_row pending 'Fixture only' 'Fixture only' 'NOT RUN — Prerequisites not satisfied'
render_rows test1 "$ROWS" > "$FIXTURE/ui"
grep -q 'Result: NOT RUN — Prerequisites not satisfied' "$FIXTURE/ui"
reject status_exit "$ROWS"
pass 'Unverifiable and not-run UI remain nonzero'
printf '{"valid":true,"confirmed":false,"score":null,"extra":7}\n' > "$WORK/allocation.json"
: > "$ROWS"
add_row E04 'Fixture only' 'Fixture only' 'UNVERIFIABLE — Calculation method unconfirmed' 'Candidate calculation'
render_rows test1 "$ROWS" > "$FIXTURE/ui"
grep -q 'Score: UNCONFIRMED' "$FIXTURE/ui"
reject status_exit "$ROWS"
allocation_interpretation_confirmed "$ROOT/test/official-sources.json"
printf '{}\n' > "$FIXTURE/no-interpretation"
reject allocation_interpretation_confirmed "$FIXTURE/no-interpretation"
jq '.evaluation_interpretations.E04.formula="U1"' "$ROOT/test/official-sources.json" > "$FIXTURE/wrong-formula"
reject allocation_interpretation_confirmed "$FIXTURE/wrong-formula"
pass 'Unconfirmed interpretation has candidate only and no official score' 
printf '%s\n' 'TINY maximum=128 zone=16384 page=4096 capacity=102 same=100 multiple=1 data=1 warmup=0 heap_header=48 block_header=32 alignment=16' > "$FIXTURE/zone"
parsed=$(parse_zone TINY "$FIXTURE/zone")
zone_evidence_valid "$parsed"
reject parse_zone SMALL "$FIXTURE/zone"
sed 's/maximum=128/maximum=128=oops/' "$FIXTURE/zone" > "$FIXTURE/zone-malformed"
reject parse_zone TINY "$FIXTURE/zone-malformed"
printf 'noise\n' >> "$FIXTURE/zone"
reject parse_zone TINY "$FIXTURE/zone"
reject zone_evidence_valid "$(jq '.same=99' <<< "$parsed")"
reject zone_evidence_valid "$(jq '.capacity=99' <<< "$parsed")"
reject zone_evidence_valid "$(jq '.zone=16385' <<< "$parsed")"
reject zone_evidence_valid "$(jq '.data=0' <<< "$parsed")"
pass 'Zone parser rejects missing/malformed evidence, multiple zones, low capacity and non-page size'
# Stop rows are explicit and cannot be mistaken for executed checks.
: > "$ROWS"; COMMAND=test1
add_row E01 'Fixture only' 'Fixture only' 'UNVERIFIABLE — Source hash mismatch'
mark_not_run
jq -se 'length==5 and ([.[]|select(.status|startswith("NOT RUN"))]|length)==4' "$ROWS" >/dev/null
reject status_exit "$ROWS"
pass 'Prerequisite stop marks every selected mandatory check not run'
: > "$ROWS"
LAST_EXIT=124
check_result 124 E04 'Fixture only' 'Fixture only' 'timeout'
reject status_exit "$ROWS"
render_rows test1 "$ROWS" > "$FIXTURE/ui"
grep -q 'Result: FAIL' "$FIXTURE/ui"
: > "$ROWS"
LAST_EXIT=139
check_result 139 E04 'Fixture only' 'Fixture only' 'signal'
reject status_exit "$ROWS"
pass 'Timeout and signal retain FAIL and nonzero status'
# Color checks use the same renderer and remain separate from allocator tests.
: > "$ROWS"
add_row E06 'Fixture only' 'Fixture only' PASS 'color fixture'
add_row E07 'Fixture only' 'Fixture only' FAIL 'color fixture'
(unset NO_COLOR; render_rows test2 "$ROWS") > "$FIXTURE/colors"
grep -Fq $'\033[34mDescription: ' "$FIXTURE/colors"
grep -Fq $'Result: \033[32mPASS\033[0m' "$FIXTURE/colors"
grep -Fq $'Result: \033[31mFAIL\033[0m' "$FIXTURE/colors"
(unset NO_COLOR; render_summary test2 "$ROWS") > "$FIXTURE/color-summary"
grep -Fq $'\033[34mDescription: ' "$FIXTURE/color-summary"
grep -Fq $'Result: \033[31mFAIL\033[0m' "$FIXTURE/color-summary"
render_rows test2 "$ROWS" > "$FIXTURE/plain"
reject grep -Fq $'\033[' "$FIXTURE/plain"
reject status_exit "$ROWS"
pass 'Blue descriptions, green PASS, red FAIL in rows/summary; NO_COLOR preserves plain output'
: > "$ROWS"
add_row B01 'Fixture only' 'Bonus description' PASS 'color fixture'
add_row C01 'Fixture only' 'Calloc description' FAIL 'color fixture'
(unset NO_COLOR; render_rows test-bonus "$ROWS") > "$FIXTURE/bonus-colors"
grep -Fq $'\033[34mDescription: Bonus description\033[0m' "$FIXTURE/bonus-colors"
grep -Fq $'\033[34mDescription: Calloc description\033[0m' "$FIXTURE/bonus-colors"
grep -Fq $'Result: \033[32mPASS\033[0m' "$FIXTURE/bonus-colors"
grep -Fq $'Result: \033[31mFAIL\033[0m' "$FIXTURE/bonus-colors"
(unset NO_COLOR; render_summary test-bonus "$ROWS") > "$FIXTURE/bonus-summary"
grep -Fq $'\033[32mPASS\033[0m' "$FIXTURE/bonus-summary"
grep -Fq $'\033[31mFAIL\033[0m' "$FIXTURE/bonus-summary"
render_rows test-bonus "$ROWS" > "$FIXTURE/bonus-plain"
render_summary test-bonus "$ROWS" >> "$FIXTURE/bonus-plain"
reject grep -Fq $'\033[' "$FIXTURE/bonus-plain"
reject grep -Fq 'color fixture' "$FIXTURE/bonus-plain"
grep -q '^Expected:' "$FIXTURE/bonus-plain"
grep -q '^Reproduce:' "$FIXTURE/bonus-plain"
pass 'Bonus and calloc descriptions/results/summary share Mandatory colors and NO_COLOR support'
printf 'Fixtures: %d passed\n' "$checks"
