# malloc: 필수 결함 수정 및 전체 보너스 완료 마일스톤

- 작성일: 2026-10-02 (Europe/Paris)
- 과제 기준: 첨부 `malloc.pdf`, Version 6.3, Chapter II–IV
- 리뷰 기준: `Donghan5/malloc`, `main@5b89b15d8b71c742b98e4775bab180e579edb5c6`
- 저장소: https://github.com/Donghan5/malloc/tree/5b89b15d8b71c742b98e4775bab180e579edb5c6
- 검증 환경: 이전 리뷰에서 Linux x86_64, 페이지 크기 4096으로 빌드·실행
- 갱신일: 2026-10-07 (Europe/Paris)
- 최신 검증: HEAD `3c106d5910d1124fe0ec1103eaeb3fad8b1e8bcd` + 미커밋 수정 사항에서 `make test-m1`, 종료 코드 0, **30/30 통과** (실패 0).
- 최신 실행 환경: Fedora Linux 44, x86_64, 페이지 크기 4096. 학교 환경도 Fedora이므로 Linux/Fedora를 M1 대상 플랫폼으로 삼는다. 필수 16개와 M1 실행 9개, 소스 점검 2개, 빌드 점검 3개를 포함한다.
- 이 문서는 수정 계획과 검증 기록이다. 필수 회귀 테스트 통과가 전체 필수 요구사항이나 보너스 완료를 의미하지는 않는다.
- 현재 판정: **INCOMPLETE**

## 1. 목표와 평가 조건

목표는 필수 구현의 확인된 결함을 제거하고, PDF에 명시된 보너스 네 항목을 모두 설명·검증 가능한 상태로 만드는 것이다.

PDF는 첫 보너스로 pthread 기반 thread safety를 제시하고, 추가 기능으로 디버그 환경변수, `show_alloc_mem_ex()`, 해제 공간 defragmentation을 예시한다. 추가 기능 목록은 비 exhaustive 목록이다. 네 항목을 구현했다고 최대 점수가 자동 보장되지는 않는다.

**평가 선행 조건:** 필수 구현이 완벽해야 보너스를 평가한다. 기존 LARGE 축소 후 SIGSEGV 반례는 최신 회귀 테스트에서 통과했다. 나머지 필수 계약과 보너스 검증이 남아 있으므로 보너스 평가 가능 상태는 아직 확정하지 않는다.

| 항목 | 현재 확인된 상태 | 목표 |
| --- | --- | --- |
| pthread 기반 thread safety | 공개 allocator·조회 함수에 mutex 존재. 기존 4스레드 테스트 실행 완료 | 상태 변경·조회 경로의 잠금 계약 확인 및 실패를 검출하는 동시성 테스트 |
| 디버그 환경변수 | MALLOC_DEBUG 로그, MALLOC_SCRIBBLE의 할당 시 0xaa 확인 | 변수 해석·적용 범위를 명시하고 malloc/free/realloc 경로별 검증 |
| show_alloc_mem_ex() | hex dump 출력 확인 | 일반 조회의 정확성을 공유하고 알려진 바이트 패턴으로 검증 |
| 해제 공간 defragmentation | 일반 양방향 병합 통과. realloc 축소 후 병합 불완전 | 인접 free 블록이 남지 않고 병합 공간을 재사용 |
| 필수 구현 | 기존 LARGE 수명 오류·group 격리 회귀 통과, 필수 테스트 16/16 통과 | 유효한 호출의 데이터·수명·분류·출력 계약 충족 |

## 2. 확인된 결함과 수정 상태

증거 구분:
- **실행 확인:** 이전 리뷰의 별도 반례에서 재현.
- **코드 확인:** 소스 흐름에서 확인. 특정 실행 실패를 재현했다는 뜻은 아님.
- **미검증:** 테스트나 근거가 아직 없음.

