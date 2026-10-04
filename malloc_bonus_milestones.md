# malloc: 필수 결함 수정 및 전체 보너스 완료 마일스톤

- 작성일: 2026-10-02 (Europe/Paris)
- 과제 기준: 첨부 `malloc.pdf`, Version 6.3, Chapter II–IV
- 리뷰 기준: `Donghan5/malloc`, `main@5b89b15d8b71c742b98e4775bab180e579edb5c6`
- 저장소: https://github.com/Donghan5/malloc/tree/5b89b15d8b71c742b98e4775bab180e579edb5c6
- 검증 환경: 이전 리뷰에서 Linux x86_64, 페이지 크기 4096으로 빌드·실행
- 이 문서는 수정 계획이다. 코드 수정이나 새로운 HEAD 재검증을 완료했다는 의미가 아니다.
- 현재 판정: **INCOMPLETE**

## 1. 목표와 평가 조건

목표는 필수 구현의 확인된 결함을 제거하고, PDF에 명시된 보너스 네 항목을 모두 설명·검증 가능한 상태로 만드는 것이다.

PDF는 첫 보너스로 pthread 기반 thread safety를 제시하고, 추가 기능으로 디버그 환경변수, `show_alloc_mem_ex()`, 해제 공간 defragmentation을 예시한다. 추가 기능 목록은 비 exhaustive 목록이다. 네 항목을 구현했다고 최대 점수가 자동 보장되지는 않는다.

**평가 선행 조건:** 필수 구현이 완벽해야 보너스를 평가한다. 현재 재현된 SIGSEGV를 해결하기 전에는 보너스 평가 가능 상태로 표시하지 않는다.

| 항목 | 현재 확인된 상태 | 목표 |
| --- | --- | --- |
| pthread 기반 thread safety | 공개 allocator·조회 함수에 mutex 존재. 기존 4스레드 테스트 실행 완료 | 상태 변경·조회 경로의 잠금 계약 확인 및 실패를 검출하는 동시성 테스트 |
| 디버그 환경변수 | MALLOC_DEBUG 로그, MALLOC_SCRIBBLE의 할당 시 0xaa 확인 | 변수 해석·적용 범위를 명시하고 malloc/free/realloc 경로별 검증 |
| show_alloc_mem_ex() | hex dump 출력 확인 | 일반 조회의 정확성을 공유하고 알려진 바이트 패턴으로 검증 |
| 해제 공간 defragmentation | 일반 양방향 병합 통과. realloc 축소 후 병합 불완전 | 인접 free 블록이 남지 않고 병합 공간을 재사용 |
| 필수 구현 | 다른 살아 있는 할당까지 munmap하는 오류 재현 | 유효한 호출의 데이터·수명·분류·출력 계약 충족 |

## 2. 현재 수정이 필요한 부분

증거 구분:
- **실행 확인:** 이전 리뷰의 별도 반례에서 재현.
- **코드 확인:** 소스 흐름에서 확인. 특정 실행 실패를 재현했다는 뜻은 아님.
- **미검증:** 테스트나 근거가 아직 없음.

| 우선순위 | 문제 | 증거 | 위치 | 필요한 수정 범위 |
| --- | --- | --- | --- | --- |
| CRITICAL | LARGE를 축소한 뒤 그 잔여 공간에 다른 할당이 들어가며, 첫 할당 free가 두 번째 할당까지 해제 | 실행 확인: 두 번째 포인터 접근 SIGSEGV | src/realloc.c, src/block/block.c, src/free.c, src/heap/heap.c | LARGE 분할·재사용 정책과 mapping 소유권을 일관되게 변경 |
| HIGH | 요청 크기와 heap 종류가 일치하지 않음 | 실행 확인: malloc(512) 뒤 malloc(64)가 SMALL에 배치 | find_free_block(), start_malloc() | free 블록 검색에 요청 group 조건 적용 |
| HIGH | realloc 축소 이후 인접 free 블록이 남음 | 실행 확인 | split_block(), start_realloc(), coalesce_block() | 축소 잔여 블록 병합 및 block_count/free_size 유지 |
| HIGH | show_alloc_mem()이 주소 오름차순을 보장하지 않음 | 코드 확인: 연결 리스트 순서 그대로 출력 | src/tools/show_alloc_mem.c, heap 삽입 경로 | heap 출력 순서와 전체 합계 검증 |
| HIGH | 서로 다른 할당의 포인터 차를 계산함 | 코드 확인: ft_memmove()의 d - s. 서로 다른 객체 간 포인터 뺄셈은 C의 정의된 연산이 아님 | src/tools/tools.c | 포인터 차에 의존하지 않는 복사 방향 결정 |
| MEDIUM | 총 free_size만으로 heap을 선택한 뒤 첫 블록만 검사하는 fallback 경로 | 코드 확인, 실패 입력 별도 재현 필요 | get_available_heap(), start_malloc() | 실제 사용 가능한 연속 free 블록을 기준으로 heap 선택 |
| MEDIUM | 멀티스레드 데이터 손상을 최종 실패 판정에 반영하지 않음 | 코드 확인 | main.c의 thread_routine(), test_multithread() | worker 결과 수집, 생성 성공 스레드만 join, 실패 exit code |
| 검증 필요 | 재할당 후 메타데이터·빈 heap 회수·실패 시 원본 보존의 전체 계약 | 일부 테스트만 실행됨 | allocator 전반 | 아래 회귀 테스트로 확정 |
| 검증 필요 | Linux 허용 함수·빌드 재실행·헤더 의존성 | PDF는 Linux에서 sysconf(_SC_PAGESIZE)를 명시, 현재 getpagesize() 사용 | inc/define.h, helper_heap.c, show_alloc_mem.c, Makefile | 플랫폼별 페이지 크기 취득 및 빌드 계약 점검 |

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

