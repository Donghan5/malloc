# malloc 테스트

저장소 루트에서 실행한다. Linux/POSIX, GCC, make, pthread, GNU time,
GNU timeout, readelf, nm, awk, sha256sum이 필요하다. src/inc와 correction 원본은
테스트 정리 작업에서 수정하지 않는다. commit/push도 하지 않는다.

## 이전 분석에서 남긴 결론

- 전체 heap 초기화를 헤더 초기화로 제한한 뒤 관찰된 페이지 비용이 감소했다.
- 같은 프로세스에서 첫 malloc/write/free 이후 반복 중 추가 minor fault가 없었고,
  별도 tracing에서도 반복적인 allocator mapping 생성·해제는 관찰되지 않았다.
- correction 기반 free 품질은 +4/+5 등으로 간헐적으로 기준을 초과했다.
  공식 순서 200회 측정에서 184회 통과, 16회 실패였다(아래 "free 품질 실패율 측정").
- 즉시 심볼 해석의 일관된 개선 효과는 확인되지 않았다. 해석 시점의 이동을
  fault 비용 감소로 단정할 수 없고 ELF 배치와 startup도 달라질 수 있다.
- 개별 실패 원인은 아직 미확정이다. 간소화가 실패를 해결했다고 주장하지 않는다.

이후 보존했던 perf 실패 쌍은 GNU time test0=75, test2=80(차이 +5),
비교용 쌍은 76/79(+3)이었다. 사용한 라이브러리 SHA-256은
`4701e42a7af8c14d76ae0eeefb808929fd5abeafecb869d85db753e7640316d4`다.
실패 쌍의 자식 사용자 모드 표본은 69/73(+4)이며 exec 전 준비 +1,
로더·초기화 +2, malloc +1로 구성됐다. malloc의 heap 헤더 초기화 쓰기
(`create_new_heap → ft_memset+0x2f`) 1건은 비교용과 같았고 free·종료 순증가는 0이었다.
시작·로더 변동은 관측됐지만 GNU time과 표본 차이 사이의 잔여 +1은 미해소다.
kernel 제외 필터만으로 잔여 원인을 확정하지 않았으며, 후속 접근 주소 추적은
손실 기록 때문에 INVALID였다. 그 손실 기록의 heap 첫 페이지 접근은 정성적 관찰로만
남긴다. 이 근거로 allocator 수정이 필요하다고 판단할 수 없다.

잔여 +1을 다시 확인하려면 kernel 모드 집계가 필요하지만 현재 환경은
`kernel.perf_event_paranoid=2`이고 perf capability·tracefs 접근이 없다.
`-e minor-faults:uk`는 **종료 코드 0으로 성공처럼 보이면서** 헤더가
`minor-faults:uku`, `exclude_kernel=1`로 조용히 강등되므로, 헤더를 확인하지 않으면
커널 모드를 측정했다고 오판한다. capability bounding set도 0이어서 파일 capability
설정만으로 해결된다고 보장할 수 없다.

위 결론을 확인한 뒤 과거 perf 원본·출력·분석 문서를 삭제했다. 별도 보관본은 없으며,
따라서 과거 원시 자료를 다시 집계하거나 검증할 수 없다. 이번 내부 진단은
그 잔여 +1을 소급 설명하지 않는다.

누적 실험 파일과 원시 결과는 이 결론을 정리한 뒤 삭제한다. 별도 보관본이나
자동 결과 디렉터리를 만들지 않는다. 이후 결과는 기본적으로 터미널에 출력하며,
사용자가 보존을 원할 때만 직접 리다이렉션한다.

## 주요 명령

