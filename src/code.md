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

```cpp
// [변경] 이전: void turn(const std::vector<Direction>& directions);
void turn(Direction direction);   // [변경]
void rotateRight();               // [추가]
void rotateLeft();                // [추가]
```

---

## NavigationController.cpp

| 구분 | 항목 | 내용 |
|------|------|------|
| `[삭제]` | ~~`#include <vector>`, `#include <random>`~~ | 랜덤 선택 로직 제거로 헤더 불필요 |
| `[변경]` | `turn()` 구현 | ~~벡터를 받아 랜덤으로 방향 선택~~ → 단일 방향을 `motor_.turn(direction)`에 그대로 위임 |
| `[추가]` | `rotateRight()` 구현 | `motor_.rotateRight()` 위임 |
| `[추가]` | `rotateLeft()` 구현 | `motor_.rotateLeft()` 위임 |

```cpp
// [변경] 이전: 벡터에서 랜덤 선택하는 로직
void NavigationController::turn(Direction direction) {
    motor_.turn(direction);
}

// [추가]
void NavigationController::rotateRight() { motor_.rotateRight(); }
void NavigationController::rotateLeft()  { motor_.rotateLeft();  }
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

```cpp
// [변경] 이전: void allSidesBlocked();  ← [삭제]
// [변경] 이전: void sideStatus(const std::vector<Direction>& clearDirections);
void sideStatus(Direction direction, bool clear);  // [변경]

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
| `[변경]` | `sideStatus()` 시그니처 | `const std::vector<Direction>&` → `Direction direction, bool clear` |
| `[변경]` | `sideStatus()` 내부 로직 | ~~방향 목록 기반 단순 분기~~ → 4개 상태에 걸친 순차 상태머신으로 전면 재작성 |
| `[추가]` | `STOPPED` 분기 | `leftClear_` 저장 후 `rotateRight()` 호출, `STOPPED_CHECKING_RIGHT` 전이 |
| `[추가]` | `STOPPED_CHECKING_RIGHT` 분기 | `rotateLeft()` 정면 복귀 후 LEFT 우선 방향 결정. 양쪽 막힘 시 `moveBackward()` → `REVERSING` |
| `[추가]` | `REVERSING` 분기 | 좌측 여유 시 즉시 `stop+turn(LEFT)+forward`. 막힘 시 `rotateRight()` → `REVERSING_CHECKING_RIGHT` |
| `[추가]` | `REVERSING_CHECKING_RIGHT` 분기 | `rotateLeft()` 복귀 후 우측 여유 시 `stop+turn(RIGHT)+forward`. 막힘 시 `REVERSING` 복귀(계속 후진) |

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
| `CleaningController.hpp` | 청소 컨트롤러 헤더 |
| `CleaningController.cpp` | 청소 컨트롤러 구현 |
| `HighPowerTimer.hpp` | 고출력 타이머 헤더 |
| `HighPowerTimer.cpp` | 고출력 타이머 구현 |