| 우선순위 | 문제 | 증거 | 위치 | 필요한 수정 범위 |
| --- | --- | --- | --- | --- |
| 수정 확인 (기존 CRITICAL) | LARGE를 축소한 뒤 그 잔여 공간에 다른 할당이 들어가며, 첫 할당 free가 두 번째 할당까지 해제 | 이전 SIGSEGV 재현 → 최신 양쪽 free 순서 회귀 통과 | src/realloc.c, src/block/block.c, src/free.c, src/heap/heap.c | LARGE 축소 시 분할하지 않고 기존 mapping 유지. 별도 할당 수명 보존 확인 |
| 수정 확인 (기존 HIGH) | 요청 크기와 heap 종류가 일치하지 않음 | 이전 SMALL 오배치 → 최신 size class isolation 통과 | find_free_block(), start_malloc() | find_free_block()에서 요청 group과 다른 heap을 건너뛰도록 적용 |
| HIGH | realloc 축소 이후 인접 free 블록이 남음 | 실행 확인 | split_block(), start_realloc(), coalesce_block() | 축소 잔여 블록 병합 및 block_count/free_size 유지 |
| 수정 확인 (기존 HIGH) | show_alloc_mem()이 주소 오름차순을 보장하지 않음 | 강제로 역순 연결한 LARGE heap 출력·Total 회귀 통과 | src/tools/show_alloc_mem.c | next_heap_by_address()로 출력 순서 선택. 혼합 group·block 순서, 해제 블록 제외, 빈 상태·Total 회귀 추가 통과 |
| 수정 확인 (기존 HIGH) | 서로 다른 할당의 포인터 차를 계산함 | d - s 소스 점검 및 memmove 기능 회귀 통과 | src/tools/tools.c | uintptr_t 정수 연산으로 변경. 소스 패턴 통과는 C 이식성 전체의 증명이 아님 |
| 수정 확인 (기존 MEDIUM) | 총 free_size만으로 heap을 선택한 뒤 첫 블록만 검사하는 fallback 경로 | 같은 group 내 연속 공간 부족 회귀 통과 | get_available_heap(), start_malloc() | start_malloc()에서 적합한 free 블록이 없으면 새 heap 생성. 기존 fallback 경로 우회 |
| MEDIUM | 멀티스레드 데이터 손상을 최종 실패 판정에 반영하지 않음 | 코드 확인 | main.c의 thread_routine(), test_multithread() | worker 결과 수집, 생성 성공 스레드만 join, 실패 exit code |
| 검증 필요 | 재할당 후 메타데이터·빈 heap 회수·실패 시 원본 보존의 전체 계약 | 일부 테스트만 실행됨 | allocator 전반 | 아래 회귀 테스트로 확정 |
| Linux 점검 통과 | Linux 페이지 API·빌드 재실행·헤더 의존성 | getpagesize() 소스 점검, 페이지 출력, HOSTTYPE fallback·symlink·재실행·define.h rebuild 통과 | inc/define.h, helper_heap.c, show_alloc_mem.c, Makefile | 다른 OS·전체 허용 함수 계약은 미검증 |

현재 `Page size` 출력은 `show_alloc_mem()`과 `show_alloc_mem_ex()`에 존재한다. “출력 없음”을 현재 결함으로 다시 기록하지 않는다.

이전 평가의 “Crash on buffer overflow”가 이번 SIGSEGV와 같은 원인인지는 **검증 불가**다. 이번 반례는 사용자 buffer overflow 없이 유효한 malloc/realloc/free 호출만으로 발생한다. 임의의 사용자 OOB 쓰기를 모두 복구하는 기능을 PDF 요구사항으로 추가하지 않는다.

## 3. 마일스톤 순서

| 순서 | 마일스톤 | 완료를 막는 핵심 조건 |
| --- | --- | --- |
| M0 | 메모리 수명과 영역 분류 수정 | 살아 있는 다른 할당을 munmap하거나 다른 group에서 재사용하면 실패 |
| M1 | 필수 allocator 계약 및 출력 정리 | 데이터 보존·연속 공간 선택·정렬·주소 순서가 틀리면 실패 |
| M2 | defragmentation 완료 | 축소·해제 뒤 인접 free 블록이 남으면 실패 |
| M3 | 디버그 환경변수와 확장 조회 검증 | 명시한 동작과 로그·바이트가 다르면 실패 |
| M4 | thread safety 검증과 테스트 판정 수정 | 손상·hang·실패가 PASS로 처리되면 실패 |
| M5 | 통합 회귀 및 제출 게이트 | 필수 실패가 하나라도 남으면 보너스 완료 판정 금지 |

### M0. CRITICAL 수명 오류와 group 분류 수정

**상태:** 완료 — 아래 수명·분류 회귀는 `make test`에서 통과. LARGE 축소는 기존 블록을 분할하지 않아 mapping 안에 다른 독립 할당을 만들지 않는다.