| 명령 | 실행 범위 |
|---|---|
| `make test-mandatory` | 필수 회귀 검사, 일반/debug·scribble 모드 |
| `make test-bonus` | 병합·환경변수·hex dump·동시성 회귀 검사 |
| `make test-eval` | correction 기반 페이지 평가 1묶음 |
| `make test` | 필수·보너스·빌드 계약·페이지 평가, 전체 실패 집계 |
| `make test-build` | 상시 검사 바이너리와 현재 라이브러리 빌드 |
| `make test-diagnostic-build` | 명시적 보조 진단용 C 바이너리 빌드 |
| `make test-clean` / `make test-fclean` | 테스트 객체 / 테스트 바이너리 정리 |

```sh
make test-eval REPEATS=5
make test-free-quality REPEATS=30
sh test/integration/run.sh eval 5
make test-diagnostic-build
sh test/integration/run.sh binding
sh test/integration/run.sh diag cold
sh test/integration/run.sh diag repeat
sh test/integration/run.sh diag phases
sh test/integration/run.sh diag fullwrite
```

평가와 진단은 같은 셸 실행 파일에서 명시적으로 선택한다. 기본은 평가 1묶음이다.
`make test-free-quality`는 `test-eval`의 별칭이다. 변경된 소스로 라이브러리를
빌드하고 correction test0 → test1 → test2의 원시 수치와 개별 판정을 출력한다.
기본 1묶음이며 `REPEATS=30`처럼 반복 횟수를 지정한다. free 품질뿐 아니라
실행·할당·free 기능 기준도 유지하고, 어느 기준이든 실패하면 make도 실패한다.
보조 진단은 `make test`에서 실행하지 않는다. 실행기 all_cases는 격리·출력 수집·
집계만 담당하며 테스트 구현이나 점수 계산을 포함하지 않는다.

## 검증 범위

| 경로 | 역할 |
|---|---|
| mandatory/basic.c | NULL/0·정렬·overflow, realloc 성장/실패 보존, LARGE 축소 후 서로 다른 free 순서의 수명 독립성, split 최소 경계, 단편화·새 heap·unlink |
| mandatory/zones_output.c | zone 용량·페이지 geometry, 단편화·주소순 출력·payload/Total·memmove |
| mandatory/heap_edges.c | TINY/SMALL 재사용, LARGE 회수, mapping 실패 뒤 복구 |
| mandatory/reclamation.c | 첫/끝 metadata, heap 회수·카운터 |
| bonus/coalescing*.c | 연속 free·앞/뒤/양쪽 병합·연속 축소, metadata·카운터·재사용 |
| bonus/diagnostics*.c | 정확한 환경변수 값·캐싱, debug·scribble, hex dump·freed hole 제외 |
| bonus/concurrency*.c | 동시 접근·초기화 경쟁·소유권 전달·worker 실패 전파·실패 후 mutex 해제 |
| correction/ | 평가용 원본 보존, 대표 페이지 평가에는 test0~2만 사용 |
| integration/ | 빌드 계약·평가 호출·보조 진단 C 파일 하나 |
| all_cases/, helpers/, ui/ | 실행·격리·집계 및 공통 fixture·검증·출력 |

단위 검사만 allocator 심볼을 edge_*로 바꿔 소스와 링크한다. 공통 helpers/ui와
기존 필수·보너스 기능을 보존한다. 초기화 상태 리셋도 테스트 fixture 안에서만 한다.

## correction 페이지 평가

측정 바이너리는 원본 test0~2를 `-O0 -g -fno-builtin`으로 컴파일하고 libc에만
링크한다. 사용자 allocator는 직접 링크하지 않는다.
별도 binding 실행에서 correction의 malloc/free와 time의 실제 주입을 확인하며,
평가 원본에는 getrusage·기록·binding 코드를 삽입하지 않는다.

각 묶음은 test0 → test1 → test2 순서다. MALLOC_DEBUG/MALLOC_SCRIBBLE 및
binding 실험용 환경변수를 제거하고 절대경로 LD_PRELOAD/LD_LIBRARY_PATH를 사용한다.

```sh
env -u MALLOC_DEBUG -u MALLOC_SCRIBBLE \
  LD_LIBRARY_PATH="$PWD" LD_PRELOAD="$PWD/libft_malloc.so" \
  /usr/bin/time -v ./test/bin/eval/testN
```

