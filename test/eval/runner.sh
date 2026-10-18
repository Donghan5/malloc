#!/usr/bin/env bash
# Linux-only evaluator. Bash controls execution; jq handles structured evidence.
set -u -o pipefail
ROOT=$(CDPATH='' cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)
source "$ROOT/test/eval/common.sh"
source "$ROOT/test/eval/calloc.sh"
source "$ROOT/test/eval/zones.sh"
TITLES=('Preliminary tests' 'Library compilation' 'Functions export' 'Malloc test' 'Pre-allocated zones' 'Tests of free' 'Quality of the free function' 'Realloc test' 'Show_alloc_mem test' 'Alignment test')
EXTRA=('Request alignment overflow' 'Metadata addition overflow' 'Page rounding overflow' 'Original data and ownership after realloc failure')
BONUS=('Competitive access' 'Additional bonuses — Debug environment variables' 'Additional bonuses — Extended output' 'Additional bonuses — Adjacent free block coalescing and reuse')

run_command() {
    local started ended count
    count=$(wc -l < "$RECORDS")
    LAST_OUT="$WORK/command-$count.stdout"; LAST_ERR="$WORK/command-$count.stderr"
    started=$(date +%s%N)
    if (exec 9>&-; cd "$WORK" && timeout --signal=TERM --kill-after=2s "${RUN_TIMEOUT:-30}s" \
        env -i "PATH=$PATH" LC_ALL=C "${RUN_ENV[@]}" "$@") >"$LAST_OUT" 2>"$LAST_ERR"; then
        LAST_EXIT=0
    else LAST_EXIT=$?; fi
    ended=$(date +%s%N)
    jq -nc --args --argjson code "$LAST_EXIT" --argjson started "$started" --argjson ended "$ended" \
        --rawfile stdout "$LAST_OUT" --rawfile stderr "$LAST_ERR" \
        '{command:$ARGS.positional,exit:$code,stdout:$stdout,stderr:$stderr,started_ns:$started,finished_ns:$ended}' -- "$@" >> "$RECORDS"
}

add_row() {
    local id=$1 title=$2 description=$3 status=$4 detail=${5:-} expected=${6:-Exit 0 and all check conditions satisfied} actual=${7:-}
    local score_info=null
    if [[ $id == E04 && -f $WORK/allocation.json ]]; then
        score_info=$(jq -c 'if .valid then . else null end' "$WORK/allocation.json")
    fi
    jq -nc --arg id "$id" --arg title "$title" --arg description "$description" --arg status "$status" \
        --argjson score_info "$score_info" --arg detail "$detail" --arg expected "$expected" --arg actual "$actual" --arg log "$LOG" --arg reproduce "make $COMMAND" \
        '{id:$id,title:$title,description:$description,status:$status,score_info:$score_info,detail:$detail,expected:$expected,actual:$actual,log:$log,reproduce:$reproduce}' >> "$ROWS"
}

check_result() {
    local status=FAIL
    [[ $1 == 0 ]] && status=PASS
    add_row "$2" "$3" "$4" "$status" "${5:-}" "${6:-Exit 0 and all check conditions satisfied}" \
        "${7:-exit=$LAST_EXIT; stdout=$(cat "$LAST_OUT"); stderr=$(cat "$LAST_ERR")}"
}

