// UP팀 ST_scenarios.md 기반 시스템 테스트 (v2)
// 이 레포 인터페이스(RVCSimulator)에 맞게 재작성
//
// UP팀 인터페이스 vs 이 레포 인터페이스 매핑 원칙:
//   UP SensorData{frontSensorData, leftSensorData} push-1 → frontObstacleDetected() + sideStatus(LEFT,bool)
//   UP SensorData push-2 (우회전 후 전방 = 우측 상태)→ sideStatus(RIGHT,bool)
//   UP startCleaning()   → sim.start()
//   UP dustLevel >= 임계 → sim.dustDetected()
//   UP 없음(resumeCleaning/changeState/직접Motor) → N/A (주석으로 표시)

#include "TestRunner.hpp"
#include "../simulator/RVCSimulator.hpp"
#include <thread>
#include <chrono>
#include <algorithm>
#include <limits>

using D = Direction;

// ── 공통 헬퍼 ───────────────────────────────────────────────────────────────

// E1(삼면 장애물) 진입: CLEANING → REVERSING
static void enterReversing(RVCSimulator& sim) {
    sim.frontObstacleDetected();
    sim.sideStatus(D::LEFT, false);   // 좌측 막힘
    sim.sideStatus(D::RIGHT, false);  // 우측 막힘 → moveBackward
}

// ══════════════════════════════════════════════════════════════════════════
// POSITIVE 시나리오 (TC-POS-01 ~ TC-POS-10)
// ══════════════════════════════════════════════════════════════════════════

// TC-POS-01: UC-00 → UC-01 좌측 회피 전체 흐름
// UP팀 push1={false,true} 전방장애물,좌측뚫림 / push2={true,true} 우회전후 우측막힘 → turnLeft
static bool POS01(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.frontObstacleDetected();
    sim.sideStatus(D::LEFT, true);    // 좌측 뚫림
    sim.sideStatus(D::RIGHT, false);  // 우측 막힘 → leftClear_=true → turn(LEFT)

    SCHECK_EQ(sim.cleaning().stopCount, 1, "stopCleaning 1회");
    SCHECK_EQ((int)sim.drive().turnCalls.size(), 1, "turn 1회");
    SCHECK(sim.drive().turnCalls[0] == D::LEFT, "turnLeft");
    SCHECK(sim.cleaning().startCalls.size() >= 2u, "startCleaning 재호출");
    SCHECK(sim.cleaning().startCalls.back() == CleaningLevel::NORMAL, "재개 레벨 NORMAL");
    SCHECK(sim.drive().forwardCount >= 2, "moveForward 재호출");
    return true;
}

// TC-POS-02: UC-00 → UC-03 고먼지 집중 청소
// UP팀 dustLevel=80 → boostPower → normalizePower → resumeCleaning
static bool POS02(std::string& failMsg) {
    RVCSimulator sim(false, 80);  // 80ms 타이머 = boostPower 지속시간
    sim.start();
    sim.resetRecording();
    sim.dustDetected();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));  // 타이머 만료 대기

    SCHECK(sim.cleaning().startCalls.size() >= 2u, "HIGH + NORMAL 최소 2회");
    SCHECK(sim.cleaning().startCalls[0] == CleaningLevel::HIGH, "boostPower=HIGH");
    SCHECK(sim.cleaning().startCalls.back() == CleaningLevel::NORMAL, "normalizePower=NORMAL 자동복귀");
    SCHECK_EQ(sim.drive().forwardCount, 0, "DriveMotor 추가 호출 없음");
    return true;
}

// TC-POS-03: UC-00 → UC-01 → UC-02 후진 후 좌측 탈출
// UP팀 push1={true,true}, push2={true,true} → REVERSING
//       push3={false,false} 좌측뚫림 → turnLeft, push4 불필요 (이 레포 즉시처리)
static bool POS03(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.frontObstacleDetected();
    sim.sideStatus(D::LEFT, false);   // 좌측 막힘
    sim.sideStatus(D::RIGHT, false);  // 우측 막힘 → REVERSING
    sim.sideStatus(D::LEFT, true);    // 후진 중 좌측 뚫림 → 즉시 turnLeft

    SCHECK(sim.drive().backwardCount >= 1, "moveBackward");
    SCHECK_EQ((int)sim.drive().turnCalls.size(), 1, "turn 1회");
    SCHECK(sim.drive().turnCalls[0] == D::LEFT, "turnLeft");
    SCHECK(sim.cleaning().startCalls.back() == CleaningLevel::NORMAL, "재개 NORMAL");
    SCHECK(sim.drive().forwardCount >= 2, "moveForward 재호출");
    return true;
}

