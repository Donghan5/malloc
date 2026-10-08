# malloc 테스트

프로젝트 루트에서 실행합니다. C 컴파일러, Linux/POSIX 프로세스 API,
pthread, `make`, `cp`가 필요합니다.

## 실행 명령

| 명령 | 범위 |
| --- | --- |
| `make test` 또는 `make test-all` | M1–M5, 전체 경계 테스트, 보너스 활성화 필수 회귀, M1 빌드·소스 검사 |
| `make test-edge` | 필수 경계 16개 + M2–M5 경계 12개 |
| `make test-m1` | 필수 회귀 16개 + M1 9개 + 빌드·소스 검사 5개 |
| `make test-m2` | 병합·축소·heap 회수 9개 |
| `make test-m3` | 환경변수·로그·scribble·hex dump 5개 |
| `make test-m4` | 동시성·포인터 전달·오류 주입 3개 |
| `make test-m5` | heap 재사용·보너스 활성화 회귀 2개 |
| `make test-build` | 전체 테스트 실행 파일 빌드 |

`test-m5`는 M5 개별 테스트입니다. 제출용 전체 검증은 `test-all`을 사용합니다.
개별 사례는 `./test/bin/edge_cases 16`처럼 0부터 시작하는 인덱스로 실행합니다.
인자 없이 실행 파일을 호출하면 사례 개수를 출력합니다.

## 구조

```text
test/
├── milestones/       m1_cases.c/.h … m5_cases.c/.h
├── all_cases/        all_cases.c/.h
├── helpers/          test_helpers.c/.h, bonus_helpers.c/.h
├── ui/               test_ui.c/.h
├── edge_cases/       edge_cases.c, bonus_edge_cases.c/.h
├── run_tests.c
├── codex.md
├── README.md
└── bin/              생성된 실행 파일 (Git 제외)
```

헤더에는 함수 선언만 둡니다. 타입·시스템 선언은 C 파일이 `inc/malloc.h`를
통해 가져옵니다. 새 테스트와 UI는 프로젝트 헤더와 `write` 기반 출력을
사용합니다. 기존 `run_tests.c`는 M1의 임시 빌드·소스 검사를 위해 사용하던
시스템 헤더를 유지합니다.

## 결과와 격리

각 runtime 사례는 별도 프로세스에서 실행합니다. 성공은 녹색 `PASS`, 실패는
적색 `FAIL`로 표시합니다. assertion 실패, crash, timeout 모두 전체 실패로
전파되며, 남은 사례는 계속 실행합니다. 실행 파일은 실패 시 1, `make`는
실패 시 비정상 종료합니다. core dump는 비활성화합니다.

runtime 사례의 alarm은 10초, 통합 실행기의 exec alarm은 20초입니다.
M1 전용 runner는 runtime 외부 제한 15초, 개별 빌드 제한 60초를 사용합니다.
출력은 삭제된 임시 파일에 캡처합니다. 통합 화면은 성공 시 첫 줄, 실패 시
최대 4095바이트를 표시합니다. M1 빌드는 임시 프로젝트 복사본에서 실행합니다.

allocator 함수를 `edge_malloc`, `edge_free`, `edge_realloc`로 바꾸어 링크하므로
libc의 내부 할당이 테스트 대상 heap에 섞이지 않습니다. LD_PRELOAD와 실제
시스템 프로그램 연동은 별도 검증이 필요합니다. 통합 실행기는 각 실행 파일의
사례 개수를 읽으므로, 사례를 추가해도 고정된 개수를 수정할 필요가 없습니다.

## 경계 사례

기존 필수 16개는 NULL/0, 정렬·크기 경계, overflow, realloc 보존·실패,
LARGE 축소 수명, 최소 분할, 단편화·다중 heap, heap unlink, 조회 출력을
검증합니다. malloc(0)과 realloc(ptr,0)은 이 프로젝트의 NULL 정책을 사용합니다.

추가한 M2–M5 12개는 다음을 검증합니다.