평가 시트 방식처럼 **time에도 라이브러리를 주입**한다. time을 주입 밖에 두는
측정과 결과를 섞지 않는다. /usr/bin/time이 없거나 GNU time 확인에 실패하면
명확하게 실패하며 다른 방법으로 대체하지 않는다. 숫자 필드 파싱을 위해 LC_ALL=C를
사용한다. 드라이버 자체에는 LD_PRELOAD를 적용하지 않는다.

원시 minor faults와 baseline 차이를 반복별로 출력하며 판정은 다음과 같다.

- 할당 만점: test1 − test0가 255~272.
- free 품질: test2 − test0가 최대 3.
- free 기능: test2 < test1.

실행 성공과 세 기준을 별도로 표시한다. 음수를 보정하거나 평균으로 실패를
숨기지 않는다. crash·timeout·필드 누락/파싱 실패·점수 실패는 비정상 종료로 전파한다.
각 실행은 15초 timeout 뒤 필요하면 SIGKILL로 종료된다. 임시 출력 파일은 종료·
오류·인터럽트 시 삭제한다. stdout/time 출력은 터미널에서 확인할 수 있다.
correction 파일이 공식 첨부본과 동일하다는 출처는 아직 미확정이다.

## 보조 진단의 한계

C 진단은 주입된 malloc/free의 실제 함수 주소와 dladdr 파일 device/inode를 확인한다.
`cold`는 명시적 allocator warm-up 없이 첫 malloc(1024) → 첫 바이트에 42 쓰기 →
유효한 포인터의 free를 각각 snapshot 전후 차이로 기록한다. 인접 구간은 경계 snapshot을
공유한다. `repeat`는 동일한 첫 구간을 기록한 뒤 누적 100·1024회까지의 증가량을
추가 기록한다. 할당 크기·반복 수·쓰기 범위는 correction test2에 맞췄지만,
계측 코드를 포함하는 별도 로컬 진단이며 공식 평가를 대체하지 않는다.
`fullwrite`만 전체 1024바이트를 쓰는 별도 진단이다. `phases`는 기존 호환 명령으로
공통 준비·기록 경로의 baseline/free를 별도 프로세스에서 실행하며 같은 프로세스 비교가 아니다.

첫 구간 전과 최종 구간 뒤에 연속 snapshot만 실행하는 빈 구간을 둔다. 고정 크기
기록 공간을 미리 쓰고 출력은 마지막 snapshot 이후에만 수행한다. free 경로에 포인터
배열이나 진단용 동적 할당은 없다. binding 확인은 첫 측정 malloc 전에 수행되므로
그 과정에서 코드·로더 상태가 달라질 수 있다. 여기서 cold는 첫 명시적 malloc을 뜻하며,
main 이전 로더 내부의 할당까지 없었다고 보증하지 않는다.

원본 빌드 산출물을 보존하면서 실행하려면 아래처럼 /tmp의 별도 진단 바이너리를 사용한다.
`FAULT_PROBE` 재지정은 `diag`에만 적용된다. 평가 경로와 임계값은 변경하지 않는다.

```sh
temporary=$(mktemp -d /tmp/malloc-probe.XXXXXX)
trap 'rm -rf "$temporary"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
cc -std=gnu11 -Wall -Wextra -Werror -O0 -g -fno-builtin -fPIE -pie \
  test/integration/fault_probe.c -ldl -o "$temporary/fault_probe"
FAULT_PROBE="$temporary/fault_probe" sh test/integration/run.sh diag cold
FAULT_PROBE="$temporary/fault_probe" sh test/integration/run.sh diag repeat
```