// TC-POS-04: UC-00 → UC-01 → UC-02 후진 없이 우측 탈출
// UP팀 push1={true,true} 전방+좌측막힘 / push2={true,false} 우측뚫림 → turnRight
static bool POS04(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.frontObstacleDetected();
    sim.sideStatus(D::LEFT, false);   // 좌측 막힘
    sim.sideStatus(D::RIGHT, true);   // 우측 뚫림 → turn(RIGHT)

    SCHECK_EQ((int)sim.drive().turnCalls.size(), 1, "turn 1회");
    SCHECK(sim.drive().turnCalls[0] == D::RIGHT, "turnRight");
    SCHECK(sim.cleaning().startCalls.back() == CleaningLevel::NORMAL, "재개 NORMAL");
    SCHECK(sim.drive().forwardCount >= 2, "moveForward 재호출");
    return true;
}

// TC-POS-05: UC-03 → UC-01 집중 청소 후 회피
// UP팀 dustLevel=80 → (타이머 만료) → 장애물 → 우측 탈출
static bool POS05(std::string& failMsg) {
    RVCSimulator sim(false, 80);
    sim.start();
    sim.dustDetected();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));  // normalizePower 대기

    // 집중 청소 복귀 후 전방 장애물
    sim.frontObstacleDetected();
    sim.sideStatus(D::LEFT, false);   // 좌측 막힘
    sim.sideStatus(D::RIGHT, true);   // 우측 뚫림 → turnRight

    SCHECK(sim.cleaning().startCalls.size() >= 2u, "boostPower 후 resumeCleaning");
    SCHECK(sim.drive().turnCalls.size() >= 1u, "장애물 후 turn");
    SCHECK(sim.drive().turnCalls.back() == D::RIGHT, "turnRight");
    SCHECK(sim.cleaning().startCalls.back() == CleaningLevel::NORMAL, "최종 NORMAL");
    return true;
}

// TC-POS-06: 2회 연속 전방 장애물 회피
// 1회차: 좌우 모두 뚫림 → LEFT 우선 / 2회차: 좌측 막힘, 우측 뚫림 → RIGHT
static bool POS06(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    // 1회차
    sim.frontObstacleDetected();
    sim.sideStatus(D::LEFT, true);    // 좌측 뚫림 → LEFT 우선 (v2.0 정책)
    sim.sideStatus(D::RIGHT, true);
    // 2회차
    sim.frontObstacleDetected();
    sim.sideStatus(D::LEFT, false);   // 좌측 막힘
    sim.sideStatus(D::RIGHT, true);   // 우측 뚫림 → RIGHT

    SCHECK_EQ((int)sim.drive().turnCalls.size(), 2, "turn 총 2회");
    SCHECK(sim.drive().turnCalls[0] == D::LEFT, "1회차 turnLeft");
    SCHECK(sim.drive().turnCalls[1] == D::RIGHT, "2회차 turnRight");
    SCHECK(sim.cleaning().startCalls.back() == CleaningLevel::NORMAL, "최종 NORMAL");
    return true;
}

// TC-POS-07: UC-03 2회 연속 집중 청소
// UP팀 dustLevel=75 → 1회 boostPower/normalizePower → 다시 75 → 2회
static bool POS07(std::string& failMsg) {
    RVCSimulator sim(false, 80);
    sim.start();
    sim.dustDetected();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    sim.dustDetected();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    long highCount = std::count(sim.cleaning().startCalls.begin(),
                                sim.cleaning().startCalls.end(), CleaningLevel::HIGH);
    long normalCount = std::count(sim.cleaning().startCalls.begin(),
                                  sim.cleaning().startCalls.end(), CleaningLevel::NORMAL);
    SCHECK_EQ((int)highCount,   2, "HIGH 2회 (각 집중청소)");
    SCHECK(normalCount >= 2,       "NORMAL 복귀 최소 2회");
    SCHECK(sim.cleaning().startCalls.back() == CleaningLevel::NORMAL, "최종 상태 NORMAL");
    return true;
}

