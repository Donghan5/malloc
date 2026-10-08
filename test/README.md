# 기능별 malloc 테스트

저장소 루트에서 실행한다. Linux/POSIX, GCC, pthread, make, cp, nm가 필요하다.
allocator의 `src/`, `inc/`는 변경하지 않는다. 과제 `malloc.pdf` v6.3의
최소 100개 zone 용량, 정렬, realloc 보존, 주소순 출력, 공유 라이브러리 규칙을
검사한다. 페이지 점수 임계값은 이번 요청의 평가 기준이며 PDF 본문에는 없다.

| 명령 | 빌드·실행 범위 |
|---|---|
| `make test-mandatory` | mandatory만; 일반 모드와 debug/scribble 모드 |
| `make test-bonus` | bonus만 |
| `make test-integration` | 실제 공유 라이브러리, 빌드 계약, LD_PRELOAD 측정 |
| `make test-all` 또는 `make test` | 모든 그룹, 실패 시 최종 종료 코드도 실패 |
| `make test-build` | 모든 실행 파일과 공유 라이브러리 빌드만 |
| `make test-clean` | test/obj 제거 |
| `make test-fclean` | test/obj와 test/bin 전체 제거, test_malloc 제거 |
| `make test-correction` | 기존 test0–test5 수동 관찰용 프로그램 |

```sh
make test-fclean
make test-mandatory
make test-bonus
make test-integration
make test-all
./test/bin/mandatory/basic 8
./test/bin/bonus/diagnostics 0
env -u LD_PRELOAD ./test/bin/integration/preload
readelf -d test/bin/integration/workload
ldd test/bin/all_cases
```

단위 suite를 인자 없이 실행하면 케이스 수를 출력한다. 인자는 0부터 시작하는
케이스 번호다. 그룹 선택 빌드는 해당 그룹과 libc runner만 빌드한다.
`test-integration`은 공유 라이브러리도 빌드한다. 이미 생성된 다른 그룹의
파일은 선택 실행에 포함되지 않는다.

구조와 검증 범위:

| 경로 | 검증 |
|---|---|
| mandatory/basic.c | NULL/0, 정렬·분류·overflow, realloc 성장·축소·실패 보존, mmap 실패, LARGE 축소 후 수명 독립성, split 경계·단편화·unlink |
| mandatory/zones_output.c | TINY/SMALL 각 100개 최대 크기 할당, 단편화, memmove, 강제로 뒤섞은 heap의 주소순 출력, payload 범위·정확한 Total·빈 출력, 페이지 geometry |
| mandatory/reclamation.c | 첫/끝 블록 metadata, 여러 TINY/SMALL heap의 정방향·역방향 회수, 카운터 |
| mandatory/reuse.c, heap_edges.c | TINY/SMALL 반복 재사용, LARGE 중간·양끝 회수, mmap 실패 후 복구 |
| bonus/coalescing*.c | 앞·뒤·양쪽 인접 병합, 연속 축소·병합, header 공간 회수와 재사용, metadata·카운터 |
| bonus/diagnostics*.c | 환경변수 정확한 값과 캐싱, 성공·실패 로그, scribble, realloc(NULL), hex 행 경계, freed hole 제외 |
| bonus/concurrency*.c | 2/4/8 스레드 malloc/free/realloc와 출력, 초기화 경쟁, 소유권 전달, 실패·조기 반환 뒤 mutex 해제, 실제 worker 오류 전파 |
| integration/build_contracts.c | HOSTTYPE fallback·빌드·symlink, 공개 심볼, 불필요한 재빌드 방지, header 의존성, 기존 소스 audit |
| integration/workload.c, preload.c | 실제 LD_PRELOAD 주입 및 페이지 측정 |
| all_cases/all_cases.c | 실행, 프로세스 격리, 출력 수집, 결과 집계만 |
| helpers/, ui/ | 공통 fixture·metadata 검사·출력 |

[케이스 이동 대응표](CASE_MAP.md)에 모든 기존 runtime 케이스와 빌드 검사의
원래 위치, 새 위치, 합친 검증을 기록했다. 중복된 필수 edge/M1 실행 등록을
합치고, Total·주소순 검사는 더 강한 mixed/강제 역순 출력 검사에 통합했다.
M5 enabled regression은 필수 성장·overflow 검사의 enabled 재실행으로 통합했다.
동일한 metadata 검사 구현도 공통 fixture로 통합했다. 새 구조에서 회귀 범위를
확인한 뒤 milestones/, edge_cases/, 이전 run_tests.c를 삭제했다.

`all_cases`, 측정 드라이버, workload는 allocator 소스를 링크하지 않는다.
단위 테스트만 malloc/free/realloc을 edge_*로 바꿔 allocator 소스와 링크하며,
libc 내부의 할당과 테스트 heap을 분리한다. workload는 이름 변경 없이 libc만
링크한다(현 환경에서 DT_NEEDED는 libc.so.6뿐). PIE를 사용하여 malloc 함수
주소가 실행 파일의 PLT 대신 실제 동적 심볼 주소를 나타내게 한다.
각 runtime 케이스는 별도 exec 프로세스에서 10초 alarm으로 실행한다.
runner는 core dump를 끄고 외부 180초 제한을 둔다. 빌드 audit는 임시 복사에서
진행하고 각 빌드에 60초 제한을 둔다. crash·timeout·비정상 종료는 실패다.