첫 getrusage와 기록 공간·코드·GOT·stack 접근 자체에도 비용이 있다. 초기 기록은
순수 main 진입 값이 아니며 이후 기록에 첫 snapshot 쓰기의 fault가 반영될 수도 있다.
main 이전 비용은 내부 기록만으로 직접 분리할 수 없다. 진단 출력도 마지막 내부
기록 이후에 fault를 만들 수 있다. 따라서 내부 진단 값을 GNU time의 전체 평가
수치와 직접 동일시하거나 단순 fault 수로 코드/heap/stack 원인을 확정하지 않는다.
추적 실행은 별도로 수행하고 비계측 평가 통계에 합치지 않는다.

## 같은 프로세스 구간 진단 결과

현재 원본 라이브러리(위 SHA-256)로 독립 프로세스 총 10회, cold 5회·repeat 5회를
실행했다. perf/LD_DEBUG를 사용하지 않았고 debug·scribble·LD_BIND_NOW를 제거했다.
10회 모두 binding 확인·실행이 성공했고 종료 코드는 0이었다. 원시 snapshot과
증가량은 터미널에 출력했으며 영구 로그는 만들지 않았다.

| 구간 증가량 | cold 5회 | repeat 5회 |
|---|---:|---:|
| 초기 snapshot → 기록 공간 준비 | 매번 +2 | 매번 +2 |
| 준비 → binding 완료 | +0~1 | +0~1 |
| 측정 전 빈 구간 | 매번 0 | 매번 0 |
| 첫 malloc | 매번 +1 | 매번 +1 |
| 첫 바이트 쓰기 / 첫 free | 매번 0 / 0 | 매번 0 / 0 |
| 2~100회 / 101~1024회 | 미측정 | 매번 0 / 0 |
| 측정 후 빈 구간 | 매번 0 | 매번 0 |

별도로 모드별 10회씩(`free`/공식 1바이트 쓰기, `fullwrite`/1024바이트 전체 쓰기,
`baseline`/할당 없음) 추가 실행한 표본에서는 첫 malloc이 `free` 8/10회 +1,
**2/10회 +2**였고 `fullwrite`는 10/10회 +1이었다. 즉 첫 malloc 비용은 항상 +1이
아니라 +1~+2로 흔들린다. 1바이트 쓰기와 1024바이트 전체 쓰기의 fault 수가 같은 것은
payload가 heap 헤더 memset으로 이미 touch 된 첫 페이지 안에 들어가기 때문이다.
`baseline`은 모든 측정 구간이 0이었다.

첫 할당 비용과 이후 반복 비용은 위와 같이 구분됐다. 반복 중 free를 포함한
전체 구간에서 추가 fault가 없었지만 correctness 전체나 병합·heap 회수 코드의
실행 여부를 입증하지 않는다. 빈 구간 0도 getrusage·기록 비용의 완전한 제거를
뜻하지 않는다. main 이전 startup과 binding 비용은 첫 malloc의 내부 차이에
포함되지 않지만 진단 조건에 영향을 준다. 공식 test2−test0를 대체하거나
만점·매번 통과를 보장할 수 없고, free 소스 수정 근거는 확보되지 않았다.

## free 품질 실패율 측정

공식 순서 `test0 → test1 → test2`로 200회 측정했다. free 품질은 **184회 통과,
16회 실패(초과율 8.0%)**였고 **할당 기준(255~272) 이탈은 0회**였다.
free 차이 범위는 −3 ~ +5, 평균 1.08이었다. 영구 로그는 남기지 않았으므로
재검증에는 재측정이 필요하다. 한 머신·한 세션의 스냅샷이다.

축약 순서 `test0 → test2` 200회에서는 16/200, 평균 1.11이었다.
**이번 표본에서 두 순서의 실패 비율이 같고 평균 차이가 가까웠다.**
두 순서의 동등성을 검증한 실험은 아니다.

**lag-1 자기상관(+0.059 / −0.069)과 전후반 실패 건수(8/8, 7/9)에서 뚜렷한 이상을
발견하지 못했다.** 지연 1의 무상관은 독립성을 입증하지 않는다. 8.0%에 대한
Wilson 95% 구간 [5.0%, 12.6%]는 **독립적인 이항 시행을 가정한 참고값**이다.

