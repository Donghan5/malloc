#!/usr/bin/env bash
# Shared Linux evaluation helpers. No allocator is loaded into this shell.

allocation_score() {
    local n=$1
    if (( n < 0 )); then echo 0
    elif (( n <= 20 )); then echo 5
    elif (( n <= 50 )); then echo 4
    elif (( n <= 90 )); then echo 3
    elif (( n <= 180 )); then echo 2
    else echo 1; fi
}

allocation_interpretation_confirmed() {
    jq -e '.evaluation_interpretations.E04 |
      .metric=="GNU time minor page faults" and
      (.formula=="O=(U1−U0)−(S1−S0)" or .formula=="O=(U1-U0)-(S1-S0)") and
      (.status|startswith("Adopted operational interpretation"))' "$1" >/dev/null
}

parse_time() {
    LC_ALL=C awk '
        function field(key, value) {
            sub(/^[^:]*:[[:space:]]*/, "", value)
            sub(/[[:space:]]*$/, "", value)
            count[key]++; data[key]=value
            if (value !~ /^[0-9]+$/) bad=1
        }
        /^[[:space:]]*Major \(requiring I\/O\) page faults:/ {field("major",$0)}
        /^[[:space:]]*Minor \(reclaiming a frame\) page faults:/ {field("minor",$0)}
        /^[[:space:]]*Maximum resident set size \(kbytes\):/ {field("rss",$0)}
        END {
            if (bad || count["major"]!=1 || count["minor"]!=1 || count["rss"]!=1) exit 1
            printf "{\"major\":%s,\"minor\":%s,\"rss_kib\":%s}\n",data["major"],data["minor"],data["rss"]
        }' "$1"
}

