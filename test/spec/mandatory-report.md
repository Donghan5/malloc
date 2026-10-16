# Mandatory Linux 검증 보고 (2026-10-16)

AGENTS.md는 저장소·상위 경로에서 발견되지 않았다. 최신 eval-sheet.pdf (8쪽), malloc-test-specification.md, manifest와 기존 구현을 먼저 확인했다. PDF 도구가 없어 PDF의 Flate content stream을 로컬 Python 표준 라이브러리로 해제해 평가 문구를 읽었다.

## 변경 파일

- 추가: `zones.c`, `zones_main.c`, `zones.h`, `../eval/zones.sh`: 기존 계산 전용 zone 검사를 확장·이동. test1의 별도 프로세스에서 실제 heap 소속·비중첩·요청 전체 패턴·페이지 배수 확인.
- 추가: `probe.h`, `probe_data.c`, `probe_alignment.c`; 수정: `probe.c`: 기존 정렬/overflow 검사 helper 분리, 중복 E05 제거. 파일당 5함수 제한 유지.
- 수정: `../eval/runner.sh`, `../eval/common.sh`: 한국어 설명→로그→결과 UI·요약, 내부 ID 숨김, 점수 별도 표시, zone을 공통 선행 조건에서 제거, test2의 잘못 연결된 calloc 실행 제거, 원시 증거 및 계산 근거 보존, 선행 중단 시 각 선택 항목 미실행 표시.
- 수정: `../eval/check_runner.sh`: UI·부분 점수·미확정 해석·zone parser·선행 중단·timeout/signal fixture 보완.
- 수정: 루트 `Makefile`의 test1 help 문구만 변경. `../README.md`, 루트 `malloc-test-specification.md` 계약 갱신. 이 보고서 추가.

src/inc 16개 파일은 작업 시작 해시와 전부 동일하다. 기존 사용자 변경을 보존했고 commit/push하지 않았다. allocator 옵션·HOSTTYPE·라이브러리 이름 계약 및 공식 지정 원본은 변경하지 않았다.

## 실제 실행

루트에서 `make test1`, `make test2`, `make test0`, `make test3`, `make test4`, `make test5`, `make test-edge`를 실행했다. 모두 PASS 및 make 종료 0. `make check`의 fixture 18개 PASS와 `shellcheck -s bash -x -P SCRIPTDIR test/eval/*.sh` 종료 0은 실제 기능 통과 수에 합산하지 않는다.

최종 test1 원시값: S0=65, S1=324, U0=70, U1=337. 시스템 증가량=259, 프로젝트 증가량=267, 추가량 O=8, 운영 해석 점수=5/5. 시스템 측정은 LD_PRELOAD 없이, 프로젝트 측정만 실제 라이브러리를 주입했다. GNU time -v, LC_ALL=C를 사용하며 각 조건 1회다. 수정 검증 실행도 이전 실패·출력을 삭제하지 않고 별도 evidence에 보존했다.

기존 manifest는 baseline 보정 운영 해석을 채택했다고 기록한다. 점수는 그 기록에 따라 계산하며 평가표의 명시적 수식이라 주장하지 않는다. 기록이 없거나 다른 수식이면 후보 O만 남기고 점수 미확정·검증 불가·실행기 종료 1이다. minor faults는 누적 사건 수이며 현재 상주 페이지 수가 아니다. 음수를 보정하지 않으며 fault 차이만으로 메모리 손상을 입증하지 않는다.

| 분류 | 최대 요청 | 실제 zone | 페이지 | 계산 용량 | 실제 동일 zone 유지 | 결과 |
|---|---:|---:|---:|---:|---:|---|
| TINY | 128 | 16384 | 4096 | 102 | 100/100 | PASS |
| SMALL | 1024 | 131072 | 4096 | 124 | 100/100 | PASS |