prerequisites() {
    local source_ok=1 path digest actual mode expected valid before after ok=1 index=0 no_relink link
    while IFS=$'\t' read -r path digest; do
        [[ -f $ROOT/$path ]] || { source_ok=0; continue; }
        actual=$(sha256sum "$ROOT/$path"); [[ ${actual%% *} == "$digest" ]] || source_ok=0
    done < <(jq -r '.files|to_entries[]|[.key,.value]|@tsv' "$PROVENANCE")
    if [[ $source_ok == 1 ]] && jq -e '.manual_review_accepted==true' "$PROVENANCE" >/dev/null; then
        add_row E01 "${TITLES[0]}" 'Submission, allowed functions, globals and user manual confirmation' PASS 'Designated source SHA-256 matches; existing user manual approval recorded (not an automated source review)'
    else
        add_row E01 "${TITLES[0]}" 'Submission and manual confirmation' 'UNVERIFIABLE — Manual approval missing or designated source hash mismatch'
        return 1
    fi
    cp -R "$ROOT/src" "$ROOT/inc" "$WORK/" || return 1
    cp "$ROOT/Makefile" "$WORK/Makefile" || return 1
    : > "$WORK/builds.jsonl"
    for mode in Testing __unset__ ''; do
        RUN_ENV=(); [[ $mode != __unset__ ]] && RUN_ENV=("HOSTTYPE=$mode")
        if [[ $index == 0 ]]; then run_command make re; else run_command make all; fi
        expected="libft_malloc_${mode}.so"
        [[ $mode == Testing ]] || expected="libft_malloc_$(uname -m)_Linux.so"
        valid=false; no_relink=false
        link=$(readlink -f "$WORK/libft_malloc.so" 2>/dev/null || :)
        [[ $LAST_EXIT == 0 && -f $WORK/$expected && -L $WORK/libft_malloc.so && $link == "$WORK/$expected" ]] && valid=true
        before=$(find "$WORK" -maxdepth 1 -name 'libft_malloc*' -printf '%f %T@ %l\n' | LC_ALL=C sort)
        run_command make all
        after=$(find "$WORK" -maxdepth 1 -name 'libft_malloc*' -printf '%f %T@ %l\n' | LC_ALL=C sort)
        [[ $LAST_EXIT == 0 && $before == "$after" ]] && no_relink=true
        [[ $valid == true && $no_relink == true ]] || ok=0
        jq -nc --arg mode "$mode" --arg expected "$expected" --argjson valid "$valid" --argjson no_relink "$no_relink" \
            '{HOSTTYPE:(if $mode=="__unset__" then null else $mode end),expected:$expected,valid:$valid,no_relink:$no_relink}' >> "$WORK/builds.jsonl"
        index=$((index+1))
    done
    RUN_ENV=()
    if [[ $ok == 1 ]]; then actual=PASS; else actual=FAIL; fi
    add_row E02 "${TITLES[1]}" 'Environment HOSTTYPE, filenames, symlink and no-op rebuild' "$actual" \
        "HOSTTYPE=Testing/unset/empty; filename, symlink and no-op rebuild checks=$actual" 'Correct filenames/symlinks in three environments; no unnecessary relinking'
    [[ $ok == 1 ]] || return 1
    LIB=$(readlink -f "$WORK/libft_malloc.so"); LIB_HASH=$(sha256sum "$LIB"); LIB_HASH=${LIB_HASH%% *}
    run_command nm -D --defined-only "$LIB"
    ok=$LAST_EXIT
    for path in malloc free realloc show_alloc_mem; do
        awk -v symbol="$path" '$2=="T" && $3==symbol {found=1} END {exit !found}' "$LAST_OUT" || ok=1
    done
    check_result "$ok" E03 "${TITLES[2]}" 'Four exported ELF definitions from the submission build' 'Expected public definitions=malloc/free/realloc/show_alloc_mem; ELF symbol checks completed'
    [[ $ok == 0 ]] || return 1
    PROBE="$WORK/probe"
    run_command gcc -std=gnu11 -fno-builtin -I"$WORK/inc" "$ROOT/test/spec/probe.c" "$ROOT/test/spec/probe_data.c" "$ROOT/test/spec/probe_alignment.c" "$LIB" -pthread -o "$PROBE"
    if [[ $LAST_EXIT != 0 ]]; then
        add_row pending 'Probe build' 'Probe compilation' FAIL '' 'Probe build success' "exit=$LAST_EXIT"
        return 1
    fi
    return 0
}