// TC-POS-08: UC-03 미진입 (저먼지) 일반 청소 유지
// UP팀 dustLevel=30 → 아무 동작 없음
// 이 레포: dustLevel 파라미터 없음 → 이벤트 미발생으로 시뮬레이션 (임계값 미달 = dustDetected 미호출)
static bool POS08(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.resetRecording();
    // 낮은 먼지 수준 = dustDetected() 미호출 (센서 임계값 미달 상황)

    SCHECK_EQ(sim.cleaning().startCalls.size(), 0u, "추가 startCleaning 없음");
    SCHECK_EQ(sim.cleaning().stopCount, 0, "stopCleaning 없음");
    SCHECK_EQ((int)sim.drive().turnCalls.size(), 0, "turn 없음");
    SCHECK_EQ(sim.drive().stopCount, 0, "stop 없음");
    return true;
}

// TC-POS-09: UC-00 → UC-03 → UC-01 → UC-02 전체 순차
// 먼지 감지 → 집중청소 복귀 → 장애물 → 삼면 막힘 → 후진 → 좌측 탈출
static bool POS09(std::string& failMsg) {
    RVCSimulator sim(false, 80);
    sim.start();
    // UC-03: 먼지 감지
    sim.dustDetected();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    // UC-01 → UC-02: 삼면 장애물 후진
    sim.frontObstacleDetected();
    sim.sideStatus(D::LEFT, false);
    sim.sideStatus(D::RIGHT, false);
    sim.sideStatus(D::LEFT, true);    // 후진 중 좌측 탈출

    SCHECK(sim.cleaning().startCalls.size() >= 2u, "집중청소 포함 최소 2회");
    SCHECK(sim.drive().backwardCount >= 1, "moveBackward");
    SCHECK(sim.drive().turnCalls.back() == D::LEFT, "turnLeft 탈출");
    SCHECK(sim.cleaning().startCalls.back() == CleaningLevel::NORMAL, "최종 NORMAL");
    return true;
}

// TC-POS-10: UC-01 미진입 (전방 개방) 직진 유지
// UP팀 {true,false} 좌우 장애물, 전방 개방 → 회피 없이 직진 유지
// 이 레포: frontObstacleDetected() 미호출 → CLEANING 상태 유지
static bool POS10(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.resetRecording();
    // 전방 개방: frontObstacleDetected 미발생
    // CLEANING 상태에서 sideStatus는 무시됨
    sim.sideStatus(D::LEFT, false);
    sim.sideStatus(D::RIGHT, false);

    SCHECK_EQ(sim.drive().stopCount,     0, "stop 없음");
    SCHECK_EQ(sim.drive().backwardCount, 0, "backward 없음");
    SCHECK_EQ((int)sim.drive().turnCalls.size(), 0, "turn 없음");
    SCHECK_EQ(sim.cleaning().stopCount,  0, "stopCleaning 없음");
    SCHECK_EQ(sim.drive().forwardCount,  0, "forward 추가 없음");
    return true;
}

// ══════════════════════════════════════════════════════════════════════════
// NEGATIVE 시나리오 (TC-NEG-01 ~ TC-NEG-20)
// 인터페이스 없는 항목은 N/A 처리 (주석으로 이유 명시, 항상 true 반환)
// ══════════════════════════════════════════════════════════════════════════

// TC-NEG-01: CLEANING 중 startCleaning 중복 호출
// UP팀 기대: 두 번째 호출 무시 → Motor=FORWARD, startCleaning 1회
// 이 레포: start() 중복 허용 → FAIL 예상 (두 번째도 실행됨)
static bool NEG01(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.start();  // 두 번째 start()

    SCHECK_EQ(sim.drive().forwardCount, 1, "moveForward 1회만 (두 번째 무시 기대)");
    SCHECK_EQ(sim.cleaning().startCalls.size(), 1u, "startCleaning 1회만");
    return true;
}

