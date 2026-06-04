# 시스템 테스트 변경 이력

> 변경 태그: `[추가]` / `[삭제]` (~~취소선~~) / `[변경]`
> 기준 버전: v1.0 (Initial commit) → v2.0

---

## 전체 요약

| 파일 | 상태 | 시나리오 수 | 비고 |
|------|------|:-----------:|------|
| `SystemTests.cpp` | `[추가]` | 30 (POS 10 + NEG 20) | v2.0 핵심 시나리오 |
| `SystemTests_v2.cpp` | `[추가]` | 30 (POS 10 + NEG 20) | UP팀 제공 시나리오 |

> v1.0에는 시스템 테스트 파일이 존재하지 않았다. 두 파일 모두 v2.0에서 신규 추가.

---

## SystemTests.cpp — v2.0 핵심 시나리오

### 긍정 시나리오 (정상 흐름)

| ID | 시나리오 | 검증 내용 |
|----|----------|----------|
| ST-P01 | 자동 청소 시작 | `startCleaning(NORMAL)` + `moveForward()` 호출 순서 |
| ST-P02 | 전방 장애물 감지 | `stopCleaning()` + `stop()` 호출 |
| ST-P03 | 장애물 회피 우측 | 좌측 막힘·우측 여유 시 `turn(RIGHT)` → `startCleaning` → `forward` 완전 시퀀스 |
| ST-P04 | 장애물 회피 좌측 | 좌측 여유 시 `turn(LEFT)` → `startCleaning` → `forward` 완전 시퀀스 |
| ST-P05 | 좌·우 모두 여유 | LEFT 우선 선택 (v2.0 정책 검증) |
| ST-P06 | 삼면 장애물 후 우측 전진 | E1 진입 후 후진 중 우측 여유 확보 시 `turn(RIGHT)` → 전진 |
| ST-P07 | 삼면 장애물 후 좌측 전진 | E1 진입 후 후진 중 좌측 여유 확보 시 `turn(LEFT)` → 전진 |
| ST-P08 | 삼면 장애물 — 좌우 반복 막힘 후 방향 확보 | 여러 번 막힘 후 최종 탈출 |
| ST-P09 | 먼지 감지 HIGH 전환 | `startCleaning(HIGH)` 호출 확인 |
| ST-P10 | 먼지 감지 후 타이머 만료 | NORMAL 자동 복귀 (실제 타이머 대기) |

### 부정 시나리오 (비정상 입력·중복 호출 무시 검증)

| ID | 시나리오 | 검증 내용 |
|----|----------|----------|
| ST-N01 | IDLE에서 `sideStatus(LEFT, true)` | 무시 — 모터 미호출 |
| ST-N02 | IDLE에서 `sideStatus(LEFT, false)` | 무시 — 모터 미호출 |
| ST-N03 | CLEANING에서 `sideStatus(LEFT, true)` | 무시 — CLEANING 상태는 sideStatus 처리 안 함 |
| ST-N04 | CLEANING에서 `sideStatus(LEFT, false)` | 무시 |
| ST-N05 | REVERSING에서 좌우 막힘 | `stop` / `turn` / `forward` 미호출 |
| ST-N06 | REVERSING에서 좌우 막힘 3회 | `forward` 0회 확인 |
| ST-N07 | `dustDetected` 2회 | `startCleaning(HIGH)` 1회만 호출 (중복 boost 무시) |
| ST-N08 | `dustDetected` | DriveMotor 호출 전혀 없음 확인 |
| ST-N09 | `frontObstacleDetected` | `startCleaning` 미호출 확인 |
| ST-N10 | E1 진입 | `startCleaning` 미호출 (`allSidesBlocked` 제거 후 내부 판정 검증) |
| ST-N11 | 좌·우 모두 여유 | `turn` 1회, LEFT 선택 |
| ST-N12 | UC-002 호출 순서 | `stopCleaning` → `stop` 순서 보장 |
| ST-N13 | UC-002 호출 순서 | `turn` → `startCleaning(NORMAL)` 순서 보장 |
| ST-N14 | UC-003 호출 순서 | `forward` → `startCleaning(NORMAL)` 순서 보장 |
| ST-N15 | `start()` 중복 — CLEANING 상태 | 무시 |
| ST-N16 | `start()` 중복 — REVERSING 상태 | 무시 |
| ST-N17 | E1 진입 | `stopCleaning` 1회, `moveBackward` 1회 확인 |
| ST-N18 | REVERSING 중 `frontObstacleDetected` | 재감지 무시 — 후진 유지 |
| ST-N19 | 장애물 2회 연속 | `turn` 총 2회 |
| ST-N20 | 고출력 중 장애물 감지 | `stop` 1회, 마지막 모터 호출이 NORMAL |