prepare_binary() {
    local n=$1
    [[ -v COMPILED[$n] ]] && return "${COMPILED[$n]}"
    mkdir -p "$WORK/test/correction"
    cp "$ROOT/test/correction/test$n.c" "$WORK/test/correction/test$n.c" || return 1
    local -a args=(gcc -O0 -fno-builtin "$WORK/test/correction/test$n.c" -o "$WORK/test$n")
    [[ $n != 5 ]] || args+=("$LIB" -pthread)
    run_command "${args[@]}"
    COMPILED[$n]=$LAST_EXIT
    return "$LAST_EXIT"
}

launch() { run_command sh "$ROOT/test/eval/linux_preload.sh" "$LIB" "$@"; }

measure() {
    local label=$1 n=$2 user=$3 selected=$4 purpose parsed valid=false library=null hash=null
    [[ ! -f $WORK/measure-$label.json ]] || return 0
    if prepare_binary "$n"; then
        if [[ $user == true ]]; then launch /usr/bin/time -v "$WORK/test$n"
        else run_command /usr/bin/time -v "$WORK/test$n"; fi
    fi
    parsed=$(parse_time "$LAST_ERR") || parsed='{}'
    if [[ $LAST_EXIT == 0 && ! -s $LAST_OUT ]] && jq -e 'has("minor")' <<< "$parsed" >/dev/null; then valid=true; fi
    purpose='Comparison baseline'; [[ $n != "$selected" ]] || purpose='Selected test'
    local preload='{}'
    if [[ $user == true ]]; then
        library=$(jq -Rn --arg v "$LIB" '$v'); hash=$(jq -Rn --arg v "$LIB_HASH" '$v')
        preload=$(jq -n --arg v "$LIB" '{LD_PRELOAD:$v}')
    fi
    jq -n --arg label "$label" --arg purpose "$purpose" --arg path "$PATH" --argjson parsed "$parsed" \
        --argjson valid "$valid" --argjson code "$LAST_EXIT" --argjson page "$PAGE" --argjson library "$library" --argjson hash "$hash" \
        --argjson preload "$preload" --slurpfile records "$RECORDS" \
        '$parsed+{label:$label,exit:$code,page_size:$page,valid:$valid,purpose:$purpose,library:$library,library_sha256:$hash,
         environment:({PATH:$path,LC_ALL:"C"}+$preload),command:$records[-1].command,
         started_ns:$records[-1].started_ns,finished_ns:$records[-1].finished_ns,
         provenance:"User-designated official source + explicit Linux preload adapter"}' > "$WORK/measure-$label.json"
}

measurement_plan() {
    case $1 in
        test0) printf '%s\n' 'S0 0 false' 'U0 0 true';;
        test1) printf '%s\n' 'S0 0 false' 'S1 1 false' 'U0 0 true' 'U1 1 true';;
        test2) printf '%s\n' 'U0 0 true' 'U1 1 true' 'U2 2 true';;
    esac
}