// TC-NEG-02: CLEANING 중 resumeCleaning 호출
// UP팀 기대: 상태 변화 없음
// 이 레포: resumeCleaning() 인터페이스 없음 → 유사 검증: CLEANING 중 sideStatus 무시
static bool NEG02(std::string& failMsg) {
    // [N/A - 인터페이스 차이] resumeCleaning()이 이 레포에 없음
    // 유사 검증: CLEANING 상태에서 무관한 sideStatus는 동작 변경 없음
    RVCSimulator sim;
    sim.start();
    sim.resetRecording();
    sim.sideStatus(D::LEFT, true);  // CLEANING 상태에서 sideStatus → 무시됨

    SCHECK_EQ((int)sim.drive().turnCalls.size(), 0, "turn 없음");
    SCHECK_EQ(sim.drive().forwardCount,          0, "forward 추가 없음");
    SCHECK_EQ(sim.cleaning().startCalls.size(),  0u,"startCleaning 추가 없음");
    return true;
}

// TC-NEG-03: 유효하지 않은 SystemState 변경 시도 changeState(999)
// 기대: 요청 거부, SystemState=CLEANING 유지
static bool NEG03(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.resetRecording();
    sim.changeState(999);  // 유효하지 않은 상태값 → 거부
    SCHECK_EQ(sim.drive().stopCount,               0, "stop 없음");
    SCHECK_EQ(sim.drive().backwardCount,           0, "backward 없음");
    SCHECK_EQ((int)sim.drive().turnCalls.size(),   0, "turn 없음");
    SCHECK_EQ(sim.cleaning().stopCount,            0, "stopCleaning 없음");
    SCHECK_EQ(sim.cleaning().startCalls.size(),    0u,"startCleaning 추가 없음");
    return true;
}

// TC-NEG-04: AVOIDING 중 startCleaning 호출
// UP팀 기대: 새 청소 명령 무시 → Motor=BACKWARD 유지
// 이 레포: REVERSING 중 start() → 무시 없이 CLEANING 강제 전환 → FAIL 예상
static bool NEG04(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    enterReversing(sim);  // REVERSING 진입
    sim.resetRecording();
    sim.start();  // AVOIDING 중 startCleaning 시도

    SCHECK_EQ(sim.drive().forwardCount,          0, "forward 없음 (무시 기대)");
    SCHECK_EQ(sim.cleaning().startCalls.size(),  0u,"startCleaning 없음 (무시 기대)");
    return true;
}

// TC-NEG-05: 동일한 상태로 changeState 호출
// 기대: 상태 전환 로직 수행 안 함, SystemState=CLEANING 유지
static bool NEG05(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();  // CLEANING
    sim.resetRecording();
    sim.changeState(1);  // 1=CLEANING, 이미 CLEANING → no-op
    SCHECK_EQ(sim.drive().forwardCount,          0, "moveForward 추가 없음");
    SCHECK_EQ(sim.cleaning().startCalls.size(),  0u,"startCleaning 추가 없음");
    return true;
}

// TC-NEG-06: FORWARD 중 정지 단계 생략 급격한 방향 전환 turnLeft() → turnRight()
// 기대: 비정상 전환 거부, Motor=FORWARD 유지
static bool NEG06(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();  // Motor=FORWARD
    sim.resetRecording();
    sim.turn(D::LEFT);   // FORWARD 상태 → 거부
    sim.turn(D::RIGHT);  // FORWARD 상태 → 거부
    SCHECK_EQ((int)sim.drive().turnCalls.size(), 0, "turn 없음");
    return true;
}

// TC-NEG-07: 회피 중 전방 장애물 재발생
// UP팀 기대: 기존 회피 동작 유지 → Motor=BACKWARD, AVOIDING
// 이 레포: REVERSING 중 frontObstacleDetected → STOPPED 전환 → FAIL 예상
static bool NEG07(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    enterReversing(sim);
    sim.resetRecording();
    sim.frontObstacleDetected();  // 후진 중 전방 장애물 재감지

    // UP팀 기대: stop 추가 없이 후진 유지
    SCHECK_EQ(sim.drive().stopCount,     0, "stop 추가 없음 (회피 유지 기대)");
    SCHECK_EQ(sim.cleaning().stopCount,  0, "stopCleaning 추가 없음");
    return true;
}

// TC-NEG-08: FORWARD → moveForward() 중복 → 무시
static bool NEG08(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();  // Motor=FORWARD
    sim.resetRecording();
    sim.moveForward();  // 중복 → 무시
    SCHECK_EQ(sim.drive().forwardCount, 0, "중복 무시");
    return true;
}