---

## SystemTests_v2.cpp — UP팀 제공 시나리오

### 긍정 시나리오 (정상 흐름)

| ID | 시나리오 | 검증 내용 |
|----|----------|----------|
| TC-POS-01 | 좌측 회피 전체 흐름 | 전방 장애물 → 좌측 여유 → `turn(LEFT)` → 재개 전체 시퀀스 |
| TC-POS-02 | 고먼지 집중 청소 → 타이머 만료 → NORMAL 복귀 | HIGH → 자동 NORMAL 복귀 |
| TC-POS-03 | 후진 후 좌측 탈출 | 삼면 막힘 → 후진 → 좌측 여유 시 탈출 |
| TC-POS-04 | 좌측 막힘·우측 뚫림 → `turn(RIGHT)` | 좌측 막힘 시 우측 선택 정책 |
| TC-POS-05 | 집중 청소 복귀 후 장애물 → `turn(RIGHT)` | NORMAL 복귀 후 장애물 발생 시나리오 |
| TC-POS-06 | 2회 연속 회피 (1차 LEFT·2차 RIGHT) | 회피 방향 교대 시나리오 |
| TC-POS-07 | 2회 연속 집중 청소 → 각 NORMAL 복귀 | boost 후 NORMAL 복귀 2회 반복 |
| TC-POS-08 | 저먼지 미발생 → 일반 청소 유지 | `dustDetected` 미호출 시 NORMAL 유지 |
| TC-POS-09 | 전체 순차: 먼지 → 장애물 → 후진 → 탈출 | 복합 시나리오 end-to-end |
| TC-POS-10 | 전방 개방 → 직진 유지 | sideStatus 없을 때 forward 유지 |

### 부정 시나리오 (guard·경계값·중복 무시 검증)

| ID | 시나리오 | 검증 내용 |
|----|----------|----------|
| TC-NEG-01 | CLEANING 중 `start()` 중복 | 무시 — 모터 추가 호출 없음 |
| TC-NEG-02 | `sideStatus` 무시 확인 | 부적절한 상태에서 sideStatus 처리 없음 |
| TC-NEG-03 | `changeState(999)` | 범위 외 값 무시 |
| TC-NEG-04 | AVOIDING 중 `start()` | 무시 |
| TC-NEG-05 | 동일 상태 `changeState(CLEANING)` | 동일 상태 전이 무시 |
| TC-NEG-06 | FORWARD 중 `turn()` | MotorState guard — 주행 중 전환 거부 |
| TC-NEG-07 | AVOIDING 중 장애물 재감지 | 후진 유지 — 재감지 무시 |
| TC-NEG-08 | FORWARD → `moveForward()` 중복 | MotorState guard — 중복 호출 무시 |
| TC-NEG-09 | BACKWARD → `moveBackward()` 중복 | MotorState guard — 중복 호출 무시 |
| TC-NEG-10 | LEFT → `turn(LEFT)` 중복 | MotorState guard — 중복 방향 무시 |
| TC-NEG-11 | RIGHT → `turn(RIGHT)` 중복 | MotorState guard — 중복 방향 무시 |
| TC-NEG-12 | STOPPED → `stop()` 중복 | MotorState guard — 중복 호출 무시 |
| TC-NEG-13 | 음수 `dustLevel` | `dustDetected(float)` 음수 입력 무시 |
| TC-NEG-14 | `FLT_MAX` dustLevel | 최대값 입력 시 크래시 없음 확인 |
| TC-NEG-15 | OFF → `stopCleaning()` | active_ guard — 비활성 상태에서 중복 무시 |
| TC-NEG-16 | NORMAL → `startCleaning()` | active_ guard — 이미 활성 시 중복 무시 |
| TC-NEG-17 | BOOST → `boostCleaning()` | 이미 부스트 중일 때 모터 재호출 없음 |
| TC-NEG-18 | NORMAL → `normalizePower()` | 부스트 상태가 아닐 때 무시 |
| TC-NEG-19 | OFF → `normalizePower()` | 비활성 상태에서 무시 |
| TC-NEG-20 | OFF → `boostCleaning()` | active_ guard — 비활성 상태에서 거부 |