measurement_checks() {
    local command=$1 label n user valid=1 detail status extra system_delta user_delta score values
    local -a selected_files=()
    run_command /usr/bin/time --version
    if [[ $LAST_EXIT != 0 ]] || ! rg -q GNU "$LAST_OUT" "$LAST_ERR"; then
        if [[ $command == test2 ]]; then n=E06; label=${TITLES[5]}; else n=E04; label=${TITLES[3]}; fi
        add_row "$n" "$label" 'GNU time availability' 'UNVERIFIABLE — GNU time unavailable or unsupported format'
        if [[ $command == test2 ]]; then
            add_row E07 "${TITLES[6]}" 'GNU time availability' 'UNVERIFIABLE — GNU time unavailable or unsupported format'
        fi
        return
    fi
    while read -r label n user; do
        measure "$label" "$n" "$user" "${command: -1}"
        selected_files+=("$WORK/measure-$label.json")
        jq -e '.valid' "$WORK/measure-$label.json" >/dev/null || valid=0
    done < <(measurement_plan "$command")
    values=$(jq -sc 'map({key:.label,value:.minor})|from_entries' "${selected_files[@]}")
    [[ $valid == 1 ]] && status=PASS || status=FAIL
    case $command in
        test0)
            add_row E04 "${TITLES[3]}" 'Baseline' "$status" \
                "S0=$(jq .S0 <<< "$values"), U0=$(jq .U0 <<< "$values"); minor faults are cumulative events, not current resident page counts; no score calculated" \
                'Valid baseline measurements and normal termination' "$values";;
        test1)
            detail="S0=$(jq .S0 <<< "$values"), S1=$(jq .S1 <<< "$values"), U0=$(jq .U0 <<< "$values"), U1=$(jq .U1 <<< "$values")"
            if [[ $valid == 1 ]]; then
                system_delta=$(jq '.S1-.S0' <<< "$values"); user_delta=$(jq '.U1-.U0' <<< "$values")
                extra=$((user_delta-system_delta))
                local confirmed=false
                if allocation_interpretation_confirmed "$PROVENANCE"; then confirmed=true; fi
                score=null
                [[ $confirmed != true ]] || score=$(allocation_score "$extra")
                detail+="; system delta=$system_delta, project delta=$user_delta, extra=$extra"
                detail+='; O=(U1-U0)-(S1-S0); minor faults are cumulative events, not current resident page counts'
                if [[ $confirmed == true ]]; then
                    detail+='; adopted baseline-adjusted operational interpretation (not an explicit equation in the evaluation sheet)'
                else
                    detail+='; candidate calculation; operational interpretation unconfirmed'
                    status='UNVERIFIABLE — Measurements valid but calculation method unconfirmed'
                fi
                jq -n --argjson system "$system_delta" --argjson user "$user_delta" --argjson extra "$extra" \
                    --argjson score "$score" --argjson confirmed "$confirmed" \
                    '{valid:true,confirmed:$confirmed,formula:"O=(U1-U0)-(S1-S0)",basis:"eval-sheet.pdf pp.3-5",
                      interpretation:"Operational reading; not an explicit equation in the PDF",
                      system_delta:$system,user_delta:$user,extra:$extra,score:$score,maximum:5}' > "$WORK/allocation.json"
            else
                printf '{"valid":false}\n' > "$WORK/allocation.json"
                detail+='; Measurement failure or missing fields: excluded from scoring'
            fi
            add_row E04 "${TITLES[3]}" 'Allocation overhead' "$status" "$detail" \
                'Normal termination and valid GNU time fields for S0/S1/U0/U1' "$values";;
        test2)
            for n in 6 7; do
                status=FAIL
                detail="baseline=$(jq .U0 <<< "$values"), allocation-only=$(jq .U1 <<< "$values"), malloc/free=$(jq .U2 <<< "$values")"
                if [[ $n == 6 ]]; then
                    detail+='; Expected: malloc/free < allocation-only'
                    [[ $valid == 1 ]] && jq -e '.U2<.U1' <<< "$values" >/dev/null && status=PASS
                elif [[ $valid == 1 ]]; then
                    extra=$(jq '.U2-.U0' <<< "$values"); detail+="; difference=$extra, limit <=10"
                    ((extra<=10)) && status=PASS
                else detail+='; Measurement failure or missing fields'; fi
                add_row "E0$n" "${TITLES[n-1]}" 'Free function / quality' "$status" "$detail" \
                    'Valid measurements and the displayed free criterion satisfied' "$values"
            done;;
    esac
}

attachment() {
    if prepare_binary "$1"; then launch "$WORK/test$1"; fi
}

