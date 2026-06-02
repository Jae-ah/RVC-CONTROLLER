# src/ 코드 변경 이력 — v2.0

> 변경 태그: `[추가]` / `[삭제]` (~~취소선~~) / `[변경]`
> 단위 테스트(`tests/`)·시스템 테스트(`system-tests/`) 파일은 별도 관리하며 이 문서에 포함하지 않는다.
> 변경이 없는 파일은 목록 하단 **변경 없음** 절에 나열한다.

---

## IDriveMotor.hpp

| 구분 | 항목 | 내용 |
|------|------|------|
| `[추가]` | `rotateRight() = 0` | 전방 센서를 우측으로 회전시키기 위한 순수 가상 메서드 추가 |
| `[추가]` | `rotateLeft() = 0` | 우측 스캔 후 정면으로 복귀하기 위한 순수 가상 메서드 추가 |

```cpp
// [추가]
virtual void rotateRight() = 0;
virtual void rotateLeft()  = 0;
```

---

## NavigationController.hpp

| 구분 | 항목 | 내용 |
|------|------|------|
| `[삭제]` | ~~`#include <vector>`~~ | `turn()` 시그니처 변경으로 vector 불필요 |
| `[변경]` | `turn(const std::vector<Direction>&)` → `turn(Direction direction)` | 단일 방향 인자로 단순화 |
| `[추가]` | `rotateRight()` | IDriveMotor::rotateRight() 위임 메서드 |
| `[추가]` | `rotateLeft()` | IDriveMotor::rotateLeft() 위임 메서드 |
| `[추가]` | `MotorState` enum | 모터 상태 추적을 위한 내부 열거형 (STOPPED, FORWARD, BACKWARD, LEFT, RIGHT, ROTATING_RIGHT, ROTATING_LEFT) |
| `[추가]` | `MotorState motorState_` | 현재 모터 상태 멤버 변수 (초기값 STOPPED) |

```cpp
// [변경] 이전: void turn(const std::vector<Direction>& directions);
void turn(Direction direction);   // [변경]
void rotateRight();               // [추가]
void rotateLeft();                // [추가]

// [추가]
enum class MotorState {
    STOPPED, FORWARD, BACKWARD, LEFT, RIGHT, ROTATING_RIGHT, ROTATING_LEFT
};
MotorState motorState_ = MotorState::STOPPED;
```

---

## NavigationController.cpp

| 구분 | 항목 | 내용 |
|------|------|------|
| `[삭제]` | ~~`#include <vector>`, `#include <random>`~~ | 랜덤 선택 로직 제거로 헤더 불필요 |
| `[변경]` | `turn()` 구현 | ~~벡터를 받아 랜덤으로 방향 선택~~ → 단일 방향을 `motor_.turn(direction)`에 위임. 중복 호출 및 FORWARD/BACKWARD 중 전환 시도를 거부하는 guard 추가 |
| `[추가]` | `rotateRight()` 구현 | `motor_.rotateRight()` 위임 |
| `[추가]` | `rotateLeft()` 구현 | `motor_.rotateLeft()` 위임 |
| `[변경]` | `moveForward()` 구현 | 이미 FORWARD 상태면 중복 호출 무시 |
| `[변경]` | `moveBackward()` 구현 | 이미 BACKWARD 상태면 중복 호출 무시 |
| `[변경]` | `stop()` 구현 | 이미 STOPPED 상태면 중복 호출 무시 |

```cpp
// [변경] 모든 메서드에 MotorState 추적 및 중복/무효 guard 적용
void NavigationController::moveForward() {
    if (motorState_ == MotorState::FORWARD) return;
    motor_.moveForward();
    motorState_ = MotorState::FORWARD;
}

void NavigationController::stop() {
    if (motorState_ == MotorState::STOPPED) return;
    motor_.stop();
    motorState_ = MotorState::STOPPED;
}

void NavigationController::turn(Direction direction) {
    MotorState target = (direction == Direction::LEFT) ? MotorState::LEFT : MotorState::RIGHT;
    if (motorState_ == target) return;                                          // 중복 guard
    if (motorState_ == MotorState::FORWARD || motorState_ == MotorState::BACKWARD) return; // 주행 중 전환 거부
    motor_.turn(direction);
    motorState_ = target;
}
```