**범위:** `src/malloc.c`, `src/realloc.c`, `src/block/block.c`, `src/free.c`, 필요 시 `src/heap/heap.c`.

- [x] LARGE mapping 하나에 독립된 다른 할당이 들어가지 않도록 정책을 정한다.
- [x] LARGE realloc 축소 시 잔여 공간의 처리와 free 정책을 일치시킨다.
- [x] TINY/SMALL free 블록 검색에서 요청 크기에 대응하는 group만 선택한다.
- [x] free가 해제하는 mapping과 그 안의 살아 있는 블록 관계를 명시한다.
- [x] 다음 반례를 독립 프로세스의 회귀 테스트로 추가한다.

```c
char *a = malloc(4096);
a = realloc(a, 2048);
char *b = malloc(64);
b[0] = 0x77;
free(a);
/* 여기서 b[0]은 여전히 0x77이어야 한다. */
free(b);
```

이 코드는 실패 재현 순서이며 완성된 테스트 harness가 아니다. 실제 테스트는 NULL을 확인하고 realloc 결과를 임시 포인터로 받아 실패 시 원본을 보존한다.

**완료 조건:**
- [x] 위 순서를 반복해도 SIGSEGV·데이터 손상이 없다.
- [x] b를 먼저 free하는 순서도 안전하다.
- [x] malloc(512) 뒤 malloc(64)가 각각 SMALL/TINY에 배치된다.
- [x] SMALL·LARGE가 이미 존재하는 상태에서도 모든 경계 크기의 group이 맞는다.

### M1. 필수 계약과 출력 정확성 확보

**상태:** Fedora에서 자동 검사 30/30 통과 (2026-10-07). show_alloc_mem 및 페이지 크기 항목을 재검증했다. getenv 사용 근거 문서화가 남아 M1 전체 완료 판정은 보류한다.

**범위:** `get_heap.c`, `helper_heap.c`, `malloc.c`, `realloc.c`, `tools.c`, `show_alloc_mem.c`, 헤더 및 Makefile.

- [x] 총 free_size와 “요청을 수용하는 연속 블록”을 구분한다.
- [x] 적합한 연속 블록이 없으면 새 heap을 생성하며, 다른 free 공간 합계만으로 잘못 선택하지 않는다.
- [x] ft_memmove의 서로 다른 객체 간 포인터 뺄셈을 제거한다.
- [x] realloc 확장 성공 시 기존 데이터가 보존된다.
- [x] realloc 실패 시 원래 포인터·데이터·할당 상태가 유지된다.
- [x] 사이즈 정렬, metadata 합산, 페이지 반올림의 overflow 검사 유지.
- [x] 반환 주소가 해당 플랫폼의 기본 객체 정렬 요구사항을 충족한다.
- [x] TINY/SMALL zone은 metadata까지 포함해 최대 크기 할당 100개 이상을 수용한다.
- [x] show_alloc_mem의 heap·block 주소가 오름차순이고 Total 합계가 일치한다. — 혼합 TINY/SMALL/LARGE, 여러 block, 강제 역순 heap, free 후 hole·heap 제거, 빈 상태를 검증했다. Total은 요청 크기가 아닌 살아 있는 block의 data_size 합계다.
- [x] 대상 플랫폼 Fedora/Linux에서 페이지 크기를 sysconf(_SC_PAGESIZE)로 취득하고 출력한다. — get_page_size(), 일반·확장 출력, TINY/SMALL zone 배수 및 LARGE metadata 포함 페이지 반올림·overflow 회귀 통과. 다른 OS는 이번 학교 환경 검증 범위 밖이다.
- [x] HOSTTYPE 미설정 시 fallback, 라이브러리 이름, symlink, 헤더 변경 시 rebuild를 검증한다.
- [x] 변경 없는 두 번째 make에서 불필요한 컴파일·링크가 없다.
- [ ] 추가 보너스 함수 getenv 등의 사용은 방어 가능한 이유를 문서화한다.

**완료 조건:** 모든 유효한 경계 입력에서 데이터 보존·정렬·분류·출력 검증을 통과하고, 실패 경로에서 allocator 상태를 잃지 않는다.

### M2. 해제 공간 defragmentation 완료

**범위:** `split_block()`, `coalesce_block()`, `start_free()`, `start_realloc()`.