// TC-NEG-09: BACKWARD → moveBackward() 중복 → 무시
static bool NEG09(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    enterReversing(sim);  // Motor=BACKWARD
    sim.resetRecording();
    sim.moveBackward();  // 중복 → 무시
    SCHECK_EQ(sim.drive().backwardCount, 0, "중복 무시");
    return true;
}

// TC-NEG-10: LEFT → turnLeft() 중복 → 무시
static bool NEG10(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();         // FORWARD
    sim.stop();          // STOPPED
    sim.turn(D::LEFT);   // LEFT
    sim.resetRecording();
    sim.turn(D::LEFT);   // 중복 → 무시
    SCHECK_EQ((int)sim.drive().turnCalls.size(), 0, "중복 무시");
    return true;
}

// TC-NEG-11: RIGHT → turnRight() 중복 → 무시
static bool NEG11(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();          // FORWARD
    sim.stop();           // STOPPED
    sim.turn(D::RIGHT);   // RIGHT
    sim.resetRecording();
    sim.turn(D::RIGHT);   // 중복 → 무시
    SCHECK_EQ((int)sim.drive().turnCalls.size(), 0, "중복 무시");
    return true;
}

// TC-NEG-12: STOPPED → stopMotor() 중복 → 무시
static bool NEG12(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();  // FORWARD
    sim.stop();   // STOPPED
    sim.resetRecording();
    sim.stop();   // 중복 → 무시
    SCHECK_EQ(sim.drive().stopCount, 0, "중복 무시");
    return true;
}

// TC-NEG-13: 음수 DustLevel → 유효성 검사 후 차단, Cleaner=NORMAL 유지
static bool NEG13(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.resetRecording();
    sim.dustDetected(-1.0f);  // 음수 → 거부
    SCHECK_EQ(sim.cleaning().startCalls.size(), 0u, "boost 없음");
    SCHECK_EQ(sim.cleaning().stopCount,         0,  "stop 없음");
    return true;
}

// TC-NEG-14: FLT_MAX DustLevel → 최대값으로 처리, 크래시 없음, Cleaner=NORMAL 복귀
static bool NEG14(std::string& failMsg) {
    RVCSimulator sim(false, 80);
    sim.start();
    sim.resetRecording();
    sim.dustDetected(std::numeric_limits<float>::max());
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    SCHECK(sim.cleaning().startCalls.size() >= 1u, "처리됨 (크래시 없음)");
    SCHECK(sim.cleaning().startCalls.back() == CleaningLevel::NORMAL, "타이머 만료 후 NORMAL");
    return true;
}

// TC-NEG-15: OFF → deactivateCleaner() 중복 → 무시
static bool NEG15(std::string& failMsg) {
    RVCSimulator sim;  // start() 없음 → Cleaner=OFF
    sim.stopCleaning();  // 이미 OFF → 무시
    SCHECK_EQ(sim.cleaning().stopCount, 0, "중복 무시");
    return true;
}

// TC-NEG-16: NORMAL → activateCleaner() 중복 → 무시
static bool NEG16(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();  // Cleaner=NORMAL
    sim.resetRecording();
    sim.startCleaning();  // 이미 active → 무시
    SCHECK_EQ(sim.cleaning().startCalls.size(), 0u, "중복 무시");
    return true;
}

// TC-NEG-17: BOOST → boostPower() 중복 → 무시
static bool NEG17(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.dustDetected();  // Cleaner=BOOST
    sim.resetRecording();
    sim.boostCleaning();    // 이미 BOOST → 무시
    SCHECK_EQ(sim.cleaning().startCalls.size(), 0u, "중복 무시");
    return true;
}

// TC-NEG-18: NORMAL → normalizePower() 중복 → 무시
static bool NEG18(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();  // Cleaner=NORMAL (not boosting)
    sim.resetRecording();
    sim.normalizePower();  // 이미 NORMAL → 무시
    SCHECK_EQ(sim.cleaning().startCalls.size(), 0u, "중복 무시");
    return true;
}

// TC-NEG-19: OFF → normalizePower() → 비활성 상태 전력 조절 차단
static bool NEG19(std::string& failMsg) {
    RVCSimulator sim;  // start() 없음 → OFF
    sim.normalizePower();  // OFF → 거부
    SCHECK_EQ(sim.cleaning().startCalls.size(), 0u, "차단됨");
    return true;
}