---

## CleaningController.hpp

| 구분 | 항목 | 내용 |
|------|------|------|
| `[추가]` | `normalizePower()` | BOOST → NORMAL 강제 복귀 (비활성·비부스트 상태 시 무시) |
| `[추가]` | `bool active_` | 청소 활성화 여부 추적 멤버 변수 (초기값 false) |

```cpp
// [추가]
void normalizePower();  // BOOST → NORMAL (중복 무시, OFF 시 거부)

// [추가]
bool active_;
```

---

## CleaningController.cpp

| 구분 | 항목 | 내용 |
|------|------|------|
| `[변경]` | `startCleaning()` | 이미 활성 상태(`active_==true`)면 중복 호출 무시. `active_=true` 설정 추가 |
| `[변경]` | `stopCleaning()` | 비활성 상태(`active_==false`)면 중복 호출 무시. `active_=false` 설정 추가 |
| `[변경]` | `boostCleaning()` | 비활성 상태(`active_==false`)면 호출 거부 |
| `[추가]` | `normalizePower()` 구현 | 비활성 또는 비부스트 상태면 무시. 활성+부스트 중일 때 타이머 중단 후 NORMAL 복귀 |

```cpp
// [변경]
void CleaningController::startCleaning() {
    if (active_) return;   // [추가] 중복 guard
    active_ = true;        // [추가]
    timer_.stop();
    boosting_ = false;
    motor_.startCleaning(CleaningLevel::NORMAL);
}

void CleaningController::stopCleaning() {
    if (!active_) return;  // [추가] 비활성 guard
    active_ = false;       // [추가]
    timer_.stop();
    boosting_ = false;
    motor_.stopCleaning();
}

void CleaningController::boostCleaning() {
    if (!active_) return;  // [추가] 비활성 guard
    if (boosting_) { timer_.reset(); }
    else { boosting_ = true; motor_.startCleaning(CleaningLevel::HIGH); timer_.start(); }
}

// [추가]
void CleaningController::normalizePower() {
    if (!active_) return;
    if (!boosting_) return;
    timer_.stop();
    boosting_ = false;
    motor_.startCleaning(CleaningLevel::NORMAL);
}
```

---

## RVCController.hpp

| 구분 | 항목 | 내용 |
|------|------|------|
| `[삭제]` | ~~`#include <vector>`~~ | `sideStatus()` 시그니처 변경으로 불필요 |
| `[삭제]` | ~~`void allSidesBlocked()`~~ | 전방·좌·우 동시 막힘 판정을 `sideStatus()` 내부 상태머신으로 대체 |
| `[변경]` | `sideStatus(const std::vector<Direction>&)` → `sideStatus(Direction direction, bool clear)` | 방향별 상태를 순차 호출로 분리 |
| `[변경]` | `State` enum | ~~`IDLE`, `CLEANING`, `REVERSING` 3개~~ → 6개 상태로 확장 |
| `[추가]` | `State::STOPPED` | `frontObstacleDetected()` 수신 후 좌측 스캔 대기 상태 |
| `[추가]` | `State::STOPPED_CHECKING_RIGHT` | `sideStatus(LEFT)` 수신 후 `rotateRight()` 호출, 우측 스캔 대기 상태 |
| `[추가]` | `State::REVERSING_CHECKING_RIGHT` | 후진 중 `sideStatus(LEFT)` 막힘 확인 후 `rotateRight()`, 우측 스캔 대기 상태 |
| `[추가]` | `bool leftClear_` | `STOPPED` → `STOPPED_CHECKING_RIGHT` 전이 시 좌측 결과 임시 보관 |
| `[추가]` | `dustDetected(float dustLevel)` | 먼지 수준 float 파라미터 오버로드. 음수 입력 시 무시 |
| `[추가]` | `changeState(int newState)` | 외부에서 상태 직접 지정 (0=IDLE, 1=CLEANING). 범위 외 값 및 동일 상태 전이 무시 |

