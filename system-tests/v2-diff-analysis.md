# UP팀 시나리오 vs 바이브코딩 시나리오 차이 분석

## 실행 결과 요약

| 구분 | 전체 | PASS | FAIL (로직 차이) | FAIL (인터페이스 없음) |
|------|------|------|-----------------|----------------------|
| Positive (TC-POS-01~10) | 10 | 10 | 0 | 0 |
| Negative (TC-NEG-01~20) | 20 | 1 | 3 | 16 |
| **합계** | **30** | **11** | **3** | **16** |

---

## FAIL 시나리오 상세 — 로직 차이

### TC-NEG-01: CLEANING 중 startCleaning 중복 호출

**오류 메시지**
```
moveForward 1회만 (두 번째 무시 기대): expected 1 got 2
```

**UP팀 기대 동작**
- `startCleaning()` × 2 호출 시 두 번째 호출을 무시한다.
- 결과: `Motor=FORWARD`, `startCleaning` 1회

**이 레포 실제 동작**
- `start()`는 내부 상태(`state_`)를 확인하지 않고 항상 실행된다.

```cpp
// RVCController.cpp
void RVCController::start() {
    cleaning_.startCleaning();   // 상태 체크 없이 항상 실행
    nav_.moveForward();
    state_ = State::CLEANING;
}
```

**왜 실패하는가**
`RVCController::start()`에 guard 조건이 없어서 `CLEANING` 상태에서도 두 번째 호출이 그대로 실행된다. `moveForward`가 2회 발생한다.

**수정 방법**
```cpp
void RVCController::start() {
    if (state_ == State::CLEANING) return;  // 중복 방지
    cleaning_.startCleaning();
    nav_.moveForward();
    state_ = State::CLEANING;
}
```

---

### TC-NEG-04: AVOIDING 중 startCleaning 호출

**오류 메시지**
```
forward 없음 (무시 기대): expected 0 got 1
```

**UP팀 기대 동작**
- `AVOIDING` 상태에서 `startCleaning()` 호출 시 무시한다.
- 결과: `Motor=BACKWARD` 유지, `startCleaning` 추가 없음

**이 레포 실제 동작**
- `REVERSING` 상태에서 `start()`를 호출해도 guard 없이 실행된다.
- `moveForward`가 1회 발생하고 상태가 `CLEANING`으로 강제 전환된다.

**왜 실패하는가**
`start()`에 `REVERSING`/`AVOIDING` 상태 guard가 없다. UP팀은 상태 전이 규칙상 `AVOIDING` 중에는 외부 `startCleaning` 명령을 거부해야 한다고 정의한다.

**수정 방법**
```cpp
void RVCController::start() {
    if (state_ == State::CLEANING ||
        state_ == State::REVERSING ||
        state_ == State::REVERSING_CHECKING_RIGHT) return;  // 회피 중 무시
    cleaning_.startCleaning();
    nav_.moveForward();
    state_ = State::CLEANING;
}
```

---

### TC-NEG-07: 회피 중 전방 장애물 재감지

**오류 메시지**
```
stop 추가 없음 (회피 유지 기대): expected 0 got 1
```

**UP팀 기대 동작**
- `AVOIDING(REVERSING)` 상태에서 전방 장애물이 재감지되면 기존 후진 동작을 유지한다.
- 결과: `Motor=BACKWARD`, `AVOIDING` 상태 유지, `stop` 추가 없음

**이 레포 실제 동작**
- `frontObstacleDetected()`는 `state_`에 무관하게 항상 `stopCleaning + stop + STOPPED` 전환을 수행한다.

```cpp
// RVCController.cpp
void RVCController::frontObstacleDetected() {
    cleaning_.stopCleaning();  // 상태 체크 없이 항상 실행
    nav_.stop();
    state_ = State::STOPPED;
}
```

- `REVERSING` 중 재호출 시 `stop()`이 추가로 발생하고 상태가 `STOPPED`로 전환된다.

**왜 실패하는가**
`frontObstacleDetected()`에 `REVERSING` 상태 guard가 없어서, 이미 후진 중임에도 `stop()` + 상태 변경이 발생한다. UP팀 설계에서는 회피 중 장애물 재감지 시 기존 동작을 유지하도록 설계되어 있다.

**수정 방법**
```cpp
void RVCController::frontObstacleDetected() {
    if (state_ == State::REVERSING ||
        state_ == State::REVERSING_CHECKING_RIGHT) return;  // 회피 중 재감지 무시
    cleaning_.stopCleaning();
    nav_.stop();
    state_ = State::STOPPED;
}
```

