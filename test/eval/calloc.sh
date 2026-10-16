# Sourced by the existing runner: shares execution, evidence and UI helpers.
calloc_checks() {
    local binary="$WORK/calloc-spec" built id status ok mode output expected
    local -a descriptions=(
        'C01: Zero every requested byte across size pairs and TINY/SMALL/LARGE boundaries; write and free'
        'C02: Reuse the guarded dirty block at its actual address and zero all requested bytes'
        'C03: Reject runtime multiplication wrapping to zero or a small value; recover normally'
        'C04: Fail valid multiplication under verified child RLIMIT_AS and recover normally'
        'C05: Match malloc(0) policy for all three zero-size combinations; safely free'
        'C06: Fresh-process debug arguments/address and final zero bytes despite scribble'
        'C07: Synchronized finite threads check zeros, owned data and deadlocks'
    )
    run_command gcc -Wall -Wextra -Werror -std=gnu11 -fno-builtin -I"$WORK/inc" \
        "$ROOT/test/spec/calloc_main.c" "$ROOT/test/spec/calloc_common.c" \
        "$ROOT/test/spec/calloc_failure.c" "$ROOT/test/spec/calloc_threads.c" "$LIB" -pthread -o "$binary"
    built=$LAST_EXIT
    for id in 1 2 3 4 5 6 7; do
        ok=$built
        if [[ $built == 0 ]]; then
            run_command "$binary" "C0$id"; ok=$LAST_EXIT
            if [[ $id == 6 ]]; then
                output=$(cat "$LAST_OUT")
                [[ $output =~ ^EXPECT\ calloc\(7,\ 13\)\ =\ 0x[0-9a-f]+$ ]] || ok=1
                for mode in debug scribble both; do
                    RUN_ENV=()
                    [[ $mode == scribble ]] || RUN_ENV+=(MALLOC_DEBUG=1)
                    [[ $mode == debug ]] || RUN_ENV+=(MALLOC_SCRIBBLE=1)
                    run_command "$binary" C06; RUN_ENV=()
                    [[ $LAST_EXIT == 0 ]] || ok=$LAST_EXIT
                    expected=$(sed -n 's/^EXPECT /[MALLOC] /p' "$LAST_OUT")
                    [[ $expected =~ ^\[MALLOC\]\ calloc\(7,\ 13\)\ =\ 0x[0-9a-f]+$ ]] || ok=1
                    if [[ $mode == scribble ]]; then
                        rg -q '\[MALLOC\]' "$LAST_OUT" && ok=1
                    else
                        [[ $(grep -Fxc "$expected" "$LAST_OUT") == 1 ]] || ok=1
                        [[ $(grep -c 'calloc(' "$LAST_OUT") == 2 ]] || ok=1
                    fi
                done
            elif [[ $id == 3 ]]; then
                RUN_ENV=(MALLOC_DEBUG=1)
                run_command "$binary" C03; RUN_ENV=()
                [[ $LAST_EXIT == 0 ]] || ok=$LAST_EXIT
                # Overflow calls must not enter public malloc, even for wrap=0.
                [[ $(grep -c ' bytes$' "$LAST_OUT") == 1 ]] || ok=1
                [[ $(grep -c ' : 91 bytes$' "$LAST_OUT") == 1 ]] || ok=1
                [[ $(grep -c 'calloc(7, 13)' "$LAST_OUT") == 1 ]] || ok=1
            elif [[ $id == 7 ]]; then
                RUN_ENV=(MALLOC_DEBUG=1)
                run_command "$binary" C07-debug; RUN_ENV=()
                [[ $LAST_EXIT == 0 ]] || ok=$LAST_EXIT
            fi
        fi
        status=FAIL
        [[ $ok != 0 ]] || status=PASS
        [[ $ok != 77 ]] || status='UNVERIFIABLE — reuse not obtained or resource/thread setup unavailable (exit 77)'
        add_row "C0$id" 'calloc development check' "${descriptions[id-1]}" "$status" \
            'No automatic bonus score. C07 does not prove race freedom. Raw output per invocation: evidence.json records.' \
            'All conditions satisfied and exit 0; overflow and limited requests return NULL' \
            "exit=$ok; final stdout=$(cat "$LAST_OUT"); final stderr=$(cat "$LAST_ERR")"
    done
    calloc_binding "$binary" "$built"
}