유지할 불변식:
1. 같은 heap 안에서 물리적으로 인접한 free 블록은 작업 완료 후 하나로 병합된다.
2. 병합된 크기에는 제거된 block header 공간이 포함된다.
3. prev/next 링크, block_count, free_size는 실제 상태와 일치한다.
4. 살아 있는 payload는 이동하거나 손상시키지 않는다.

- [ ] realloc 축소로 생성한 free 블록을 기존 다음 free 블록과 병합한다.
- [ ] 연속 free 블록 여러 개가 있어도 병합이 중간에 멈추지 않는다.
- [ ] 앞·뒤·양쪽 병합과 heap 첫/마지막 블록을 검증한다.
- [ ] 병합 후 더 큰 요청이 같은 공간을 재사용함을 확인한다.
- [ ] 완전히 빈 heap의 유지·회수 후에도 metadata와 group별 개수가 맞는다.

**필수 반례:**
- a=malloc(128), b=malloc(64), c=malloc(64).
- free(b), a=realloc(a,64), free(a).
- c는 계속 유효해야 하고, 그 앞의 연속 free 공간은 하나여야 한다.

**완료 조건:** free 또는 realloc 축소 뒤 전체 블록 리스트 검사에서 인접 free 블록이 0쌍이며, 병합 공간 재사용 테스트가 통과한다.

### M3. 디버그 환경변수와 show_alloc_mem_ex 완료

**범위:** `init_debug_flags()`, debug 출력, scribble 경로, hex dump, README.

PDF는 사용자 정의 디버그 변수를 허용한다. 현재 동작의 범위를 명시하고 그 범위대로 검증한다.

- [ ] MALLOC_DEBUG의 미설정/0/1/빈 문자열 해석과 초기화 시점을 정한다.
- [ ] MALLOC_SCRIBBLE의 값 해석과 실행 중 변경 지원 여부를 정한다.
- [ ] malloc 성공/실패, free(NULL), realloc 성공/실패의 로그를 검증한다.
- [ ] 할당 시 0xaa 적용 범위를 문서화하고 확인한다.
- [ ] 해제 시 0xdd를 LARGE에만 적용할지 TINY/SMALL까지 적용할지 결정한다.
- [ ] realloc(NULL,n) 및 확장된 새 영역의 scribble 계약을 명시한다.
- [ ] 로그·dump 경로가 libc malloc을 재호출하거나 mutex를 중복 획득하지 않는지 확인한다.
- [ ] show_alloc_mem_ex는 살아 있는 할당과 알려진 바이트 패턴을 정확히 출력한다.
- [ ] 16바이트 행 경계, 마지막 불완전 행, 빈 상태를 검증한다.
- [ ] 일반 출력과 확장 출력의 주소 순서·Total을 일치시킨다.

**완료 조건:** 변수별 동작 표와 테스트 결과가 일치한다. hex dump가 구현되었으므로 allocation history를 추가로 만드는 것은 이 계획의 필수 범위가 아니다.

주의: free한 포인터를 다시 읽어 0xdd를 검증하지 않는다. 내부 검사 지점 또는 munmap 이전의 계측으로 확인한다.

### M4. thread safety와 실패 검출 강화

**범위:** 모든 public entry point, start_* 내부 호출 계약, `main.c` 또는 별도 테스트 harness, Makefile.

- [ ] malloc/free/realloc/show_alloc_mem/show_alloc_mem_ex의 공유 상태 접근이 같은 mutex로 보호된다.
- [ ] 내부 start_* 호출은 이미 잠긴 상태에서 호출된다는 계약을 문서화한다.
- [ ] 잠금 보유 중 다시 public allocator를 호출하는 경로가 없다.
- [ ] 각 early return과 오류 경로의 unlock을 확인한다.
- [ ] pthread 사용에 필요한 컴파일·링크 옵션을 플랫폼에 맞게 적용한다.
- [ ] pthread_create 성공한 스레드만 join한다.
- [ ] worker의 할당 실패·데이터 손상·검증 실패를 최종 결과에 반영한다.
- [ ] malloc/free뿐 아니라 realloc 축소·확장과 조회 함수의 동시 실행을 검증한다.
- [ ] 전달된 포인터는 동기화한 ownership handoff 후 다른 스레드에서 해제한다.
- [ ] 같은 payload의 동시 쓰기와 free 같은 잘못된 사용자 동작을 allocator 실패로 혼동하지 않는다.
- [ ] timeout으로 deadlock·hang를 실패 처리한다.

