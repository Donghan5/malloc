# malloc 테스트

저장소 루트에서 실행한다. Linux/POSIX, GCC, make, pthread, GNU time,
GNU timeout, readelf, nm, awk, sha256sum이 필요하다. src/inc와 correction 원본은
테스트 정리 작업에서 수정하지 않는다. commit/push도 하지 않는다.

## 이전 분석에서 남긴 결론

- 전체 heap 초기화를 헤더 초기화로 제한한 뒤 관찰된 페이지 비용이 감소했다.
- 같은 프로세스에서 첫 malloc/write/free 이후 반복 중 추가 minor fault가 없었고,
  별도 tracing에서도 반복적인 allocator mapping 생성·해제는 관찰되지 않았다.
- correction 기반 free 품질은 +4/+5 등으로 간헐적으로 기준을 초과했다.
- 즉시 심볼 해석의 일관된 개선 효과는 확인되지 않았다. 해석 시점의 이동을
  fault 비용 감소로 단정할 수 없고 ELF 배치와 startup도 달라질 수 있다.
- 개별 실패 원인은 아직 미확정이다. 간소화가 실패를 해결했다고 주장하지 않는다.

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
sh test/integration/run.sh eval 5
make test-diagnostic-build
sh test/integration/run.sh binding
sh test/integration/run.sh diag repeat
sh test/integration/run.sh diag phases
```

평가와 진단은 같은 셸 실행 파일에서 명시적으로 선택한다. 기본은 평가 1묶음이다.
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
`repeat`는 binding 후 기준과 첫·100·1024회 malloc/전체 1024바이트 쓰기/free를
기록한다. `phases`는 같은 공통 준비·기록 경로의 baseline/free를 별도로 실행해
초기 기록, 기록 공간 준비, binding, 첫 malloc, payload, 첫 free, 100·1024회로 나눈다.
고정 크기 기록 공간을 미리 준비하고 모든 측정이 끝난 뒤 출력한다. malloc warm-up은 없다.

첫 getrusage와 기록 공간·코드·GOT·stack 접근 자체에도 비용이 있다. 초기 기록은
순수 main 진입 값이 아니며 이후 기록에 첫 snapshot 쓰기의 fault가 반영될 수도 있다.
main 이전 비용은 내부 기록만으로 직접 분리할 수 없다. 진단 출력도 마지막 내부
기록 이후에 fault를 만들 수 있다. 따라서 내부 진단 값을 GNU time의 전체 평가
수치와 직접 동일시하거나 단순 fault 수로 코드/heap/stack 원인을 확정하지 않는다.
추적 실행은 별도로 수행하고 비계측 평가 통계에 합치지 않는다.

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