---

## FAIL 시나리오 상세 — 인터페이스/파라미터 없음

아래 16개 시나리오는 UP팀과 이 레포의 인터페이스 설계 차이로 직접 테스트가 불가능하다. 해당 인터페이스가 이 레포에 존재하지 않음을 명시하는 오류 메시지와 함께 FAIL로 처리한다.

| TC | 오류 원인 | 없는 인터페이스 |
|----|-----------|----------------|
| TC-NEG-03 | 인터페이스 없음 | `changeState(int)` |
| TC-NEG-05 | 인터페이스 없음 | `changeState(int)` |
| TC-NEG-06 | 인터페이스 없음 | `turnLeft()`, `turnRight()` 직접 호출 |
| TC-NEG-08 | 인터페이스 없음 | `moveForward()` 직접 호출 |
| TC-NEG-09 | 인터페이스 없음 | `moveBackward()` 직접 호출 |
| TC-NEG-10 | 인터페이스 없음 | `turnLeft()` 직접 호출 |
| TC-NEG-11 | 인터페이스 없음 | `turnRight()` 직접 호출 |
| TC-NEG-12 | 인터페이스 없음 | `stopMotor()` 직접 호출 |
| TC-NEG-13 | 파라미터 없음 | `dustDetected(float dustLevel)` |
| TC-NEG-14 | 파라미터 없음 | `dustDetected(float dustLevel)` |
| TC-NEG-15 | 인터페이스 없음 | `deactivateCleaner()` |
| TC-NEG-16 | 인터페이스 없음 | `activateCleaner()` |
| TC-NEG-17 | 인터페이스 없음 | `boostPower()` |
| TC-NEG-18 | 인터페이스 없음 | `normalizePower()` |
| TC-NEG-19 | 인터페이스 없음 | `normalizePower()` (OFF 상태) |
| TC-NEG-20 | 인터페이스 없음 | `boostPower()` (OFF 상태) |

**TC-NEG-02 특이 사항**
UP팀의 `resumeCleaning()`은 이 레포에 없으나, `CLEANING` 상태에서 `sideStatus()`가 무시된다는 동일한 원칙을 유사 검증으로 대체하여 PASS 처리하였다.

---

## 인터페이스 설계 차이 총정리

| 항목 | UP팀 | 이 레포 |
|------|------|---------|
| 센서 입력 방식 | `SensorData{frontSensorData, leftSensorData}` 한 번에 2개 필드 push | `frontObstacleDetected()` + `sideStatus(Direction, bool)` 별도 이벤트 |
| 청소 시작/재개 | `startCleaning()`, `resumeCleaning()` 분리 | `start()` 하나로 통합 |
| 상태 직접 변경 | `changeState(int)` 존재 | 없음 (상태는 내부에서만 변경) |
| Motor 직접 명령 | `turnLeft()`, `turnRight()`, `moveBackward()`, `stopMotor()` 직접 호출 | 없음 (RVCController 이벤트로만 간접 제어) |
| Cleaner 직접 명령 | `activateCleaner()`, `deactivateCleaner()`, `boostPower()`, `normalizePower()` 직접 호출 | 없음 (`dustDetected()` 이벤트로만 제어) |
| 먼지 수준 | `dustLevel: float` 파라미터 + 임계값 비교 로직 | `dustDetected()` 이벤트 기반 (호출되면 항상 boost) |
| 상태 노출 | `SystemState`, `Motor`, `Cleaner` 상태 직접 확인 가능 | 없음 (모터 stub 호출 수로 간접 확인) |
| 우측 장애물 감지 | 두 번째 push의 `frontSensorData`로 우측 상태 판단 | `sideStatus(RIGHT, bool)` 명시적 호출 |

---

## 로직 차이 요약 (수정이 필요한 항목)

| TC | 로직 차이 | 수정 방향 |
|----|-----------|-----------|
| TC-NEG-01 | `start()` CLEANING 상태 guard 없음 | `if (state_ == CLEANING) return` 추가 |
| TC-NEG-04 | `start()` AVOIDING/REVERSING 상태 guard 없음 | `if (state_ == REVERSING ...) return` 추가 |
| TC-NEG-07 | `frontObstacleDetected()` REVERSING 상태 guard 없음 | `if (state_ == REVERSING ...) return` 추가 |

세 항목 모두 **상태 guard 누락** 문제로, `RVCController`의 각 퍼블릭 메소드 진입부에서 현재 `state_`를 확인하여 유효하지 않은 전이를 거부하는 로직이 필요하다.