**완료 조건:** 여러 스레드 수·반복 횟수에서 패턴 보존과 내부 상태 검사를 통과한다. 오류를 의도적으로 주입하면 테스트가 비정상 종료 코드로 실패한다.

기존 “all threads completed without crash: PASS”는 데이터 정확성의 충분한 근거가 아니다. 사용자 payload 쓰기와 hex dump 읽기도 별도 동기화 없이 병행하지 않는다.

### M5. 통합 회귀 및 제출 게이트

**범위:** 전체 프로젝트, README, 재현 테스트와 실행 기록.

- [ ] M0–M4의 회귀 테스트를 한 번에 실행할 수 있다.
- [ ] 테스트 전체 실패는 nonzero exit code로 전달된다.
- [ ] 필수 테스트를 debug/scribble 미설정 상태와 활성화 상태에서 모두 실행한다.
- [ ] 지원하는 OS별 빌드·실행 결과를 따로 기록한다. 미실행 플랫폼은 미검증으로 표시한다.
- [ ] 빈 TINY/SMALL heap을 유지하는 정책이 반복 malloc/free의 mmap/munmap 호출 수를 제한함을 계측한다.
- [ ] 큰 할당의 해제, 모든 블록 해제, 여러 heap의 회수 순서를 검증한다.
- [ ] README의 실제 동작·제한과 코드가 일치한다.
- [ ] 최종 커밋 SHA, 환경, 명령, 종료 코드, 실패 수를 기록한다.

**제출 게이트:**
- [ ] 유효한 호출에서 재현되는 crash·손상·deadlock이 없다.
- [ ] PDF 필수 요구사항 전체가 통과했다.
- [ ] 보너스 네 항목이 각각 독립 검증을 통과했다.
- [ ] 미검증 범위를 성공으로 표시하지 않았다.

## 4. 검증 케이스 목록

아래는 전체 검증 계획이다. `tests/edge_cases.c`의 필수 16개 테스트는 통과했으며, M1의 Linux 빌드 계약도 추가 실행에서 통과했다. 보너스 및 다른 OS는 아직 미검증이다.

| 케이스 | 입력·순서 | 확인할 결과 |
| --- | --- | --- |
| LARGE 수명 회귀 | 4096 → realloc 2048 → 별도 64 할당 → 첫 포인터 free | 두 번째 할당 유지 |
| group 격리 | SMALL/LARGE 생성 뒤 TINY 요청 | 요청 group에서 할당 |
| 축소 병합 | M2의 a/b/c 반례 | 인접 free 블록 없음, c 보존 |
| 연속 공간 부족 | 여러 작은 free 공간의 합은 충분하지만 개별 공간은 부족 | 새 heap에서 성공, 기존 데이터 보존 |
| 경계 크기 | 1, n, n+1, m, m+1 및 정렬 경계 | group·주소 정렬 일치 |
| 크기 overflow | SIZE_MAX 및 metadata/정렬 반올림 경계 | NULL, 기존 상태 유지 |
| realloc 보존 | 축소·확장·이동·강제 실패 | 보존해야 할 byte와 원본 수명 유지 |
| heap 회수 | 같은 group 여러 heap, 역순·정순 해제 | 리스트·개수·유효 mapping 일치 |
| 출력 정렬 | 여러 heap 생성 및 일부 회수 | 주소 오름차순, Total 정확 |
| hex dump | 0x12, 0xab 등 알려진 패턴 | 정확한 hex, 행 경계 |
| 디버그 변수 | 미설정/0/1/빈 값, realloc 경로 | 문서화된 계약과 일치 |
| 동시성 | malloc/realloc/free + 동기화된 조회 | 손상·hang 없음, 실패 전파 |
| 호출 수 | 반복적인 작은 할당/해제 | 매 반복마다 mapping 생성·해제하지 않음 |
| 빌드 계약 | HOSTTYPE fallback, 재실행, 헤더 수정 | 이름·symlink·필요한 rebuild |

## 5. 현재 사용 가능한 실행 명령과 한계

저장소 루트에서 실행한다.

```sh
make test
make test-m1
make
make run
make debug_mode
make scribble_mode
make valgrind
```

