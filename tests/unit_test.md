# 단위 테스트 변경 이력

> 변경 태그: `[추가]` / `[삭제]` (~~취소선~~) / `[변경]`
> 기준 버전: v1.0 (Initial commit) → v2.0

---

## 전체 요약

| 파일 | v1.0 테스트 수 | v2.0 테스트 수 | 변경 |
|------|:--------------:|:--------------:|------|
| `CleaningControllerTest.cpp` | 6 | 6 | 선행 상태 추가 |
| `NavigationControllerTest.cpp` | 8 | 7 | 시그니처 변경 반영, 신규 추가 |
| `RVCControllerTest.cpp` | 16 | 26 | 대폭 재작성 |
| **합계** | **30** | **39** | |

---

## CleaningControllerTest.cpp

### 변경 배경

`CleaningController`에 `active_` 상태 추적 및 guard 로직이 추가되면서,
비활성 상태에서 `stopCleaning()` / `boostCleaning()`을 호출하면 guard에 막혀
모터가 실제로 호출되지 않는다. 기존 테스트는 선행 상태(precondition) 없이
메서드를 바로 호출하고 있었으므로 수정이 필요했다.

### 변경 내용

| 구분 | 테스트 | 내용 |
|------|--------|------|
| `[변경]` | `stopCleaning_callsMotorStop` | `controller.startCleaning()` 선행 호출 추가 — `active_=false → true` |
| `[변경]` | `boostCleaning_whenIdle_switchesToHighLevel` | `controller.startCleaning()` 및 `motor.startCalls.clear()` 선행 추가 |
| `[변경]` | `timerExpiry_restoresNormal_andAllowsSubsequentBoost` | `controller.startCleaning()` 선행 호출 추가 |

### 변경 없음

| 테스트 | 설명 |
|--------|------|
| `startCleaning_callsMotorWithNormalLevel` | NORMAL 레벨로 모터 호출 검증 |
| `boostCleaning_whenAlreadyBoosting_doesNotCallMotorAgain` | 이미 부스트 중일 때 모터 재호출 없음 검증 |
| `onExpired_restoresNormalLevel` | 타이머 만료 시 NORMAL 복귀 검증 |

---

## NavigationControllerTest.cpp

### 변경 배경

v2.0에서 `turn(const std::vector<Direction>&)` 시그니처가 `turn(Direction)` 단일 방향으로
변경되고, `rotateRight()` / `rotateLeft()` 메서드가 추가됐다. 이에 따라 벡터 기반 테스트를
삭제하고 단일 방향 기반으로 재작성했다.

또한 `MotorState` guard 추가로 인해 `stop_delegatesToMotor`가 초기 STOPPED 상태에서
`stop()`을 호출하면 guard에 막히게 되어 선행 상태 설정이 필요해졌다.

### 변경 내용

| 구분 | 테스트 | 내용 |
|------|--------|------|
| `[삭제]` | ~~`turn_emptyList_doesNotCallMotor`~~ | 벡터 기반 — 빈 목록 시 모터 미호출 검증. 시그니처 변경으로 삭제 |
| `[삭제]` | ~~`turn_singleLeft_turnsLeft`~~ | 벡터 기반 단일 LEFT 검증. 삭제 후 아래로 대체 |
| `[삭제]` | ~~`turn_singleRight_turnsRight`~~ | 벡터 기반 단일 RIGHT 검증. 삭제 후 아래로 대체 |
| `[삭제]` | ~~`turn_multipleDirections_picksExactlyOne`~~ | 랜덤 방향 선택 검증. v2.0에서 랜덤 제거로 삭제 |
| `[삭제]` | ~~`turn_calledTwice_motorCalledTwice`~~ | 2회 연속 turn 검증. MotorState guard로 동작 변경되어 삭제 |
| `[추가]` | `turn_left_delegatesToMotor` | `turn(Direction::LEFT)` 단일 방향 위임 검증 |
| `[추가]` | `turn_right_delegatesToMotor` | `turn(Direction::RIGHT)` 단일 방향 위임 검증 |
| `[추가]` | `rotateRight_delegatesToMotor` | `rotateRight()` → 모터 위임 검증 |
| `[추가]` | `rotateLeft_delegatesToMotor` | `rotateLeft()` → 모터 위임 검증 |
| `[변경]` | `stop_delegatesToMotor` | `controller.moveForward()` 선행 추가 — `motorState_: STOPPED → FORWARD` |

### 변경 없음

| 테스트 | 설명 |
|--------|------|
| `moveForward_delegatesToMotor` | `moveForward()` → 모터 위임 검증 |
| `moveBackward_delegatesToMotor` | `moveBackward()` → 모터 위임 검증 |

---

## RVCControllerTest.cpp

### 변경 배경