**범위:** `src/malloc.c`, `src/realloc.c`, `src/block/block.c`, `src/free.c`, 필요 시 `src/heap/heap.c`.

- [ ] LARGE mapping 하나에 독립된 다른 할당이 들어가지 않도록 정책을 정한다.
- [ ] LARGE realloc 축소 시 잔여 공간의 처리와 free 정책을 일치시킨다.
- [ ] TINY/SMALL free 블록 검색에서 요청 크기에 대응하는 group만 선택한다.
- [ ] free가 해제하는 mapping과 그 안의 살아 있는 블록 관계를 명시한다.
- [ ] 다음 반례를 독립 프로세스의 회귀 테스트로 추가한다.

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
- [ ] 위 순서를 반복해도 SIGSEGV·데이터 손상이 없다.
- [ ] b를 먼저 free하는 순서도 안전하다.
- [ ] malloc(512) 뒤 malloc(64)가 각각 SMALL/TINY에 배치된다.
- [ ] SMALL·LARGE가 이미 존재하는 상태에서도 모든 경계 크기의 group이 맞는다.

### M1. 필수 계약과 출력 정확성 확보

**범위:** `get_heap.c`, `helper_heap.c`, `malloc.c`, `realloc.c`, `tools.c`, `show_alloc_mem.c`, 헤더 및 Makefile.

- [ ] 총 free_size와 “요청을 수용하는 연속 블록”을 구분한다.
- [ ] 적합한 연속 블록이 없으면 새 heap을 생성하며, 다른 free 공간 합계만으로 잘못 선택하지 않는다.
- [ ] ft_memmove의 서로 다른 객체 간 포인터 뺄셈을 제거한다.
- [ ] realloc 확장 성공 시 기존 데이터가 보존된다.
- [ ] realloc 실패 시 원래 포인터·데이터·할당 상태가 유지된다.
- [ ] 사이즈 정렬, metadata 합산, 페이지 반올림의 overflow 검사 유지.
- [ ] 반환 주소가 해당 플랫폼의 기본 객체 정렬 요구사항을 충족한다.
- [ ] TINY/SMALL zone은 metadata까지 포함해 최대 크기 할당 100개 이상을 수용한다.
- [ ] show_alloc_mem의 heap·block 주소가 오름차순이고 Total 합계가 일치한다.
- [ ] 페이지 크기는 플랫폼별 허용 API로 취득하고 출력한다.
- [ ] HOSTTYPE 미설정 시 fallback, 라이브러리 이름, symlink, 헤더 변경 시 rebuild를 검증한다.
- [ ] 변경 없는 두 번째 make에서 불필요한 컴파일·링크가 없다.
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

아래는 구현할 테스트 목록이다. 기존 저장소에 이 테스트가 모두 있다는 뜻은 아니다.

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
make
make run
make debug_mode
make scribble_mode
make valgrind
```

- 현재 main.c는 마지막에 항상 0을 반환하므로 종료 코드 0만으로 성공을 판단할 수 없다. M4/M5에서 수정할 항목이다.
- make valgrind는 현재 x86_64_Linux 라이브러리 이름을 하드코딩한다. 다른 HOSTTYPE에서는 조정이 필요하다.
- 새 회귀 테스트의 실행 명령은 테스트 파일과 target을 실제로 추가한 뒤 README에 기록한다.
- sanitizer나 Valgrind 적용 시 custom allocator의 interposition·도구 호환성을 먼저 확인한다. 도구 실행 성공만으로 allocator 정확성을 판정하지 않는다.

## 6. 완료 기록 템플릿

| 마일스톤 | 상태 | 검증 커밋 | 증거·테스트 결과 | 남은 미검증 |
| --- | --- | --- | --- | --- |
| M0 | 미완료 | — | — | — |
| M1 | 미완료 | — | — | — |
| M2 | 미완료 | — | — | — |
| M3 | 미완료 | — | — | — |
| M4 | 미완료 | — | — | — |
| M5 | 미완료 | — | — | — |

각 마일스톤은 코드를 작성했다는 이유만으로 완료 처리하지 않는다. 해당 완료 조건을 검증한 커밋과 실행 결과가 있어야 한다.