validate_show() {
    local file=$1 sizes=${2:-} line address start end size group='' previous=-1 last_end=0
    local seen_total=0 total=0 sum=0 blocks=0 key wanted
    local -A expected_blocks=() actual_blocks=() counts=()
    local expect_re='^EXPECT (0x[0-9a-fA-F]+) ([0-9]+)$'
    local heap_re='^(TINY|SMALL|LARGE) : (0x[0-9a-fA-F]+)$'
    local block_re='^(0x[0-9a-fA-F]+) - (0x[0-9a-fA-F]+) : ([0-9]+) bytes$'
    local total_re='^Total : ([0-9]+) bytes$'
    for size in $sizes; do counts[$size]=$(( ${counts[$size]:-0}+1 )); done
    while IFS= read -r line || [[ -n $line ]]; do
        if [[ $line =~ $expect_re ]]; then
            address=$((BASH_REMATCH[1])); size=${BASH_REMATCH[2]}
            [[ ! -v expected_blocks[$address] && -z $sizes ]] || return 1
            expected_blocks[$address]=$size
        elif [[ $line == '===== Show Allocated Memory =====' || $line == '=================================' ]]; then :
        elif [[ $line =~ ^Page\ size\ :\ [0-9]+\ bytes$ ]]; then :
        elif [[ $line =~ $heap_re ]]; then
            group=${BASH_REMATCH[1]}; address=$((BASH_REMATCH[2]))
            (( !seen_total && address>previous && address>=last_end )) || return 1
            previous=$address
        elif [[ $line =~ $block_re ]]; then
            start=$((BASH_REMATCH[1])); end=$((BASH_REMATCH[2])); size=${BASH_REMATCH[3]}
            [[ -n $group && ! -v actual_blocks[$start] ]] || return 1
            (( !seen_total && start>previous && start>=last_end && end-start==size )) || return 1
            if (( size<=4*PAGE/128 )); then wanted=TINY
            elif (( size<=32*PAGE/128 )); then wanted=SMALL
            else wanted=LARGE; fi
            [[ $group == "$wanted" ]] || return 1
            actual_blocks[$start]=$size
            if [[ -n $sizes ]]; then
                (( ${counts[$size]:-0}>0 )) || return 1
                counts[$size]=$(( ${counts[$size]}-1 ))
            else
                [[ ${expected_blocks[$start]:-missing} == "$size" ]] || return 1
            fi
            previous=$start; last_end=$end; sum=$((sum+size)); blocks=$((blocks+1))
        elif [[ $line =~ $total_re ]]; then
            (( !seen_total )) || return 1
            seen_total=1; total=${BASH_REMATCH[1]}
        else return 1; fi
    done < "$file"
    (( seen_total && blocks>0 && total==sum )) || return 1
    if [[ -n $sizes ]]; then
        for key in "${!counts[@]}"; do (( counts[$key]==0 )) || return 1; done
    else (( ${#expected_blocks[@]}==blocks )) || return 1; fi
}

status_exit() {
    jq -se 'all(.[]; .status!="FAIL" and (.status|startswith("UNVERIFIABLE")|not) and (.status|startswith("NOT RUN")|not))' "$1" >/dev/null
}

# ANSI colors are enabled by default; NO_COLOR keeps redirected output plain.
ui_colors() {
    blue='' green='' red='' reset=''
    if [[ ! -v NO_COLOR ]]; then
        blue=$'\033[34m'; green=$'\033[32m'; red=$'\033[31m'; reset=$'\033[0m'
    fi
}

render_view() {
    local command=$1 file=$2 view=$3 blue green red reset
    ui_colors
    jq -rs --arg command "$command" --arg view "$view" \
        --arg blue "$blue" --arg green "$green" --arg red "$red" --arg reset "$reset" '
      def description:
        {E01:"Check existing manual approval of the submission, allowed functions and globals, and designated source hashes.",
         E02:"Check library names, symbolic links and unnecessary rebuilds for each HOSTTYPE setting.",
         E03:"Check that the built library exports malloc, free, realloc and show_alloc_mem.",
         E04:(if $command=="test0" then "Measure the no-allocation baseline with system malloc and project malloc."
              else "Run the designated allocation workload with system malloc and project malloc to compare additional page usage." end),
         E05:"Check that one TINY/SMALL zone holds 100 simultaneous maximum-size requests and its size is a multiple of the OS page size.",
         E06:"Check that repeated allocation/free causes fewer minor faults than repeated allocation.",
         E07:"Check that repeated malloc/free adds at most 10 minor faults over the baseline.",
         E08:"Check output and normal termination for the designated realloc string and invalid-pointer cases.",
         E09:"Check that displayed classes, address order, ranges, sizes and totals match live allocations.",
         E10:"Check Linux ABI alignment for malloc/realloc and data preservation after resizing.",
         O01:"Check rejection of request-alignment overflow and preservation of existing data.",
         O02:"Check rejection of heap/block metadata addition overflow.",
         O03:"Check rejection of page-rounding overflow.",
         O04:"Check original data, ownership and safe free after realloc overflow failure.",
         pending:"Check whether the selected tests can run."}[.id] // .description;
      def result:
        if .status=="PASS" then $green + .status + $reset
        elif .status=="FAIL" then $red + .status + $reset
        else .status end;
      def colored_counts:
        tojson
        | gsub("\"PASS\""; "\"" + $green + "PASS" + $reset + "\"")
        | gsub("\"FAIL\""; "\"" + $red + "FAIL" + $reset + "\"");
      def description_line: $blue + "Description: " + description + $reset;
      def score:
        if .score_info != null then
          if .score_info.confirmed then "Score: \(.score_info.score)/5 — baseline-adjusted operational interpretation (measurement PASS does not imply full marks)"
          else "Score: UNCONFIRMED — candidate extra=\(.score_info.extra), calculation method awaiting confirmation" end
        else empty end;
      map(select($command!="test0" or .id!="E01")) as $rows |
      if $view=="rows" then
        $rows[] |
        if (.id|test("^(E|O)|^pending$")) then
          "\n\(description_line)\nLog: \(if .detail!="" then .detail else "Expected: normal termination and all check conditions satisfied" end)\nResult: \(result)",
          score,
          (if .status=="FAIL" then "Expected: \(.expected)\nActual: \(.actual)\nReproduce: \(.reproduce)\nLog file: \(.log)" else empty end)
        else
          "\n\(.id) \(.title)\n\(description_line)\nResult: \(result)",
          (if .status=="FAIL" then "Expected: \(.expected)\nActual: \(.actual)\nReproduce: \(.reproduce)\nLog: \(.log)" else empty end)
        end
      else
        (if any($rows[]; .id|test("^(E|O)|^pending$")) then "\nMandatory summary" else empty end),
        ($rows[] | select(.id|test("^(E|O)|^pending$")) | "\(description_line)\nResult: \(result)", score),
        ([ ["B","Bonus development checks"], ["C","Calloc bonus development checks"] ][] |
          . as $group | select(any($rows[]; .id|startswith($group[0]))) | "\($group[1]): \($rows | map(select(.id|startswith($group[0]))) | group_by(.status) | map({key:.[0].status,value:length}) | from_entries | colored_counts)")
      end
    ' "$file"
}

render_rows() { render_view "$1" "$2" rows; }
render_summary() { render_view "$1" "$2" summary; }
