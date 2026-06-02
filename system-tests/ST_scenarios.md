# UP팀 시스템 테스트 시나리오 명세

## 개요

- 오른쪽 센서 삭제 후 변경된 요구사항 반영
- 센서 입력은 `SensorData {frontSensorData, leftSensorData}` 2개 필드
- 오른쪽 장애물 확인은 **우회전 후 전방 센서값**으로 대체
- 따라서 장애물 감지 시 **2번의 push**로 판단

---

## Positive 시나리오

### TC-POS-01: UC-00 → UC-01 좌측 회피 전체 흐름

| 항목 | 내용 |
|------|------|
| 사전 조건 | 청소 중 (CLEANING, FORWARD) |
| 입력 1 | `{frontSensorData=false, leftSensorData=true}` (전방 장애물, 좌측 뚫림) |
| 입력 2 | `{frontSensorData=true, leftSensorData=true}` (우회전 후 전방 막힘 → 우측 막힘) |
| 기대 동작 | turnLeft → resumeCleaning |
| 기대 상태 | SystemState=CLEANING, Motor=FORWARD |

---

### TC-POS-02: UC-00 → UC-03 고먼지 집중 청소

| 항목 | 내용 |
|------|------|
| 사전 조건 | 청소 중 (CLEANING) |
| 입력 | dustLevel=80.0f |
| 기대 동작 | boostPower → normalizePower → resumeCleaning |
| 기대 상태 | SystemState=CLEANING, Cleaner=NORMAL |

---

### TC-POS-03: UC-00 → UC-01 → UC-02 후진 후 좌측 탈출

| 항목 | 내용 |
|------|------|
| 사전 조건 | 청소 중 (CLEANING) |
| 입력 1 | `{true, true}` (전방+좌측 막힘) |
| 입력 2 | `{true, true}` (우회전 후 전방도 막힘 → 양쪽 막힘 → 후진) |
| 입력 3 | `{false, false}` (후진 중 좌측 뚫림) |
| 입력 4 | `{false, false}` (우회전 후 전방도 뚫림 → 좌측 탈출) |
| 기대 동작 | moveBackward → turnLeft → resumeCleaning |
| 기대 상태 | SystemState=CLEANING, Motor=FORWARD |

---

### TC-POS-04: UC-00 → UC-01 → UC-02 후진 후 우측 탈출

| 항목 | 내용 |
|------|------|
| 사전 조건 | 청소 중 (CLEANING) |
| 입력 1 | `{true, true}` (전방+좌측 막힘) |
| 입력 2 | `{true, false}` (우회전 후 전방 뚫림 → 우측 뚫림 → 바로 우측 탈출) |
| 기대 동작 | turnRight → resumeCleaning |
| 기대 상태 | SystemState=CLEANING, Motor=FORWARD |

---

### TC-POS-05: UC-03 → UC-01 집중 청소 후 회피

| 항목 | 내용 |
|------|------|
| 사전 조건 | 청소 중 (CLEANING) |
| 입력 1 | dustLevel=80.0f |
| 입력 2 | `{true, true}` (집중 청소 복귀 후 전방 장애물) |
| 입력 3 | `{true, false}` (우회전 후 우측 뚫림 → 우측 탈출) |
| 기대 동작 | boostPower → resumeCleaning → turnRight → resumeCleaning |
| 기대 상태 | SystemState=CLEANING, Motor=FORWARD |

---

### TC-POS-06: 2회 연속 전방 장애물 회피

| 항목 | 내용 |
|------|------|
| 사전 조건 | 청소 중 (CLEANING) |
| 1회차 입력 1 | `{false, true}` (전방 장애물, 좌측 뚫림) |
| 1회차 입력 2 | `{true, true}` (우측 막힘 → 양쪽 막힘 아님, 좌측으로 탈출) |
| 2회차 입력 1 | `{true, true}` (전방+좌측 막힘) |
| 2회차 입력 2 | `{true, false}` (우측 뚫림 → 우측 탈출) |
| 기대 상태 | SystemState=CLEANING, Motor=FORWARD |

---

### TC-POS-07: UC-03 2회 연속 집중 청소

| 항목 | 내용 |
|------|------|
| 사전 조건 | 청소 중 (CLEANING) |
| 입력 1 | dustLevel=75.0f |
| 입력 2 | dustLevel=75.0f |
| 기대 상태 (1회) | SystemState=CLEANING, Cleaner=NORMAL |
| 기대 상태 (2회) | SystemState=CLEANING, Cleaner=NORMAL |

---

### TC-POS-08: UC-03 미진입 (저먼지) 일반 청소 유지

| 항목 | 내용 |
|------|------|
| 사전 조건 | 청소 중 (CLEANING) |
| 입력 | dustLevel=30.0f |
| 기대 동작 | 아무 동작 없음 |
| 기대 상태 | SystemState=CLEANING, Motor=FORWARD, Cleaner=NORMAL |

---

### TC-POS-09: UC-00 → UC-03 → UC-01 → UC-02 전체 순차