execute_selected() {
    local command=$1 first second ok=0 detail i binary="$WORK/bonus" eligible=false
    case $command in
        test0|test2) measurement_checks "$command";;
        test1) measurement_checks "$command"; zone_checks;;
        test3)
            attachment 3; first=$LAST_EXIT
            printf 'Bonjours\nBonjours\n' > "$WORK/expected"
            [[ $first == 0 && ! -s $LAST_ERR ]] && cmp -s "$WORK/expected" "$LAST_OUT" || ok=1
            cp "$LAST_OUT" "$WORK/test3.stdout"; cp "$LAST_ERR" "$WORK/test3.stderr"
            attachment 4; second=$LAST_EXIT
            printf 'Bonjours\n' > "$WORK/expected"
            [[ $second == 0 && ! -s $LAST_ERR ]] && cmp -s "$WORK/expected" "$LAST_OUT" || ok=1
            check_result "$ok" E08 "${TITLES[7]}" 'Designated test3 realloc string; designated test4 invalid-pointer subcase' \
              "Expected strings=two/one Bonjours lines; exits=$first/$second; Designated source string differs from Hello world! in the evaluation sheet" \
              'test3: two Bonjours lines; test4: one Bonjours line; exit 0' \
              "test3 exit=$first; stdout=$(cat "$WORK/test3.stdout"); stderr=$(cat "$WORK/test3.stderr"); test4 exit=$second; stdout=$(cat "$LAST_OUT"); stderr=$(cat "$LAST_ERR")";;
        test4)
            attachment 5; ok=$LAST_EXIT
            [[ $ok == 0 && ! -s $LAST_ERR ]] && validate_show "$LAST_OUT" '1024 32768 1048576 16777216 134217728' || ok=1
            check_result "$ok" E09 "${TITLES[8]}" 'Runs designated source test5, which checks allocation output' \
              '5 allocations; parse classes, address order, ranges, sizes and total; designated test5 runs as the output case' \
              'Matching classes, address order, ranges, sizes and total for five allocations';;
        test5)
            run_command "$PROBE" E10
            check_result "$LAST_EXIT" E10 "${TITLES[9]}" 'Checks proper alignment from subject p.6 using Linux ABI max_align_t' \
              "$(cat "$LAST_OUT"); malloc/realloc boundaries and data preservation; designated test5 retained as output case, alignment check derived from subject" \
              'All returned addresses are multiples of _Alignof(max_align_t); live data is preserved';;
        test-edge)
            for i in 1 2 3 4; do
                run_command "$PROBE" "O0$i"
                check_result "$LAST_EXIT" "O0$i" "${EXTRA[i-1]}" 'Additional overflow check outside the official score' 'Expected=NULL, original 64-byte pattern preserved and safely freed; arithmetic boundaries assessed with source review'
            done;;
        test-bonus)
            run_command gcc -std=gnu11 -fno-builtin -I"$WORK/inc" "$ROOT/test/spec/bonus.c" "$ROOT/test/spec/calloc_common.c" "$LIB" -pthread -o "$binary"
            local built=$LAST_EXIT
            if jq -se '[.[]|select(.id|startswith("E"))]|length==10 and all(.[];.status=="PASS")' "$ROWS" >/dev/null && \
                [[ -f $WORK/allocation.json ]] && jq -e '.score==5' "$WORK/allocation.json" >/dev/null; then eligible=true; fi
            for i in 1 2 3 4; do
                ok=$built
                if [[ $built == 0 ]]; then
                    run_command "$binary" "B0$i"; ok=$LAST_EXIT
                    if [[ $i == 2 ]]; then
                        [[ ! -s $LAST_OUT ]] || ok=1
                        RUN_ENV=(MALLOC_DEBUG=1 MALLOC_SCRIBBLE=1)
                        run_command "$binary" B02; RUN_ENV=()
                        [[ $LAST_EXIT == 0 ]] && rg -q '\[MALLOC\]' "$LAST_OUT" || ok=1
                    elif [[ $i == 3 ]]; then
                        awk '/^([0-9a-f][0-9a-f] )+$/ {for(i=1;i<=NF;i++){n++;if($i!="12")bad=1}} END {exit bad || n!=32}' "$LAST_OUT" || ok=1
                        sed '/^\([0-9a-f][0-9a-f] \)\+$/d;s/===== Show Allocated Memory (Hex Dump) =====/===== Show Allocated Memory =====/' "$LAST_OUT" > "$WORK/memory-output"
                        validate_show "$WORK/memory-output" || ok=1
                    fi
                fi
                if [[ $eligible == true ]]; then detail='Eligible for official bonus review — mandatory checks and user manual confirmation passed. Bonus scoring is up to the evaluator.'
                else detail='Excluded from official bonus grading — complete mandatory compliance has not been established. Development results are separate; no bonus score is assigned.'; fi
                [[ $i != 1 ]] || detail+=' A finite concurrent workload does not prove the absence of races.'
                check_result "$ok" "B0$i" "${BONUS[i-1]}" 'Representative development check of an implemented bonus' "$detail"
            done
            calloc_checks;;
    esac
}