```cpp
// [변경] 이전: void allSidesBlocked();  ← [삭제]
// [변경] 이전: void sideStatus(const std::vector<Direction>& clearDirections);
void sideStatus(Direction direction, bool clear);  // [변경]
void dustDetected(float dustLevel);                // [추가]
void changeState(int newState);                    // [추가]

enum class State {
    IDLE,
    CLEANING,
    STOPPED,                   // [추가]
    STOPPED_CHECKING_RIGHT,    // [추가]
    REVERSING,
    REVERSING_CHECKING_RIGHT   // [추가]
};

bool leftClear_;  // [추가]
```

---

## RVCController.cpp

| 구분 | 항목 | 내용 |
|------|------|------|
| `[삭제]` | ~~`allSidesBlocked()` 구현~~ | 삭제. 좌·우 모두 막힘은 `STOPPED_CHECKING_RIGHT` 상태에서 내부 판정 |
| `[변경]` | `start()` 구현 | CLEANING·REVERSING·REVERSING_CHECKING_RIGHT 상태에서 중복 호출 무시 guard 추가 |
| `[변경]` | `frontObstacleDetected()` 구현 | REVERSING·REVERSING_CHECKING_RIGHT 상태(후진 중)에서 재감지 무시 guard 추가 |
| `[변경]` | `sideStatus()` 시그니처 | `const std::vector<Direction>&` → `Direction direction, bool clear` |
| `[변경]` | `sideStatus()` 내부 로직 | ~~방향 목록 기반 단순 분기~~ → 4개 상태에 걸친 순차 상태머신으로 전면 재작성 |
| `[추가]` | `STOPPED` 분기 | `leftClear_` 저장 후 `rotateRight()` 호출, `STOPPED_CHECKING_RIGHT` 전이 |
| `[추가]` | `STOPPED_CHECKING_RIGHT` 분기 | `rotateLeft()` 정면 복귀 후 LEFT 우선 방향 결정. 양쪽 막힘 시 `moveBackward()` → `REVERSING` |
| `[추가]` | `REVERSING` 분기 | 좌측 여유 시 즉시 `stop+turn(LEFT)+forward`. 막힘 시 `rotateRight()` → `REVERSING_CHECKING_RIGHT` |
| `[추가]` | `REVERSING_CHECKING_RIGHT` 분기 | `rotateLeft()` 복귀 후 우측 여유 시 `stop+turn(RIGHT)+forward`. 막힘 시 `REVERSING` 복귀(계속 후진) |
| `[추가]` | `dustDetected(float)` 구현 | 음수(`< 0.0f`) 입력 시 무시. 그 외 `cleaning_.boostCleaning()` 위임 |
| `[추가]` | `changeState(int)` 구현 | 유효 범위(0~1) 외 및 동일 상태 전이 무시. 0=IDLE, 1=CLEANING으로 직접 state_ 설정 |

**상태 전이 요약**

```
frontObstacleDetected()
  CLEANING → STOPPED

sideStatus(LEFT, clear)  [STOPPED]
  → rotateRight()
  → STOPPED_CHECKING_RIGHT  (leftClear_ 저장)

sideStatus(RIGHT, clear)  [STOPPED_CHECKING_RIGHT]
  → rotateLeft()
  → leftClear_==true  : turn(LEFT) → CLEANING
  → clear==true       : turn(RIGHT) → CLEANING
  → 둘 다 false       : moveBackward() → REVERSING   ← E1 진입

sideStatus(LEFT, clear)  [REVERSING]
  → clear==true  : stop → turn(LEFT) → CLEANING
  → clear==false : rotateRight() → REVERSING_CHECKING_RIGHT

sideStatus(RIGHT, clear)  [REVERSING_CHECKING_RIGHT]
  → rotateLeft()
  → clear==true  : stop → turn(RIGHT) → CLEANING
  → clear==false : REVERSING (계속 후진)
```

---

## 변경 없음

아래 파일은 v2.0에서 수정되지 않았다.

| 파일 | 역할 |
|------|------|
| `Direction.hpp` | LEFT / RIGHT 열거형 |
| `CleaningLevel.hpp` | NORMAL / HIGH 열거형 |
| `ICleaningMotor.hpp` | 청소 모터 인터페이스 |
| `ITimerExpiredCallback.hpp` | 타이머 만료 콜백 인터페이스 |
| `HighPowerTimer.hpp` | 고출력 타이머 헤더 |
| `HighPowerTimer.cpp` | 고출력 타이머 구현 |
