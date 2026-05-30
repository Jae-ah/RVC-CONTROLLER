# 현재 작업: v2.0 — 아래 작업 지시를 최우선으로 따른다

## v2.0 작업 지시 (오른쪽 센서 삭제)

### 변경 요약
- 오른쪽 전용 센서를 삭제한다. 우측 장애물 감지는 전방 센서 회전 스캔으로 대체한다.
- 장애물 회피 방향은 랜덤 선택에서 **좌측 우선**으로 변경한다. (좌측이 막힌 경우에만 우측)

### 작업 규칙
1. 기존 파일을 읽고 수정한다. 새 파일을 만들지 않는다.
2. 수정된 모든 `.md` 파일에는 변경 내용을 아래 태그로 빠짐없이 표시한다.
   - `[추가]` — 새로 추가된 내용
   - `[삭제]` — 제거된 내용 (삭제된 원문을 ~~취소선~~으로 남긴다)
   - `[변경]` — 기존 내용이 수정된 경우
---

> 이 구분선 아래는 v1.0 기준 문서다. 위 v2.0 작업 지시와 충돌 시 **v2.0을 우선 적용**한다.

---

# RVC SW Controller 프로젝트

## 프로젝트 개요

로봇 청소기(RVC) SW Controller를 OOAD 프로세스로 개발한다.

## 요구사항

- RVC는 가정의 표면을 자동으로 청소하고 닦는다
- 청소하면서 직진한다
- 전방 센서가 장애물을 감지하면 청소를 멈추고, 좌/우로 방향을 틀어 청소하며 전진한다
- 앞/좌/우 모두 장애물이 있으면 후진 후 좌/우로 방향을 틀어 전진한다
- 먼지를 감지하면 잠시 청소 출력을 높인다
- HW 제어의 상세 설계 및 구현은 고려하지 않는다
- 자동 청소 기능에만 집중한다

## 폴더 구조

```
arch/requirements/     - 요구사항 문서
arch/usecases/         - Use Case 문서
arch/analysis/ssd/     - SSD 다이어그램
arch/analysis/domain/  - 도메인 모델
arch/design/SD/        - 시퀀스 다이어그램
arch/design/class/     - 클래스 다이어그램
arch/system-tests/     - 시스템 테스트
src/                   - 소스 코드
tests/                 - 테스트 코드
simulator/             - RVC 동작 시뮬레이터 (유일하게 객체지향으로 개발하지 않아도 됨)
```

## 진행 단계

1. 요구사항 분석: `arch/system.md`, `arch/requirements/fr-nfr.md`
2. OOA: `arch/usecases/usecases.md` → `arch/usecases/UC-nnn.md` → `arch/analysis/ssd/ssd.md` → `arch/analysis/domain/domain.md`
3. OOD: `arch/design/SD/SD.md` → `arch/design/class/class-diagram.md`
4. OOI: `src/` → `tests/` → `arch/system-tests/system-tests.md` → `simulator/`

## 설계 방향

- 구동 모터, 청소 모터는 외부 액터로 취급한다
- 시스템 내부에는 모터를 제어하는 별도 컨트롤러 객체를 둔다 (예: MotorController, CleaningController)
- [추가] 오른쪽 전용 센서는 존재하지 않는다. 우측 장애물 감지는 전방 센서를 이용한 회전 스캔으로 대체한다.


## 규칙

- 모든 다이어그램은 Mermaid 형식으로 작성한다
- 모든 코드는 객체지향 원칙을 철저히 준수한다 (simulator 제외)
- simulator를 통하여 system-tests를 테스트한다
- [변경] ~~장애물 회피 시 좌/우 방향은 랜덤으로 선택한다~~ → 장애물 회피 시 좌측을 우선으로 선택한다. 좌측이 막힌 경우에만 우측으로 전환한다.
- usecases, ssd, domain-model, ssd, class-diagram 은 반드시 통일성을 가질 것
- 구현 언어: C++

## [추가] 변경 이력

| 버전 | 변경 내용 | 날짜 |
|------|----------|------|
| v2.0 | 오른쪽 센서 삭제 — 전방 센서 회전 스캔으로 우측 감지 대체, 장애물 회피 방향 좌측 우선으로 변경 | 2025-05-29 |