- 현재 main.c는 마지막에 항상 0을 반환하므로 종료 코드 0만으로 성공을 판단할 수 없다. M4/M5에서 수정할 항목이다.
- make valgrind는 현재 x86_64_Linux 라이브러리 이름을 하드코딩한다. 다른 HOSTTYPE에서는 조정이 필요하다.
- `make test`는 allocator를 별도 이름으로 컴파일하고 각 테스트를 독립 프로세스에서 실행한다. 실패·signal 종료는 nonzero 결과로 집계한다. 공유 라이브러리 interposition, 보너스 및 OS별 빌드 계약은 이 테스트 범위 밖이다.
- sanitizer나 Valgrind 적용 시 custom allocator의 interposition·도구 호환성을 먼저 확인한다. 도구 실행 성공만으로 allocator 정확성을 판정하지 않는다.

## 6. 최신 완료·검증 기록

| 마일스톤 | 상태 | 검증 커밋 | 증거·테스트 결과 | 남은 미검증 |
| --- | --- | --- | --- | --- |
| M0 | 완료 | `90bdf749b9075b655544c4bd92e4f37c0f7f0a2c` | group 격리, LARGE 축소 후 양쪽 free 순서, 크기 경계 통과 | 공유 라이브러리 통합·다른 OS는 M5에서 검증 |
| M1 | 자동 검사 통과, 일부 미검증 | `3c106d5910d1124fe0ec1103eaeb3fad8b1e8bcd` + 미커밋 수정 | `make test-m1`: Fedora 44 x86_64, 종료 코드 0, 30/30 통과, 실패 0 | getenv 사용 근거, C 이식성 전체; 다른 OS는 학교 대상 범위 밖 |
| M2 | 미완료 | — | — | — |
| M3 | 미완료 | — | — | — |
| M4 | 미완료 | — | — | — |
| M5 | 일부 검증 | 동일 커밋 | `make test`: 종료 코드 0, 실패 0, 16/16 통과 | M2–M4, 환경변수 활성화, OS별 실행, 호출 수, 전체 제출 게이트 |

각 마일스톤은 코드를 작성했다는 이유만으로 완료 처리하지 않는다. 해당 완료 조건을 검증한 커밋과 실행 결과가 있어야 한다.


### 2026-10-06 실행 결과

- 명령: 저장소 루트에서 `make test`.
- 결과: **16/16 통과**, 종료 코드 **0**, 실패 **0**.
- 확인 범위: zero/NULL, 정렬·크기 경계·비중첩, group 격리, overflow, realloc 축소·확장·원본 보존, mmap 실패, LARGE 축소 수명, 최소 분할 잔여 공간, fragmentation/여러 heap, LARGE 리스트 해제 순서, 출력 Total·주소 순서.
- 주소 순서 통과는 이번 mmap 배치에서의 결과이며 모든 배치를 증명하지 않는다.
- 미검증: thread safety, debug/scribble, show_alloc_mem_ex, defragmentation. M2–M4는 미완료로 유지한다.
- 이번 작업에서는 기존 코드의 테스트 결과를 기록했으며 allocator 소스를 추가 수정하지 않았다.

### M1 추가 테스트 기록 (2026-10-06)

- 실행: `make test-m1` — 기존 필수 16개 + M1 실행 6개 + 소스 점검 2개 + 빌드 점검 3개.
- 결과: **22/27 통과**, 실패 **5**, `make` 종료 코드 **2**. M1은 미완료다.
- 실행 실패: 같은 group 내 연속 공간 부족 시 새 heap 할당, 강제로 역순 연결한 heap의 주소 오름차순 출력.
- 소스 점검 실패: `ft_memmove`의 `d - s`, Linux의 `getpagesize()` 사용. 소스 패턴 점검이며 C 정의된 동작 전체를 증명하지 않는다.
- 빌드 실패: `inc/define.h` 변경 후 공유 라이브러리 object 재빌드 없음.
- 추가 통과: TINY/SMALL 단일 zone의 최대 크기 100개 수용, max_align_t 정렬, memmove 기능, 페이지 크기 출력, HOSTTYPE fallback·이름·symlink, 변경 없는 두 번째 make.
- 기존 `make test`도 재실행하여 **16/16 통과** 확인. allocator 소스는 수정하지 않았다.
- 상세 범위·한계: `tests/README.md`의 M1 Contract Tests. getenv 사용 근거, 다른 OS, mixed-class·block 출력 전체 검증은 아직 남아 있다.

