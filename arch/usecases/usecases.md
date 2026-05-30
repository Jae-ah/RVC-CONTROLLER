# Use Case 목록 (Use Cases)

## 1. 액터 (Actors)

| 액터 | 종류 | 설명 |
|------|------|------|
| User | Primary Actor | 시스템을 시작시키는 주체 |
| ObstacleSensor | Primary Actor | [변경] ~~전방/좌/우 장애물 감지 신호를 시스템에 전달한다~~ 전방/좌 장애물 감지 신호를 시스템에 전달한다. 우측 장애물 감지는 전방 센서 회전 스캔으로 대체한다 (우측 전용 센서 없음). |
| DustSensor | Primary Actor | 먼지 감지 신호를 시스템에 전달한다 |
| DriveMotor | Secondary Actor | 시스템으로부터 주행 명령(전진/후진/회전/정지)을 수신한다 |
| CleaningMotor | Secondary Actor | 시스템으로부터 청소 출력 명령(Normal/High/정지)을 수신한다 |

## 2. Use Case 다이어그램

```mermaid
graph LR
    User(["User«actor»"])
    ObsSensor(["ObstacleSensor«actor»"])
    DustSensor(["DustSensor«actor»"])
    DriveMtr(["DriveMotor«actor»"])
    CleanMtr(["CleaningMotor«actor»"])

    subgraph SYS["RVC SW Controller"]
        UC001("UC-001<br/>자동 청소 수행")
        UC002("UC-002<br/>측면 장애물 회피")
        UC003("UC-003<br/>삼면 장애물 후진 회피")
        UC004("UC-004<br/>고출력 청소")
    end

    User -->|start| UC001
    ObsSensor -->|frontObstacleDetected| UC002
    DustSensor -->|dustDetected| UC004

    UC001 --> DriveMtr
    UC001 --> CleanMtr
    UC002 --> DriveMtr
    UC002 --> CleanMtr
    UC003 --> DriveMtr
    UC003 --> CleanMtr
    UC004 --> CleanMtr

    UC002 -.->|«extend»| UC001
    UC003 -.->|"[변경] «extend» E1"| UC002
    UC004 -.->|«extend»| UC001

    %% [삭제] ObsSensor -->|...| UC003  (UC-002 E1 내부 판정으로 대체)
    %% [삭제] UC003 -.->|«extend»| UC001  (UC-002 E1으로 이동)
```

## 3. Use Case 목록

| Use Case ID | 이름 | 관련 액터 | 관련 요구사항 | 상세 문서 |
|-------------|------|-----------|--------------|-----------|
| UC-001 | 자동 청소 수행 | User, DriveMotor, CleaningMotor | FR-01 | [UC-001.md](UC-001.md) |
| UC-002 | 측면 장애물 회피 | ObstacleSensor, DriveMotor, CleaningMotor | FR-02 | [UC-002.md](UC-002.md) |
| UC-003 | 삼면 장애물 후진 회피 | [변경] ~~ObstacleSensor, DriveMotor~~ ObstacleSensor, DriveMotor, CleaningMotor | FR-03 | [UC-003.md](UC-003.md) |
| UC-004 | 고출력 청소 | DustSensor, CleaningMotor | FR-04 | [UC-004.md](UC-004.md) |

## 4. Use Case 요약

### UC-001: 자동 청소 수행
User가 시스템을 시작하면 CleaningMotor를 Normal 출력으로 가동하고 DriveMotor로 직진하며 바닥을 청소하고 닦는다. 이 동작은 시스템의 기본(Base) 흐름이며, 나머지 UC는 이를 확장(extend)한다.

### UC-002: 측면 장애물 회피
전방에 장애물이 감지되면 청소를 일시 정지하고, [변경] ~~좌 또는 우 중 여유 공간이 있을 때~~ sideStatus(LEFT) 및 rotateRight() + sideStatus(RIGHT) 로 양측을 순차 확인한다. [변경] ~~좌·우 모두 여유 있으면 랜덤으로 방향을 선택한다.~~ 좌측을 우선으로 선택한다. 좌측이 막힌 경우에만 우측으로 선택한다. 좌·우 모두 막힌 경우 E1으로 UC-003에 진입한다.

### UC-003: 삼면 장애물 후진 회피
[변경] ~~전방·좌·우 모두 장애물이 감지될 때 ObstacleSensor가 직접 트리거한다.~~ UC-002 E1에서 진입한다 (sideStatus(LEFT, blocked) + sideStatus(RIGHT, blocked) 내부 판정). 후진하며 sideStatus(LEFT) 또는 rotateRight() + sideStatus(RIGHT)로 여유 공간을 확인하고, 확보되면 [변경] ~~랜덤으로~~ 좌측을 우선으로 선택하여 회전하고 전진한다.

### UC-004: 고출력 청소
먼지가 감지되면 CleaningMotor 출력을 High로 높이고, 일정 시간이 경과하면 자동으로 Normal 출력으로 복귀한다. 주행은 중단 없이 계속한다.

## 5. Use Case 간 관계

```mermaid
graph TD
    UC001["UC-001<br/>자동 청소 수행<br/>(Base)"]
    UC002["UC-002<br/>측면 장애물 회피"]
    UC003["UC-003<br/>삼면 장애물 후진 회피"]
    UC004["UC-004<br/>고출력 청소"]

    UC002 -- "«extend»<br/>[변경] frontObstacleDetected" --> UC001
    UC003 -- "«extend»<br/>[변경] E1: 좌·우 모두 막힘" --> UC002
    UC004 -- "«extend»<br/>[dustDetected]" --> UC001
    %% [삭제] UC003 -- «extend» [...] --> UC001
```