calloc_binding() {
    local binary=$1 ok=$2 status output
    if ! command -v python3 >/dev/null || ! command -v objdump >/dev/null; then
        add_row C08 'calloc export and binding' 'Submission ELF and separate binding/disassembly diagnostics' \
            'UNVERIFIABLE — python3 or objdump unavailable'
        return
    fi
    run_command nm -D --defined-only "$LIB"
    [[ $LAST_EXIT == 0 ]] && awk '$2=="T" && $3=="calloc" {n++} END {exit n!=1}' "$LAST_OUT" || ok=1
    run_command objdump -d "$LIB"
    if [[ $LAST_EXIT != 0 ]]; then
        add_row C08 'calloc export and binding' 'Submission ELF and separate binding/disassembly diagnostics' 'UNVERIFIABLE — objdump unavailable'
        return
    fi
    cp "$LAST_OUT" "$WORK/disassembly"
    cp "$LAST_OUT" "$RESULT_DIR/calloc-disassembly.txt"
    # Inspect the submitted O0 code: calloc -> malloc, debug, ft_memset;
    # malloc/start_malloc/ft_memset must not call calloc (direct recursion either).
    cat > "$WORK/calloc-disassembly.py" <<'PY'
import re, sys
text = open(sys.argv[1]).read()
functions = dict(re.findall(r'^[0-9a-f]+ <([^>]+)>:\n(.*?)(?=^[0-9a-f]+ <|\Z)', text, re.M | re.S))
seen = set()
def inspect(name):
    if name in seen:
        return
    seen.add(name)
    body = functions.get(name)
    if body is None:
        raise ValueError('missing disassembly: ' + name)
    for target in re.findall(r'\b(?:call\w*|jmp\w*)\s+[0-9a-f]+ <([^>]+)>', body):
        if '+' in target:
            continue
        target = target.removesuffix('@plt')
        if target == 'calloc':
            raise ValueError('recursive calloc path from ' + name)
        if target in functions and '@' not in target:
            inspect(target)
inspect('calloc')
assert re.search(r'call.*<malloc@plt>', functions['calloc'])
assert re.search(r'call.*<ft_memset@plt>', functions['calloc'])
print('Reviewed reachable local call graph:', ', '.join(sorted(seen)))
PY
    cp "$WORK/calloc-disassembly.py" "$RESULT_DIR/"
    run_command python3 "$WORK/calloc-disassembly.py" "$WORK/disassembly"
    [[ $LAST_EXIT == 0 ]] || ok=1
    RUN_ENV=(LD_DEBUG=bindings)
    run_command "$binary" C01; RUN_ENV=()
    [[ $LAST_EXIT == 0 ]] || ok=1
    output=$LAST_ERR
    cat > "$WORK/calloc-binding.py" <<'PY'
import re, sys
text = open(sys.argv[1]).read()
pattern = r'binding file ' + re.escape(sys.argv[3]) + r' .* to ' + re.escape(sys.argv[2]) + r' .*symbol .calloc'
assert re.search(pattern, text), 'project calloc binding not found'
PY
    cp "$WORK/calloc-binding.py" "$RESULT_DIR/"
    run_command python3 "$WORK/calloc-binding.py" "$output" "$LIB" "$binary"
    [[ $LAST_EXIT == 0 ]] || ok=1
    status=FAIL; [[ $ok != 0 ]] || status=PASS
    add_row C08 'calloc export and binding' 'Submission exports, project calloc binding and recursive optimization check; separate from quality measurements' "$status" \
        "Library: $LIB; SHA256: $LIB_HASH; allocator flags unchanged; source lock path reviewed." \
        'Defined public calloc, executable binds to project ELF; no reachable local calloc recursion' "diagnostic exit=$ok"
}