v2.0에서 다음이 변경되었다:
- `sideStatus(const vector<Direction>&)` → `sideStatus(Direction, bool)` 시그니처 변경
- `allSidesBlocked()` 오퍼레이션 삭제 (내부 상태머신으로 대체)
- STOPPED, STOPPED_CHECKING_RIGHT, REVERSING_CHECKING_RIGHT 상태 추가
- `rotateRight()` / `rotateLeft()` 호출 흐름 추가
- 회피 방향 결정이 랜덤 → 좌측 우선으로 변경

이에 따라 기존 16개 테스트 전반이 재작성되고 10개가 추가됐다.

### 삭제

| 테스트 | 삭제 이유 |
|--------|----------|
| ~~`sideStatus_whenStopped_turnsToAvailableDirection`~~ | 벡터 기반 시그니처 → 방향별 2단계 호출로 대체 |
| ~~`sideStatus_whenStopped_restartsCleaning`~~ | 동일 |
| ~~`sideStatus_whenStopped_movesForward`~~ | 동일 |
| ~~`allSidesBlocked_stopsCleaning`~~ | `allSidesBlocked()` 오퍼레이션 삭제 |
| ~~`allSidesBlocked_movesBackward`~~ | 동일 |
| ~~`sideStatus_whenReversing_withAvailableDir_stopsMotor`~~ | 벡터 기반 — 좌측 우선 로직으로 전면 재작성 |
| ~~`sideStatus_whenReversing_withAvailableDir_turns`~~ | 동일 |
| ~~`sideStatus_whenReversing_withAvailableDir_movesForward`~~ | 동일 |
| ~~`sideStatus_whenReversing_withAvailableDir_restartsCleaning`~~ | 동일 |
| ~~`sideStatus_whenReversing_withEmptyDir_isIgnored`~~ | 동일 |

### 추가

| 테스트 | 검증 내용 |
|--------|----------|
| `sideStatus_firstCall_rotatesRight` | STOPPED 상태에서 첫 sideStatus(LEFT) 수신 시 rotateRight() 호출 |
| `sideStatus_secondCall_rotatesLeft` | STOPPED_CHECKING_RIGHT에서 sideStatus(RIGHT) 수신 시 rotateLeft() 호출 |
| `sideStatus_leftClear_turnsLeft` | 좌측 여유 시 LEFT 방향으로 회전 |
| `sideStatus_leftClear_restartsCleaning` | 좌측 여유 시 청소 재개 |
| `sideStatus_leftClear_movesForward` | 좌측 여유 시 전진 |
| `sideStatus_leftBlockedRightClear_turnsRight` | 좌측 막힘·우측 여유 시 RIGHT 방향으로 회전 |
| `sideStatus_leftBlockedRightClear_restartsCleaning` | 좌측 막힘·우측 여유 시 청소 재개 |
| `sideStatus_leftBlockedRightClear_movesForward` | 좌측 막힘·우측 여유 시 전진 |
| `sideStatus_bothBlocked_movesBackward` | 좌우 모두 막힘 시 후진 (E1 진입) |
| `sideStatus_bothBlocked_doesNotTurn` | 좌우 모두 막힘 시 turn 미호출 |
| `sideStatus_whenReversing_leftClear_stops` | 후진 중 좌측 여유 시 stop |
| `sideStatus_whenReversing_leftClear_turnsLeft` | 후진 중 좌측 여유 시 LEFT 회전 |
| `sideStatus_whenReversing_leftClear_movesForward` | 후진 중 좌측 여유 시 전진 |
| `sideStatus_whenReversing_leftClear_restartsCleaning` | 후진 중 좌측 여유 시 청소 재개 |
| `sideStatus_whenReversing_leftBlocked_rotatesRight` | 후진 중 좌측 막힘 시 rotateRight() 호출 |
| `sideStatus_whenReversing_rightOnly_turnsRight` | 후진 중 우측만 여유 시 RIGHT 회전 |
| `sideStatus_whenReversing_rightOnly_movesForward` | 후진 중 우측만 여유 시 전진 |
| `sideStatus_whenReversing_rightOnly_restartsCleaning` | 후진 중 우측만 여유 시 청소 재개 |
| `sideStatus_whenReversing_bothBlocked_noTurnOrForward` | 후진 중 좌우 모두 막힘 시 turn/forward 미호출 |
| `sideStatus_whenReversing_bothBlocked_rotatesRightThenLeft` | 후진 중 좌우 막힘 시 rotateRight → rotateLeft 호출 |

### 변경 없음

| 테스트 | 설명 |
|--------|------|
| `start_startsCleaning_atNormalLevel` | 시작 시 NORMAL 레벨로 청소 시작 |
| `start_movesForward` | 시작 시 전진 |
| `frontObstacleDetected_stopsCleaning` | 전방 장애물 감지 시 청소 정지 |
| `frontObstacleDetected_stopsMotor` | 전방 장애물 감지 시 모터 정지 |
| `sideStatus_whenCleaning_isIgnored` | CLEANING 상태에서 sideStatus 무시 |
| `dustDetected_boostsCleaningToHighLevel` | 먼지 감지 시 HIGH 레벨 전환 |