mark_not_run() {
    local ids='E01 E02 E03' id
    case $COMMAND in
        test0) ids+=' E04';;
        test1) ids+=' E04 E05';;
        test2) ids+=' E06 E07';;
        test3) ids+=' E08';;
        test4) ids+=' E09';;
        test5) ids+=' E10';;
        test-edge) ids+=' O01 O02 O03 O04';;
        all) ids+=' E04 E05 E06 E07 E08 E09 E10 O01 O02 O03 O04';;
    esac
    for id in $ids; do
        if ! jq -se --arg id "$id" 'any(.[]; .id==$id)' "$ROWS" >/dev/null; then
            add_row "$id" 'Selected test' 'Prerequisite stop' 'NOT RUN — Common prerequisites not satisfied' 'Not executed because prerequisites were not satisfied'
        fi
    done
}

finish() {
    jq -sc 'sort_by((if .id|startswith("E") then 0 elif .id|startswith("O") then 1 else 2 end),.id)[]' "$ROWS" > "$WORK/sorted-rows"
    mv "$WORK/sorted-rows" "$ROWS"
    render_rows "$COMMAND" "$ROWS"
    local file name compiler
    render_summary "$COMMAND" "$ROWS"
    run_command gcc --version
    compiler=$(tail -n 1 "$RECORDS")
    : > "$WORK/measurements.jsonl"
    for name in S0 S1 U0 U1 U2; do
        [[ ! -f $WORK/measure-$name.json ]] || jq -c . "$WORK/measure-$name.json" >> "$WORK/measurements.jsonl"
    done
    [[ -f $WORK/allocation.json ]] || printf 'null\n' > "$WORK/allocation.json"
    [[ -f $WORK/zones.jsonl ]] || : > "$WORK/zones.jsonl"
    : > "$WORK/hashes.jsonl"
    while IFS= read -r -d '' file; do
        name=$(sha256sum "$file"); name=${name%% *}
        jq -nc --arg key "${file#"$ROOT/"}" --arg value "$name" '{key:$key,value:$value}' >> "$WORK/hashes.jsonl"
    done < <(find "$ROOT/src" "$ROOT/inc" -type f -print0; printf '%s\0' "$ROOT/Makefile")
    jq -n --arg command "$COMMAND" --arg library "$LIB" --arg hash "$LIB_HASH" --arg workspace "$WORK" --argjson page "$PAGE" \
      --arg platform "$(uname -a)" --argjson compiler "$compiler" \
      --arg git "$(git -C "$ROOT" rev-parse HEAD)" --arg worktree "$(git -C "$ROOT" status --short)" \
      --slurpfile rows "$ROWS" --slurpfile records "$RECORDS" --slurpfile provenance "$PROVENANCE" \
      --slurpfile zones "$WORK/zones.jsonl" --slurpfile allocation "$WORK/allocation.json" --slurpfile measurements "$WORK/measurements.jsonl" --slurpfile hashes "$WORK/hashes.jsonl" \
      '{command:$command,rows:$rows,records:$records,source_provenance:$provenance[0],allocation:$allocation[0],zones:$zones,
        measurements:($measurements|map({key:.label,value:del(.label)})|from_entries),library:$library,library_sha256:$hash,
        page_size:$page,workspace:$workspace,platform:$platform,compiler:$compiler,git:$git,worktree:$worktree,
        input_sha256:($hashes|from_entries),process_invocations:($records|length)}' > "$LOG" || return 2
    if ! status_exit "$ROWS"; then printf 'Log file: %s\n' "$LOG"; fi
    status_exit "$ROWS"
}