| 항목 | 내용 |
|------|------|
| 사전 조건 | 청소 중 (CLEANING) |
| 입력 1 | dustLevel=75.0f |
| 입력 2 | `{true, true}` (전방+좌측 막힘) |
| 입력 3 | `{true, true}` (우측도 막힘 → 후진) |
| 입력 4 | `{false, false}` (후진 중 좌측 뚫림) |
| 입력 5 | `{false, false}` (우회전 후 뚫림 → 좌측 탈출) |
| 기대 상태 | SystemState=CLEANING, Motor=FORWARD |

---

### TC-POS-10: UC-01 미진입 (전방 개방) 직진 유지

| 항목 | 내용 |
|------|------|
| 사전 조건 | 청소 중 (CLEANING) |
| 입력 | `{true, false}` (좌우 장애물, 전방 개방) |
| 기대 동작 | 회피 없이 직진 유지 |
| 기대 상태 | SystemState=CLEANING, Motor=FORWARD, Cleaner=NORMAL |

---

## Negative 시나리오

### TC-NEG-01: CLEANING 중 startCleaning 중복 호출

| 항목 | 내용 |
|------|------|
| 입력 | startCleaning() × 2 |
| 기대 동작 | 두 번째 호출 무시 |
| 기대 상태 | SystemState=CLEANING, Motor=FORWARD |

---

### TC-NEG-02: CLEANING 중 resumeCleaning 호출

| 항목 | 내용 |
|------|------|
| 입력 | startCleaning() → resumeCleaning() |
| 기대 동작 | 상태 변화 없음 |
| 기대 상태 | SystemState=CLEANING, Motor=FORWARD |

---

### TC-NEG-03: 유효하지 않은 SystemState 변경 시도

| 항목 | 내용 |
|------|------|
| 입력 | changeState(999) |
| 기대 동작 | 요청 거부 |
| 기대 상태 | SystemState=CLEANING |

---

### TC-NEG-04: AVOIDING 중 startCleaning 호출

| 항목 | 내용 |
|------|------|
| 입력 1 | `{true, true}` → `{true, true}` (후진 상태 진입) |
| 입력 2 | startCleaning() |
| 기대 동작 | 새 청소 명령 무시 |
| 기대 상태 | SystemState=AVOIDING, Motor=BACKWARD |

---

### TC-NEG-05: 동일한 상태로 changeState 호출

| 항목 | 내용 |
|------|------|
| 입력 | changeState(CLEANING) (이미 CLEANING 상태) |
| 기대 동작 | 상태 전환 로직 수행 안 함 |
| 기대 상태 | SystemState=CLEANING |

---

### TC-NEG-06: FORWARD 중 정지 단계 생략 급격한 방향 전환

| 항목 | 내용 |
|------|------|
| 입력 | turnLeft() → turnRight() (FORWARD 상태에서) |
| 기대 동작 | 비정상 전환 거부 |
| 기대 상태 | Motor=FORWARD |

---

### TC-NEG-07: 회피 중 전방 장애물 재발생

| 항목 | 내용 |
|------|------|
| 입력 1 | `{true, true}` → `{true, true}` (후진 진입) |
| 입력 2 | `{true, true}` → `{true, true}` (후진 중 재감지) |
| 기대 동작 | 기존 회피 동작 유지 |
| 기대 상태 | SystemState=AVOIDING, Motor=BACKWARD, Cleaner=OFF |

---

### TC-NEG-08 ~ TC-NEG-12: Motor 중복 명령

| TC | 상황 | 기대 동작 |
|----|------|-----------|
| NEG-08 | FORWARD → moveForward() | 중복 무시, FORWARD 유지 |
| NEG-09 | BACKWARD → moveBackward() | 중복 무시, BACKWARD 유지 |
| NEG-10 | LEFT → turnLeft() | 중복 무시, LEFT 유지 |
| NEG-11 | RIGHT → turnRight() | 중복 무시, RIGHT 유지 |
| NEG-12 | STOPPED → stopMotor() | 중복 무시, STOPPED 유지 |

---

### TC-NEG-13: 음수 DustLevel 입력

| 항목 | 내용 |
|------|------|
| 입력 | dustLevel=-1.0f |
| 기대 동작 | 유효성 검사 후 명령 차단 |
| 기대 상태 | SystemState=CLEANING, Cleaner=NORMAL |

---

### TC-NEG-14: 매우 큰 DustLevel 입력

| 항목 | 내용 |
|------|------|
| 입력 | dustLevel=FLT_MAX |
| 기대 동작 | 최대값으로 처리, 크래시 없음 |
| 기대 상태 | SystemState=CLEANING, Cleaner=NORMAL |

---

### TC-NEG-15 ~ TC-NEG-20: Cleaner 중복/비정상 명령

| TC | 상황 | 기대 동작 |
|----|------|-----------|
| NEG-15 | OFF → deactivateCleaner() | 중복 무시, OFF 유지 |
| NEG-16 | NORMAL → activateCleaner() | 중복 무시, NORMAL 유지 |
| NEG-17 | BOOST → boostPower() | 중복 무시, BOOST 유지 |
| NEG-18 | NORMAL → normalizePower() | 중복 무시, NORMAL 유지 |
| NEG-19 | OFF → normalizePower() | 비활성 상태 전력 조절 차단, OFF 유지 |
| NEG-20 | OFF → boostPower() | 비활성 상태 전력 조절 차단, OFF 유지 |