### 테스트 언어 통일

- 필수·M1 실행기와 검증 로직을 `tests/run_tests.c`로 통일했다.
- Python 실행기와 shell 테스트 스크립트는 제거했다. `make test`, `make test-m1` 명령은 유지한다.
- C 실행기는 fork/exec로 각 테스트를 격리하고 timeout·signal 종료·실패를 집계한다. M1 소스 점검과 임시 복사본의 빌드 계약 점검도 C에서 수행한다.

### M1 재검증 기록 (2026-10-07)

- 검증 대상: HEAD `3c106d5910d1124fe0ec1103eaeb3fad8b1e8bcd` + 현재 미커밋 코드·테스트 수정 사항. HEAD 커밋 단독의 검증 결과로 해석하지 않는다.
- 명령: 저장소 루트에서 `make test-m1`.
- 결과: **27/27 통과**, 실패 **0**, 종료 코드 **0**. 필수 실행 16개 + M1 실행 6개 + 소스 점검 2개 + 빌드 점검 3개.
- 이전 실패 5개 모두 통과: 같은 group 내 연속 공간 부족 시 새 heap 생성, 강제 역순 heap의 오름차순 출력, ft_memmove의 포인터 뺄셈 제거 점검, Linux 페이지 API 점검, define.h 변경 후 object 재빌드.
- 기타 통과: TINY/SMALL 최대 크기 100개 수용 및 정렬·데이터 보존, memmove 기능, 페이지 크기 출력, HOSTTYPE fallback·라이브러리 이름·symlink, 변경 없는 두 번째 make.
- 헤더 rebuild 검사는 임시 복사본의 헤더 시간을 미래로 설정하므로 make의 clock skew 경고가 출력됐다. 재컴파일·재링크 및 검사 통과, 종료 코드 0을 확인했다.
- 한계: 소스 패턴 점검은 C 정의된 동작 전체를 증명하지 않는다. mixed-class·block 출력 전체, getenv 사용 근거, 다른 OS 및 보너스 검증은 남아 있다. 전체 판정은 INCOMPLETE로 유지한다.
- 이번 작업에서는 테스트 실행과 본 문서 갱신만 수행했으며 allocator 구현은 수정하지 않았다.

### show_alloc_mem·페이지 크기 재점검 (2026-10-07, Fedora)

- 환경: `/etc/os-release`에서 Fedora Linux 44 확인, x86_64, `getconf PAGESIZE`는 4096. 학교의 Fedora 버전과 동일한지는 확인하지 않았으며 학교 장비에서 직접 실행한 결과는 아니다.
- 명령·결과: `make test-m1`, **30/30 통과**, 실패 **0**, 종료 코드 **0**. 기존 27개에 실행 테스트 3개 추가.
- show_alloc_mem: 혼합 TINY/SMALL/LARGE heap을 의도적으로 역순 연결한 뒤 heap·payload 주소 오름차순, 주소별 owner/group, end = start + data_size, 중복·누락 없음, Total 합계를 확인했다. free 블록과 제거된 LARGE heap은 출력·합계에서 제외되며, 최초 빈 상태와 전체 free 후에도 Total 0이다.
- 페이지 크기: sysconf(_SC_PAGESIZE) 기준으로 get_page_size 및 일반·확장 출력이 일치한다. TINY 4페이지·SMALL 32페이지, mapping 시작·크기의 페이지 정렬, LARGE의 정렬 payload + heap/block metadata를 포함한 페이지 반올림, overflow 거절을 확인했다. 4096을 코드에 고정하지 않는다.
- 출력은 물리적 block 연결 순서와 주소별 heap 선택을 사용한다. 해당 회귀 범위에서 구현 결함을 발견하지 않아 동작 코드는 변경하지 않았다.
- 테스트 구현은 `tests/m1_cases.c`, 헤더는 함수 선언만 유지한다. 출력 캡처는 임시 파일을 사용해 pipe 용량 때문에 조회가 멈추는 상황을 피한다.
- 보너스 show_alloc_mem_ex는 이번에 페이지 출력만 확인했다. hex dump 정확성·동시성·M2–M4의 완료를 의미하지 않는다.