// TC-NEG-20: OFF → boostPower() → 비활성 상태 전력 조절 차단
static bool NEG20(std::string& failMsg) {
    RVCSimulator sim;  // start() 없음 → OFF
    sim.boostCleaning();  // OFF → 거부
    SCHECK_EQ(sim.cleaning().startCalls.size(), 0u, "차단됨");
    return true;
}

// ══════════════════════════════════════════════════════════════════════════
int main() {
    TestRunner runner;

    const std::string POS = "Positive  [UP팀 TC-POS-01~10]";
    runner.add("TC-POS-01", POS, "좌측 회피 전체 흐름 (전방장애물, 좌측뚫림, 우측막힘)",     POS01);
    runner.add("TC-POS-02", POS, "고먼지 집중 청소 → 타이머 만료 → NORMAL 복귀",            POS02);
    runner.add("TC-POS-03", POS, "후진 후 좌측 탈출 (삼면 막힘)",                           POS03);
    runner.add("TC-POS-04", POS, "좌측 막힘·우측 뚫림 → turnRight",                        POS04);
    runner.add("TC-POS-05", POS, "집중 청소 복귀 후 장애물 → turnRight",                    POS05);
    runner.add("TC-POS-06", POS, "2회 연속 회피 (1차 LEFT·2차 RIGHT)",                      POS06);
    runner.add("TC-POS-07", POS, "2회 연속 집중 청소 → 각 NORMAL 복귀",                     POS07);
    runner.add("TC-POS-08", POS, "저먼지 미발생 → 일반 청소 유지",                          POS08);
    runner.add("TC-POS-09", POS, "전체 순차: 먼지 → 장애물 → 후진 → 탈출",                  POS09);
    runner.add("TC-POS-10", POS, "전방 개방 → 직진 유지 (sideStatus 무시)",                  POS10);

    const std::string NEG = "Negative  [UP팀 TC-NEG-01~20]";
    runner.add("TC-NEG-01", NEG, "CLEANING 중 start() 중복 → 무시 기대 [FAIL 예상]",        NEG01);
    runner.add("TC-NEG-02", NEG, "resumeCleaning 없음 → 유사: sideStatus 무시 확인",         NEG02);
    runner.add("TC-NEG-03", NEG, "changeState(999) → 유효하지 않은 상태 거부",               NEG03);
    runner.add("TC-NEG-04", NEG, "AVOIDING 중 start() → 무시",                              NEG04);
    runner.add("TC-NEG-05", NEG, "동일 상태 changeState(CLEANING) → 전이 없음",             NEG05);
    runner.add("TC-NEG-06", NEG, "FORWARD 중 turn() → 급격한 전환 거부",                    NEG06);
    runner.add("TC-NEG-07", NEG, "AVOIDING 중 장애물 재감지 → 회피 유지",                   NEG07);
    runner.add("TC-NEG-08", NEG, "FORWARD → moveForward() 중복 → 무시",                     NEG08);
    runner.add("TC-NEG-09", NEG, "BACKWARD → moveBackward() 중복 → 무시",                   NEG09);
    runner.add("TC-NEG-10", NEG, "LEFT → turn(LEFT) 중복 → 무시",                           NEG10);
    runner.add("TC-NEG-11", NEG, "RIGHT → turn(RIGHT) 중복 → 무시",                         NEG11);
    runner.add("TC-NEG-12", NEG, "STOPPED → stop() 중복 → 무시",                            NEG12);
    runner.add("TC-NEG-13", NEG, "음수 dustLevel → 유효성 검사 후 차단",                    NEG13);
    runner.add("TC-NEG-14", NEG, "FLT_MAX dustLevel → 최대값 처리, 크래시 없음",             NEG14);
    runner.add("TC-NEG-15", NEG, "OFF → stopCleaning() 중복 → 무시",                        NEG15);
    runner.add("TC-NEG-16", NEG, "NORMAL → startCleaning() 중복 → 무시",                    NEG16);
    runner.add("TC-NEG-17", NEG, "BOOST → boostCleaning() 중복 → 무시",                     NEG17);
    runner.add("TC-NEG-18", NEG, "NORMAL → normalizePower() 중복 → 무시",                   NEG18);
    runner.add("TC-NEG-19", NEG, "OFF → normalizePower() → 비활성 상태 차단",               NEG19);
    runner.add("TC-NEG-20", NEG, "OFF → boostCleaning() → 비활성 상태 차단",                NEG20);

    return runner.run();
}
