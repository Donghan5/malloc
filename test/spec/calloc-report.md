# Linux calloc 검증

실행: `make test-bonus` (C01~C08 및 기존 B01~B04), `make check` (실행기 fixture).
최종 실행은 모두 종료 코드 0. B 4개, C 8개 PASS. fixture 12개 PASS는 기능 통과 수에 합산하지 않는다.

| 항목 | 구현 위치 | 결과 |
|---|---|---|
| C01 | calloc_main.c ordinary / calloc_common.c calloc_sample | PASS: 7개 조합, 요청 영역만 unsigned char로 검사·쓰기·free |
| C02 | calloc_common.c calloc_reuse | PASS: TINY의 양쪽 live guard 유지, 오염 대상 주소 실제 재사용 |
| C03 | calloc_common.c calloc_overflow / ../eval/calloc.sh | PASS: volatile SIZE_MAX/2+1 및 +2 × 2 (wrap 0, 2), NULL, heap anchor 보존, debug에서 overflow의 malloc 진입 없음, 정상 호출 복구 |
| C04 | calloc_failure.c | PASS: fork 자식 RLIMIT_AS soft limit 최대 64 MiB 설정·재조회, 유효한 128 MiB 요청 NULL, 제한 복구 후 정상 호출 |
| C05 | calloc_common.c calloc_zero | PASS: malloc(0)과 세 조합 모두 NULL 정책 일치, 비NULL 경로에서도 역참조 없이 free |
| C06 | ../eval/calloc.sh / calloc_sample | PASS: 미설정·debug·scribble·동시 활성화 각각 새 프로세스, 호출 인자와 동적 주소 로그 일치 |
| C07 | calloc_threads.c | PASS: barrier 시작, 4×80회 및 debug 4×2회, 요청 전체 0 및 기록 보존, join 후 C03/C04 재검사 |
| C08 | ../eval/calloc.sh calloc_binding | PASS: nm 공개 정의, LD_DEBUG 실제 project ELF binding, objdump 지역 호출 그래프에서 calloc 재호출 없음 |

C07은 pthread_create의 내부 할당이 환경 캐시를 먼저 초기화할 수 있어, 모든 worker가 barrier 앞에 있는 동안 테스트가 g_data.initialized를 0으로 되돌린 뒤 동시에 출발시킨다. allocator 소스 변경은 없으며, 유한 workload 통과는 race 부재의 증명이 아니다. 모든 실행은 기존 run_command의 30초 timeout, TERM 후 2초 kill 적용을 받는다. exit 77은 검증 불가, signal/timeout/파싱 실패는 PASS가 아니다.

보너스 단독 실행은 공식 평가 자료 승인과 독립적으로 격리된 제출 Makefile 빌드를 수행한다. 공식 적격·점수는 인정하지 않는다. mandatory test0~test5 및 test-edge 실행 역할은 변경하지 않았다. overflow의 calloc 사례는 보너스에만 있다.

실제 제출 옵션은 기존 `-Wall -Wextra -Werror -fPIC -O0 -g`, shared link를 유지했다. 테스트만 `-fno-builtin`을 쓴다. 실제 ELF 및 전체 disassembly와 진단 스크립트는 각 evaluation 디렉터리에 보존한다. binding 진단은 기능·fault 측정과 별도 실행한다. 결과·원본 stdout/stderr·명령·종료코드·입력 소스 해시는 evidence.json에 보존한다.

Norminette는 PATH에 없어 미실행. 신규 C/헤더는 42 헤더, while 사용, 함수당 25줄 이내, 파일당 5함수 이내, 인자/지역변수 수, 80열, 선언·대입 분리 및 들여쓰기를 수동 확인했다. 자동 Norm 인증은 검증 불가다.

AGENTS.md는 프로젝트 및 검색한 상위 경로에서 발견되지 않았다. calloc, malloc/free, 초기화/debug/scribble, heap/block, Makefile, 기존 보너스와 테스트 명세를 검토했다. calloc은 overflow → public malloc → NULL 확인 → mutex debug → 요청 바이트 memset → 반환 순서다. malloc은 반환 전에 mutex를 풀고, calloc의 로그 잠금과 중첩하지 않는다. 초기화는 malloc 잠금 안, scribble 뒤 calloc이 최종 0으로 덮는다. allocator 동작 결함은 이번 검증에서 발견되지 않았다. errno와 추가 점수는 판정하지 않는다.

변경 파일: calloc_*.c 4개, calloc_spec.h, calloc-report.md, ../eval/calloc.sh 추가. bonus.c의 기존 has_pattern을 calloc_common.c로 옮겨 공유한다. ../eval/runner.sh, ../eval/common.sh와 루트 Makefile help를 통합 용도로 수정했다. 기존 사용자 변경과 allocator 파일은 보존했으며 commit/push하지 않았다.