첫 할당 비용(같은 프로세스에서 1건)과 프로세스 시작 비용의 변동이 실패에
기여한다는 설명은 관측과 부합한다. **그러나 모든 실패의 원인은 미확정이다.**

SMALL 경계의 빈 heap 캐싱은 별도 성능 개선 후보이며 적용·회귀 검증되지 않았다.
**correction의 test2는 마지막 SMALL heap을 이미 재사용하므로 이 캐싱은 test2
경로를 개선하지 않는다.** LARGE의 회수 반복은 제출 정책의 성능 비용이며
그 자체로 결함이 아니다. LARGE 캐싱으로 확대하지 않는다.

**제출용 allocator(src/inc)는 변경하지 않았고, 매번 만점 확보는 검증되지 않았다.**

### 철회한 주장

- **"구조적 실패율 6.5%"** — 20+20 관측을 교차한 400쌍은 독립 400회 실험이 아니다.
  위 200회 측정으로 대체한다.
- **"SMALL 캐싱 적용 시 8.0% → 2.5%"** — 2.5%는 다른 변경(heap 첫 페이지
  first-touch 1건 제거)의 가상 산술이며 캐싱의 효과가 아니다.
- **"LARGE의 회수 반복은 free 결함"** — 설계대로의 정책 비용이다.
- **"모든 실패가 시작 비용 때문", "allocator 수정으로 없앨 수 없다",
  "생성자 pre-touch가 유일한 방법"** — 인과 확정이 아니며 대안을 시험하지 않았다.

## 간소화 후 검증

필수는 58/58, 보너스는 23/23 통과했다. 보너스의 중복 split 경계 검사 하나를
필수 검사로 합쳤으며 TINY/SMALL 재사용은 공통 구현으로 통합했다. LARGE 축소
수명 독립성, realloc 실패 보존, 병합 metadata·카운터, 단편화·새 heap, 동시 접근과
실패 후 mutex 해제 검사는 유지했다.

correction 평가 5묶음은 5/5 통과했고 음수 차이도 그대로 출력했다.
별도 `make test`에서는 free 차이 +4로 페이지 평가가 실패하여 **82/83 통과,
make 종료 코드 2**였다. 빌드 계약 내부 6/6은 통과했다. 이 결과를 성공한 재실행으로
대체하지 않았다. 반복 진단은 첫 구간 +1, 이후 1023회 추가 증가 0이었고,
구간 진단은 free의 첫 malloc에서 +1, payload/free/이후 반복은 +0이었다.
진단의 절대 값과 평가 값을 합산하거나 간소화가 실패를 해결했다고 해석하지 않는다.

주입 없음·잘못된 라이브러리·time 부재는 실패했고, /tmp의 별도 검증 fixture로
exit 7·SIGSEGV·timeout·파싱 오류·점수 실패의 비정상 종료 전파를 확인했다.
점수 실패 fixture도 실행 PASS/할당 PASS/free 품질 FAIL을 구분했다.
SIGTERM 인터럽트에서 자식 종료·회수와 임시 파일 삭제도 확인했다. 검증용 fixture는
원본 평가 바이너리나 라이브러리를 덮어쓰지 않았고 종료 후 삭제했다.

삭제한 도구·파일 참조, 테스트 내부 Python 및 누적 실험 디렉터리는 남아 있지 않다.
src/inc/correction의 작업 전후 SHA-256이 같고, 현재 공유 라이브러리도 보존됐다.
`git diff --check`와 셸 문법 검사가 통과했다. 원시 로그를 새 보관 디렉터리에
옮기지 않았다. 다음 조사에서 필요한 최소 증거는 실패한 한 묶음의 원시
baseline/hold/free·차이·종료 코드, 사용한 라이브러리 해시와 별도 진단 출력이다.
영구 보존이나 fault 위치 추적은 사용자가 요청한 경우에만 별도로 진행한다.
개별 free 품질 실패의 원인은 여전히 미확정이다.
