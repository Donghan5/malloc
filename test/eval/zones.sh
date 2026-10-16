# Strict parser shared with runner fixtures; no fixture enters allocator scoring.
parse_zone() {
    local group=$1 file=$2
    awk -v group="$group" '
      NR==1 && NF==12 && $1==group {
        split("maximum zone page capacity same multiple data warmup heap_header block_header alignment", names, " ")
        valid=1
        for (i=2;i<=NF;i++) {
          if (split($i, pair, "=")!=2) valid=0
          if (pair[1]!=names[i-1] || pair[2]!~/^[0-9]+$/) valid=0
        }
      }
      END {exit !(valid && NR==1)}' "$file" || return 1
    jq -Rc 'split(" ") | .[1:] | map(split("=")|{key:.[0],value:(.[1]|tonumber)})|from_entries' "$file"
}

zone_evidence_valid() {
    jq -e '.maximum>0 and .page>0 and .zone>0 and .capacity>=100 and .same==100 and
      .multiple==1 and .data==1 and .alignment==16 and .maximum%16==0 and .zone%.page==0 and
      .capacity==(((.zone-.heap_header)/(.maximum+.block_header))|floor)' <<< "$1" >/dev/null
}

# Uses the existing command runner, evidence records and UI.
zone_checks() {
    local binary="$WORK/zones" built group code state=PASS detail='' line parsed
    : > "$WORK/zones.jsonl"
    local reason='' expected='Calculated capacity >=100; observed 100/100 in one zone; data preserved and non-overlapping; zone % page == 0'
    run_command gcc -Wall -Wextra -Werror -std=gnu11 -fno-builtin -I"$WORK/inc" \
        "$ROOT/test/spec/zones.c" "$ROOT/test/spec/zones_main.c" "$LIB" -pthread -o "$binary"
    built=$LAST_EXIT
    if [[ $built != 0 ]]; then
        check_result "$built" E05 'Pre-allocated zones' 'Zone probe build' 'Zone probe build failed'
        return
    fi
    for group in TINY SMALL; do
        run_command "$binary" "$group"; code=$LAST_EXIT
        line=$(cat "$LAST_OUT")
        if ! parsed=$(parse_zone "$group" "$LAST_OUT"); then
            if [[ $code == 77 && ! -s $LAST_OUT ]]; then
                [[ $state == FAIL ]] || state='UNVERIFIABLE — Valid sysconf page size or internal zone observation unavailable'
            else state=FAIL; fi
            detail+="$group: Missing or malformed probe output, exit=$code; "
            reason+="stdout=$line; exit=$code; stderr=$(cat "$LAST_ERR"); "
            continue
        fi
        jq -nc --arg group "$group" --argjson values "$parsed" --argjson exit "$code" \
            '$values + {group:$group,exit:$exit,
              capacity_formula:"floor((zone_size-sizeof(t_heap))/(aligned_maximum+sizeof(t_block)))",
              placement:"16-byte request alignment; fresh free block; split only if residual >= block header +16; small tail absorbed",
              source_paths:["inc/define.h","inc/struct.h","src/malloc.c","src/block/block.c","src/heap/heap.c","src/heap/helper_heap.c"]}' >> "$WORK/zones.jsonl"
        # Integer-valued, strict fields above; no address conversion via floating point.
        detail+="$group: $(jq -r '"max request=\(.maximum), zone=\(.zone), page=\(.page), calculated capacity=\(.capacity), same zone=\(.same)/100, page multiple=\(.multiple==1), data/non-overlap=\(.data==1), warmup allocations=\(.warmup)"' <<< "$parsed"); "
        if jq -e '.capacity<100 or (.page>0 and .zone>0 and .zone%.page!=0)' <<< "$parsed" >/dev/null; then
            state=FAIL
        elif [[ $code == 77 ]]; then
            [[ $state == FAIL ]] || state='UNVERIFIABLE — Fresh zone identification or single-zone membership unavailable'
        elif [[ $code != 0 ]] || ! zone_evidence_valid "$parsed"; then
            state=FAIL
        fi
        reason+="$line; exit=$code; stderr=$(cat "$LAST_ERR"); "
    done
    add_row E05 'Pre-allocated zones' 'Zone capacity and page multiple' "$state" "$detail" "$expected" "$reason"
}