환경변수 계약은 두 변수 모두 정확한 문자열 `"1"`만 활성화하는 것이다.
미설정, `""`, `"0"`, `"10"`은 비활성이다. 첫 초기화에서 두 값을 함께 캐싱하며
실행 중 변경·제거는 반영하지 않는다. 이전 runtime-toggle 검사는 이 계약과
충돌하여 cached_scribble 검사로 바꿨다. 이전 README의 존재 여부/매 호출 갱신
설명도 잘못되어 제거했다. 25개 값 조합과 활성/비활성 상태의 변경·제거를
검사한다. 초기화 상태 리셋은 helpers/bonus_helpers.c의 fixture_reset_flags에만
구현하고 테스트에서만 호출한다. allocator reset API는 추가하지 않는다.

LD_PRELOAD 측정은 **로컬 동등 workload**다. correction/test0–test5가 공식 평가
첨부 파일이라는 provenance를 확인할 수 없으므로 공식 workload로 표시하지 않는다.
측정 드라이버 자체에는 주입하지 않는다. 드라이버는 절대경로로 실제 .so를
해결하여 자식에만 LD_PRELOAD를 설정한다. 각 workload에서 dlsym의 malloc과
실제 malloc 함수 주소가 같은지 검사하고, dladdr 결과의 파일 device/inode가
해당 .so와 같은지 확인한다. 주입 실패를 libc로 조용히 대체해 PASS하지 않는다.

5회 반복마다 baseline, 1024회 × 1024바이트를 유지하는 hold, 같은 할당을 매번
free하는 free를 각각 새 프로세스로 실행한다. 모든 요청 바이트에 volatile
쓰기를 수행하며 hold는 읽기도 수행한다. 기본 측정은 MALLOC_DEBUG와
MALLOC_SCRIBBLE을 해제한다. `wait4`의 `ru_minflt` 원시 값과 baseline 차이를
출력한다. baseline도 동일한 심볼 검증·동적 로딩 경로를 실행한다.
일반 실행 성공(execution)과 만점 기준(allocation-score/free-score)은 따로 표시한다.
각 반복에서 hold 차이 255~272, free 차이 최대 3이어야 전체 측정이 PASS다.
주입 실패, crash, 10초 timeout, 할당 실패도 전체 실패로 전파된다.

minor faults는 현재 사용 중인 메모리 페이지 수나 mmap 호출 수 자체가 아니다.
동적 로더, stack, metadata 초기화, ASLR, libc 및 페이지 크기의 영향을 받는다.
baseline 차감도 이 변동을 완전히 제거하지 못한다. 초기화에서 zone 전체를 쓰면
나중에 free해도 누적 fault가 사라지지 않는다. 현 구현의 create_new_heap은
전체 mapping을 ft_memset하고 최종 빈 SMALL zone(32페이지)을 보유하므로,
관찰된 free 증가와 hold의 metadata·zone overhead에 부합한다. 정확한 원인별
페이지 기여도는 시스템 호출 tracing으로 분해하지 않았다. 기준을 완화하거나
allocator를 변경하지 않았다. **Fedora에서 공식 평가 명령과 공식 첨부 workload로
재확인해야 한다.** 현재 실행 환경도 Fedora 44지만 공식 명령 검증은 아니다.

2026-10-09 최종 검증: Fedora 44 x86_64, 페이지 크기 4096.

| 실행 명령 | 결과 |
|---|---|
| make test-build | 성공, Werror 빌드 |
| make test-mandatory | 58/58 통과 (29개 × 일반/enabled) |
| make test-bonus | 24/24 통과 |
| make test-integration | 그룹 1/2 통과; 빌드 audit 내부 6/6 통과, 페이지 측정 0/5 통과 |
| make test-all | 83/84 통과; 실행 파일 exit 1, make exit 2 |
| git diff --check | 통과 |
| ldd / readelf / nm | runner·driver·workload에 allocator 의존성·정의 없음 확인 |

최종 test-all의 원시 측정:

| 반복 | baseline | hold | free | hold 차이 | free 차이 |
|---|---:|---:|---:|---:|---:|
| 1 | 97 | 385 | 130 | 288 | 33 |
| 2 | 100 | 384 | 128 | 284 | 28 |
| 3 | 95 | 388 | 129 | 293 | 34 |
| 4 | 97 | 385 | 129 | 288 | 32 |
| 5 | 97 | 384 | 127 | 287 | 30 |

15개 측정 프로세스 모두 주입·실행 성공. 두 점수 기준은 5회 모두 실패했다.
전체 집계의 integration/preload 한 항목 안에 5회 측정과 두 기준 검사가 포함되며,
빌드 한 항목 안에는 6개 audit가 포함된다. 집계 84는 독립 계약 수와 같지 않다.
새 구조의 회귀 실패는 없고, 남은 실패는 기존 allocator의 평가 기준 미달이다.
주입 없는 workload와 드라이버에 LD_PRELOAD가 설정된 경우의 거부도 확인했다.

현재 미검증: 공식 평가 첨부·명령, 다른 OS/페이지 크기, 임의 포인터 free·double
free, malloc 외 전체 libc 호환성, mmap/munmap 실제 호출 횟수, 로그 인자 전체,
확장 출력의 전체 주소순, realloc 성장 tail scribble, munmap 전 0xdd instrumentation.
concurrency stress는 dump와 payload 쓰기를 별도 mutex로 조정하며 독립 payload의
공개 allocator 호출은 동시에 수행한다. sanitizer/장기 stress는 실행하지 않았다.
할당 이력은 구현돼 있지 않아 테스트나 가짜 PASS를 추가하지 않았다.
correction 프로그램은 출력·페이지 검증 assertion이 없는 수동 관찰 도구이며
이번 자동 결과에는 포함되지 않는다.