environment_error() {
    local blue green red reset
    ui_colors
    printf '%sDescription: Check the Linux test environment.%s\nLog: %s\nResult: UNVERIFIABLE\n' "$blue" "$reset" "$1"
    return 2
}

main() {
    [[ $(uname -s) == Linux ]] || { environment_error 'Unsupported environment: Linux only'; return 2; }
    [[ -z ${LD_PRELOAD:-} ]] || { environment_error 'LD_PRELOAD must not be applied to the runner itself'; return 2; }
    export LC_ALL=C
    COMMAND=${1:-all}
    [[ $# -le 1 && " all test0 test1 test2 test3 test4 test5 test-edge test-bonus " == *" $COMMAND "* ]] || {
        echo 'Usage: runner.sh [test0|test1|test2|test3|test4|test5|test-edge|test-bonus]'; return 2; }
    local tool
    for tool in jq gcc make nm timeout flock sha256sum getconf awk rg; do
        command -v "$tool" >/dev/null || { environment_error "Required tool unavailable: $tool"; return 2; }
    done
    PAGE=$(getconf PAGESIZE)
    [[ $PAGE =~ ^[1-9][0-9]*$ ]] || { environment_error 'Valid OS page size unavailable'; return 2; }
    PROVENANCE="$ROOT/test/official-sources.json"
    local key
    key=$(printf %s "$ROOT" | sha256sum); key=${key%% *}
    exec 9>"${TMPDIR:-/tmp}/malloc-evaluation-$key.lock"
    flock -x 9 || return 2
    RESULT_DIR=$(mktemp -d "$ROOT/test/evaluation-XXXXXXXX") || return 2
    WORK=$(mktemp -d "${TMPDIR:-/tmp}/malloc-test-build-XXXXXXXX") || return 2
    trap '[[ -z ${WORK:-} ]] || rm -rf -- "$WORK"' EXIT
    trap 'exit 130' INT
    trap 'exit 143' TERM
    ROWS="$WORK/rows.jsonl"; RECORDS="$WORK/records.jsonl"; LOG="$RESULT_DIR/evidence.json"
    : > "$ROWS"; : > "$RECORDS"
    LIB=; LIB_HASH=; PROBE=
    declare -gA COMPILED=()
    declare -ga RUN_ENV=()
    if [[ $COMMAND == test-bonus ]]; then
        cp -R "$ROOT/src" "$ROOT/inc" "$WORK/"
        cp "$ROOT/Makefile" "$WORK/Makefile"
        run_command make all
        if [[ $LAST_EXIT == 0 ]]; then
            LIB=$(readlink -f "$WORK/libft_malloc.so")
            LIB_HASH=$(sha256sum "$LIB"); LIB_HASH=${LIB_HASH%% *}
            cp "$LIB" "$RESULT_DIR/allocator.so"
            execute_selected test-bonus
        else
            add_row pending 'Bonus build' 'Isolated submission build' FAIL '' 'Build success' "exit=$LAST_EXIT"
        fi
    elif prerequisites; then
        if [[ $COMMAND == all ]]; then
            local selected
            for selected in test1 test2 test3 test4 test5 test-edge test-bonus; do execute_selected "$selected"; done
        else execute_selected "$COMMAND"; fi
    else mark_not_run; fi
    finish
}

if [[ ${BASH_SOURCE[0]} == "$0" ]]; then main "$@"; exit $?; fi