계산: `floor((zone_size - 48)/(align16(maximum) + 32))`. 현재 최대 요청은 16바이트 정렬을 해도 같은 분류에 남는 최대 크기다. heap 48바이트와 block 32바이트를 반영한다. split은 잔여 공간이 block header+16 이상일 때만 수행하고 작은 tail은 기존 block에 포함된다. actual heap.total_size % sysconf page ==0을 확인한다. LARGE에는 100개 조건을 적용하지 않는다. `evidence.json.zones`에 공식·정렬·metadata·배치 정책·소스 경로를, records에 실제 출력을 보존했다.

프로세스 진입 전 할당이 있으면 기존 heap의 할당을 live warmup으로 유지하며 새 heap의 첫 block을 식별한다. 별도 선행 할당 진단에서 TINY warmup=101, SMALL=123 이후 새 zone 각각 100/100과 데이터 보존을 확인했다. 이 진단은 공식 검사 건수와 분리한다.

최종 test2: baseline=70, 반복 할당=337, malloc/free=73; free 기능 PASS, 품질 차이=3≤10 PASS. calloc은 보너스에서만 실행한다.

## 출력과 종료 코드 검증

실제 출력 예:

```text
설명: 반복 malloc/free의 minor faults 증가가 baseline 대비 10 이하인지 검사합니다.
로그: baseline=70, 반복 할당=337, malloc/free=73; 차이=3, 허용=10 이하
결과: PASS
```

fixture로 FAIL의 기대값·실제값·재현 명령·로그 파일, 검증 불가와 미실행 사유, 부분 점수 3/5의 측정 PASS/종료 0, 미확정 점수의 종료 1, signal/timeout/파싱 실패의 비PASS를 확인했다. 선행 조건 중단은 선택 검사별 미실행을 남긴다. 실행기 종료 1을 make가 자체 종료 2로 전달하는 계약을 유지한다.

## 미검증·한계

Norminette는 PATH에 없어 미실행. 변경·추가 C/헤더 7개에 대해 42 헤더, 80열, 함수당 25줄, 파일당 5함수, 인자·지역변수 수, 선언/대입 분리, 들여쓰기와 while 사용을 수동 확인했다. 자동 Norm 인증은 미검증이다.

공식 지정 test0/test1/run_linux.sh는 기존 manifest SHA-256에 일치했다. CDN 원본과의 독립 동일성 재검증은 하지 않았다. 원본 launcher는 DYLD 변수를 사용하므로 원본은 보존하고 기존 Linux LD_PRELOAD adapter를 사용한다. 사용자 지정 test3 문자열·test4/test5 역할과 평가표의 기존 차이는 README에 남겨두었다. 현재 검사는 전체 과제 통과·race 부재·공식 총점을 보장하지 않는다. 이번 범위에서 allocator 결함은 발견되지 않았다.

## 증거

- `test1`: [증거](../evaluation-5R8NxS9Q/evidence.json), [출력](../evaluation-5R8NxS9Q/output.txt)
- `test-edge`: [증거](../evaluation-lA7OwDNd/evidence.json), [출력](../evaluation-lA7OwDNd/output.txt)
- `test5`: [증거](../evaluation-IU8BGaAv/evidence.json), [출력](../evaluation-IU8BGaAv/output.txt)
- `test3`: [증거](../evaluation-YLavwsib/evidence.json), [출력](../evaluation-YLavwsib/output.txt)
- `test0`: [증거](../evaluation-973iQ3dW/evidence.json), [출력](../evaluation-973iQ3dW/output.txt)
- `test2`: [증거](../evaluation-ycCGqhqS/evidence.json), [출력](../evaluation-ycCGqhqS/output.txt)
- `test4`: [증거](../evaluation-gHuYBgzx/evidence.json), [출력](../evaluation-gHuYBgzx/output.txt)

- 선행 할당 진단: [원본 records](../evaluation-5R8NxS9Q/zone-warm-diagnostic.jsonl)
- 상태·종료 코드 fixture: [실행 출력](../evaluation-5R8NxS9Q/runner-fixtures.txt)
- 평가표 SHA-256: `5668a8aeba4250dd32ec4409c61b569c804a77f62204b709677fba0527aafe41`