| 영역 | 추가 사례 |
| --- | --- |
| M2 | 정확한 최소 분할 나머지, 살아 있는 이웃 사이 축소, 병합 공간 재사용 |
| M3 | 실행 중 scribble 변경 및 빈 값, realloc(NULL,0/1), dump에서 해제된 hole 제외 |
| M4 | 0·NULL 경로 후 unlock, 8개 worker의 첫 할당, overflow 실패 후 unlock·재사용 |
| M5 | SMALL heap 100회 재사용, LARGE 중간·양쪽 heap 회수, mapping 실패 후 원본 보존·복구 |

분할 경계는 별도 mmap 영역에 구성한 metadata를 검사합니다. free한 payload는
읽지 않습니다. M4 첫 할당은 condition variable로 worker 시작을 맞추며,
pthread_create가 부분 실패해도 생성한 worker를 모두 해제·join합니다.
RLIMIT_AS 실패 주입은 해당 사례 프로세스에만 적용하고 원래 제한을 복원한 뒤
재할당 성공을 검증합니다. 임의 포인터 free와 double free는 포함하지 않습니다.

## 마일스톤 검증 범위

M1은 최대 블록 100개, group 격리·단편화, memmove, 주소 순서·payload 경계,
Total·페이지 크기·mapping geometry와 빌드 계약을 확인합니다. 소스 검사는
알려진 `d - s` 표현과 Linux `getpagesize()` 사용에 한정됩니다.

M2는 128/64/64 축소 회귀, 양방향·양쪽 병합, metadata 공간 복구, 여러 heap
회수를 확인합니다. 공유 metadata 검사는 연결, 물리적 범위, count, free_size,
인접 free 블록을 검사합니다. 마지막 빈 TINY/SMALL heap 보존을 허용합니다.

M3은 현재 환경변수 해석을 기준으로 합니다. MALLOC_DEBUG는 값과 무관하게
존재하면 활성화되고 초기화 후 캐시됩니다. MALLOC_SCRIBBLE은 호출마다 갱신되며,
존재하고 첫 문자가 `0`이 아니면 활성화됩니다. 빈 값도 활성화됩니다.
realloc(NULL,n)에는 malloc과 동일한 요청 바이트 `0xaa` 초기화를 요구합니다.
hex dump는 16바이트 행, 부분 행, 빈 출력, 알려진 live 패턴과 Total을 검사합니다.
모든 로그 인자의 일치, 확장 출력의 전체 주소 순서, realloc 확장 tail 및
munmap 전 `0xdd` 계측은 미검증입니다.

M4는 2·4·8 worker에서 각각 120회 할당·확장·축소·해제를 수행합니다.
별도 mutex로 payload 쓰기와 dump 읽기를 동기화하고, 독립 payload의 public
allocator 호출은 동시에 실행합니다. join 후 metadata도 검사합니다.
다른 스레드에 포인터 소유권을 전달하며, 오류 주입 검사는 worker 4개 모두의
손상이 최종 실패로 전달되었을 때만 통과합니다.

M5는 heap 재사용과 debug/scribble 활성화 성장·overflow 회귀를 검사합니다.
통합 실행은 필수 16개도 debug/scribble 활성화 상태에서 반복합니다.
heap 재사용은 metadata로 확인하며 실제 mmap/munmap syscall 수를 계측하지 않습니다.
다른 OS는 미검증입니다.

## 로컬 결과

2026-10-08 Linux 실행 결과:

- M1: 30/30 통과.
- M2: 5/9 통과. 축소 후 병합 관련 기존 실패 4개.
- M3: 4/5 통과. realloc(NULL,n)의 scribble 누락.
- M4: 2/3 통과. realloc 축소 후 인접 free 블록 발견.
- M5: 2/2 통과.
- 경계 테스트: 27/28 통과. realloc(NULL,1)의 scribble 누락.
- 전체 통합: 82/89 통과, 실행 파일 종료 코드 1.

89개 항목에는 M1 runtime 25개, M2–M5 runtime 19개, 경계 28개,
보너스 활성화 필수 16개와 기존 M1 전체 runner를 실행하는 요약 항목 1개가
포함됩니다. 마지막 항목은 내부 검사 30개를 하나로 집계하므로 독립적인
계약 89개를 뜻하지 않습니다. 실패는 실제 실패로 표시하며, 마일스톤 완료나
제출 요구사항 전체 충족을 의미하지 않습니다.
