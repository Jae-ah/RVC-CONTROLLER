// RVC 시스템 테스트 — Google Test 없이 TestRunner + RVCSimulator 기반
// Positive 10개 (ST-P01~P10) / Negative 20개 (ST-N01~N20)
#include "TestRunner.hpp"
#include "../simulator/RVCSimulator.hpp"
#include <thread>
#include <chrono>
#include <algorithm>

using D = Direction;

// 순서 헬퍼: eventLog에서 a가 b보다 먼저 등장하는지 확인
static bool before(const std::vector<std::string>& log,
                   const std::string& a, const std::string& b) {
    auto ia = std::find(log.begin(), log.end(), a);
    auto ib = std::find(log.begin(), log.end(), b);
    return (ia != log.end()) && (ib != log.end()) && (ia < ib);
}

// [v2.0] UC-002 E1 진입 헬퍼: frontObstacleDetected + 좌우 모두 막힘 → REVERSING
static void enterReversing(RVCSimulator& sim) {
    sim.frontObstacleDetected();
    sim.sideStatus(D::LEFT, false);
    sim.sideStatus(D::RIGHT, false);
}

// ══════════════════════════════════════════════════════════════════════════
// POSITIVE 테스트 (ST-P01 ~ ST-P10)
// ══════════════════════════════════════════════════════════════════════════

// ST-P01: start() → CleaningMotor(NORMAL) + DriveMotor forward
static bool P01(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    SCHECK_EQ(sim.cleaning().startCalls.size(), 1u, "startCleaning 호출 수");
    SCHECK(sim.cleaning().startCalls[0] == CleaningLevel::NORMAL, "레벨이 NORMAL이어야 함");
    SCHECK_EQ(sim.drive().forwardCount, 1, "moveForward 호출 수");
    SCHECK_EQ(sim.drive().stopCount,    0, "stop 미호출");
    return true;
}

// ST-P02: frontObstacleDetected() → stopCleaning + DriveMotor stop
static bool P02(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.frontObstacleDetected();
    SCHECK_EQ(sim.cleaning().stopCount, 1, "stopCleaning 호출 수");
    SCHECK_EQ(sim.drive().stopCount,    1, "DriveMotor stop 호출 수");
    return true;
}

// ST-P03: 장애물 후 우측 회피 — 완전 시퀀스
// [v2.0] sideStatus(LEFT,false) → rotateRight → sideStatus(RIGHT,true) → rotateLeft → turn(RIGHT)
static bool P03(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.frontObstacleDetected();
    sim.sideStatus(D::LEFT, false);
    sim.sideStatus(D::RIGHT, true);

    SCHECK_EQ(sim.drive().turnCalls.size(), 1u, "turn 호출 수");
    SCHECK(sim.drive().turnCalls[0] == D::RIGHT, "방향이 RIGHT이어야 함");
    SCHECK(sim.cleaning().startCalls.size() >= 2u, "startCleaning 재호출 필요");
    SCHECK(sim.cleaning().startCalls.back() == CleaningLevel::NORMAL, "재개 레벨이 NORMAL");
    SCHECK(sim.drive().forwardCount >= 2, "회피 후 moveForward 재호출");
    return true;
}

// ST-P04: 장애물 후 좌측 회피 — 완전 시퀀스
// [v2.0] sideStatus(LEFT,true) → rotateRight → sideStatus(RIGHT,X) → rotateLeft → turn(LEFT) 우선
static bool P04(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.frontObstacleDetected();
    sim.sideStatus(D::LEFT, true);
    sim.sideStatus(D::RIGHT, false);

    SCHECK_EQ(sim.drive().turnCalls.size(), 1u, "turn 호출 수");
    SCHECK(sim.drive().turnCalls[0] == D::LEFT, "방향이 LEFT이어야 함");
    SCHECK(sim.cleaning().startCalls.back() == CleaningLevel::NORMAL, "재개 레벨이 NORMAL");
    return true;
}

// ST-P05: 장애물 후 좌·우 모두 여유 — LEFT 우선 선택 (v2.0 정책)
static bool P05(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.frontObstacleDetected();
    sim.sideStatus(D::LEFT, true);
    sim.sideStatus(D::RIGHT, true);

    SCHECK_EQ(sim.drive().turnCalls.size(), 1u, "turn 정확히 1회");
    SCHECK(sim.drive().turnCalls[0] == D::LEFT, "[v2.0] 좌·우 모두 여유 시 LEFT 우선");
    return true;
}

// ST-P06: 삼면 장애물 후 우측 전진 — 완전 시퀀스 및 순서
// [v2.0] E1: frontObstacleDetected + sideStatus(L,false) + sideStatus(R,false) → REVERSING
//        후진 중: sideStatus(L,false) + sideStatus(R,true) → stop + turn(RIGHT) + forward
static bool P06(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.resetRecording();
    enterReversing(sim);
    sim.sideStatus(D::LEFT, false);
    sim.sideStatus(D::RIGHT, true);

    const auto& log = sim.eventLog();
    SCHECK_EQ(sim.cleaning().stopCount,     1, "stopCleaning 1회");
    SCHECK_EQ(sim.drive().backwardCount,    1, "moveBackward 1회");
    SCHECK_EQ(sim.drive().stopCount,        2, "DriveMotor stop 2회 (장애물+후진종료)");
    SCHECK_EQ(sim.drive().turnCalls.size(), 1u, "turn 1회");
    SCHECK(sim.drive().turnCalls[0] == D::RIGHT, "방향이 RIGHT");
    SCHECK_EQ(sim.drive().forwardCount,     1, "회피 후 moveForward 1회");
    SCHECK(before(log, "drive:forward", "clean:start(NORMAL)"), "SD-003: forward → startCleaning 순서");
    return true;
}

// ST-P07: 삼면 장애물 후 좌측 전진
// [v2.0] REVERSING 중 sideStatus(LEFT,true) → 즉시 stop+turn(LEFT)+forward (rotateRight 생략)
static bool P07(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.resetRecording();
    enterReversing(sim);
    sim.sideStatus(D::LEFT, true);

    SCHECK(sim.drive().turnCalls[0] == D::LEFT, "방향이 LEFT");
    SCHECK_EQ(sim.drive().stopCount, 2, "stop 2회 (장애물 감지 + 후진 종료)");
    SCHECK(before(sim.eventLog(), "drive:forward", "clean:start(NORMAL)"),
           "SD-003: forward → startCleaning 순서");
    return true;
}

// ST-P08: 삼면 장애물 — 좌우 막힘 반복 후 방향 확보
static bool P08(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    enterReversing(sim);
    // 2회 반복 막힘
    sim.sideStatus(D::LEFT, false); sim.sideStatus(D::RIGHT, false);
    sim.sideStatus(D::LEFT, false); sim.sideStatus(D::RIGHT, false);
    // 우측 확보
    sim.sideStatus(D::LEFT, false);
    sim.sideStatus(D::RIGHT, true);

    SCHECK_EQ(sim.drive().backwardCount,    1, "moveBackward 1회");
    SCHECK_EQ(sim.drive().turnCalls.size(), 1u, "turn 1회");
    SCHECK_EQ(sim.drive().stopCount,        2, "stop 2회 (장애물+후진종료)");
    return true;
}

// ST-P09: 먼지 감지 → HIGH 전환
static bool P09(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.resetRecording();
    sim.dustDetected();

    SCHECK_EQ(sim.cleaning().startCalls.size(), 1u, "startCleaning 1회");
    SCHECK(sim.cleaning().startCalls[0] == CleaningLevel::HIGH, "레벨이 HIGH");
    SCHECK_EQ(sim.drive().forwardCount, 0, "DriveMotor 추가 호출 없음");
    return true;
}

// ST-P10: 먼지 감지 후 타이머 만료 → NORMAL 자동 복귀
static bool P10(std::string& failMsg) {
    RVCSimulator sim(false, 80); // 80ms 타이머
    sim.start();
    sim.resetRecording();
    sim.dustDetected();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    const auto& calls = sim.cleaning().startCalls;
    SCHECK(calls.size() >= 2u, "startCleaning 최소 2회(HIGH + NORMAL)");
    SCHECK(calls[0] == CleaningLevel::HIGH,       "첫 번째 호출이 HIGH");
    SCHECK(calls.back() == CleaningLevel::NORMAL,  "마지막 호출이 NORMAL (타이머 만료)");
    return true;
}

// ══════════════════════════════════════════════════════════════════════════
// NEGATIVE 테스트 (ST-N01 ~ ST-N20)
// ══════════════════════════════════════════════════════════════════════════

// ST-N01: IDLE에서 sideStatus(LEFT) → turn 미호출
static bool N01(std::string& failMsg) {
    RVCSimulator sim;
    sim.sideStatus(D::LEFT, true);
    SCHECK_EQ(sim.drive().turnCalls.size(), 0u, "IDLE에서 turn 미호출");
    SCHECK_EQ(sim.drive().forwardCount,     0,  "IDLE에서 forward 미호출");
    return true;
}

// ST-N02: IDLE에서 sideStatus(LEFT,false) → 모든 모터 미호출
static bool N02(std::string& failMsg) {
    RVCSimulator sim;
    sim.sideStatus(D::LEFT, false);
    SCHECK_EQ(sim.drive().forwardCount,     0, "forward 0");
    SCHECK_EQ(sim.drive().backwardCount,    0, "backward 0");
    SCHECK_EQ(sim.drive().stopCount,        0, "stop 0");
    SCHECK_EQ(sim.drive().turnCalls.size(), 0u, "turn 0");
    SCHECK_EQ(sim.cleaning().stopCount,     0, "stopCleaning 0");
    return true;
}

// ST-N03: CLEANING에서 sideStatus(LEFT) → turn/rotateRight 미호출
static bool N03(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.resetRecording();
    sim.sideStatus(D::LEFT, true);
    SCHECK_EQ(sim.drive().turnCalls.size(),  0u, "CLEANING에서 turn 미호출");
    SCHECK_EQ(sim.drive().rotateRightCount,  0,  "CLEANING에서 rotateRight 미호출");
    return true;
}

// ST-N04: CLEANING에서 sideStatus(LEFT,false) → 모터 추가 호출 없음
static bool N04(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.resetRecording();
    sim.sideStatus(D::LEFT, false);
    SCHECK_EQ(sim.drive().forwardCount,     0, "forward 추가 없음");
    SCHECK_EQ(sim.drive().turnCalls.size(), 0u, "turn 없음");
    SCHECK_EQ(sim.drive().rotateRightCount, 0, "rotateRight 없음");
    return true;
}

// ST-N05: REVERSING에서 좌우 막힘 → stop/turn/forward 미호출
static bool N05(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    enterReversing(sim);
    sim.resetRecording();
    sim.sideStatus(D::LEFT, false);
    sim.sideStatus(D::RIGHT, false);
    SCHECK_EQ(sim.drive().stopCount,        0, "stop 미호출");
    SCHECK_EQ(sim.drive().turnCalls.size(), 0u, "turn 미호출");
    SCHECK_EQ(sim.drive().forwardCount,     0,  "forward 미호출");
    return true;
}

// ST-N06: REVERSING에서 좌우 막힘 3회 반복 → forward 총 0회
static bool N06(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    enterReversing(sim);
    sim.resetRecording();
    sim.sideStatus(D::LEFT, false); sim.sideStatus(D::RIGHT, false);
    sim.sideStatus(D::LEFT, false); sim.sideStatus(D::RIGHT, false);
    sim.sideStatus(D::LEFT, false); sim.sideStatus(D::RIGHT, false);
    SCHECK_EQ(sim.drive().forwardCount,     0,  "forward 3회 반복 후도 0");
    SCHECK_EQ(sim.drive().turnCalls.size(), 0u, "turn 미호출");
    return true;
}

// ST-N07: dustDetected 연속 2회 → startCleaning(HIGH) 1회만
static bool N07(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.resetRecording();
    sim.dustDetected();
    sim.dustDetected();
    long highCount = std::count(sim.cleaning().startCalls.begin(),
                                sim.cleaning().startCalls.end(),
                                CleaningLevel::HIGH);
    SCHECK_EQ((int)highCount, 1, "startCleaning(HIGH) 정확히 1회");
    return true;
}

// ST-N08: dustDetected → DriveMotor에 어떤 호출도 없음
static bool N08(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.resetRecording();
    sim.dustDetected();
    SCHECK_EQ(sim.drive().forwardCount,     0, "forward 추가 없음");
    SCHECK_EQ(sim.drive().backwardCount,    0, "backward 없음");
    SCHECK_EQ(sim.drive().stopCount,        0, "stop 없음");
    SCHECK_EQ(sim.drive().turnCalls.size(), 0u, "turn 없음");
    return true;
}

// ST-N09: frontObstacleDetected → CleaningMotor.startCleaning 미호출
static bool N09(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.resetRecording();
    sim.frontObstacleDetected();
    SCHECK_EQ(sim.cleaning().startCalls.size(), 0u, "startCleaning 미호출");
    SCHECK_EQ(sim.cleaning().stopCount, 1, "stopCleaning 1회");
    return true;
}

// ST-N10: E1 진입 — startCleaning 미호출 (allSidesBlocked 제거)
// [v2.0] REVERSING 진입은 frontObstacleDetected + 두 sideStatus로만 가능
static bool N10(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.resetRecording();
    enterReversing(sim);
    SCHECK_EQ(sim.cleaning().startCalls.size(), 0u, "startCleaning 미호출");
    SCHECK_EQ(sim.cleaning().stopCount, 1, "stopCleaning 1회");
    return true;
}

// ST-N11: 좌·우 모두 여유 → turn 정확히 1회, LEFT 선택
static bool N11(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.frontObstacleDetected();
    sim.sideStatus(D::LEFT, true);
    sim.sideStatus(D::RIGHT, true);
    SCHECK_EQ(sim.drive().turnCalls.size(), 1u, "turn 정확히 1회");
    SCHECK(sim.drive().turnCalls[0] == D::LEFT, "[v2.0] LEFT 우선 선택");
    return true;
}

// ST-N12: UC-002 호출 순서 — stopCleaning이 DriveMotor.stop 보다 선행
static bool N12(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.frontObstacleDetected();
    SCHECK(before(sim.eventLog(), "clean:stop", "drive:stop"),
           "SD-002: stopCleaning → stop 순서");
    return true;
}

// ST-N13: UC-002 호출 순서 — turn이 startCleaning(NORMAL) 보다 선행
static bool N13(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.resetRecording();
    sim.frontObstacleDetected();
    sim.sideStatus(D::LEFT, true);
    sim.sideStatus(D::RIGHT, false);
    SCHECK(before(sim.eventLog(), "drive:turn(L)", "clean:start(NORMAL)"),
           "SD-002: turn → startCleaning 순서");
    return true;
}

// ST-N14: UC-003 호출 순서 — moveForward가 startCleaning(NORMAL) 보다 선행
static bool N14(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.resetRecording();
    enterReversing(sim);
    sim.sideStatus(D::LEFT, false);
    sim.sideStatus(D::RIGHT, true);
    SCHECK(before(sim.eventLog(), "drive:forward", "clean:start(NORMAL)"),
           "SD-003: forward → startCleaning 순서");
    return true;
}

// ST-N15: start() 후 DriveMotor.stop 미호출
static bool N15(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    SCHECK_EQ(sim.drive().stopCount, 0, "아무 이벤트 없으면 stop 없음");
    return true;
}

// ST-N16: start() 후 moveForward 정확히 1회
static bool N16(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    SCHECK_EQ(sim.drive().forwardCount, 1, "start 직후 forward 정확히 1회");
    return true;
}

// ST-N17: E1 진입 — stopCleaning 정확히 1회, moveBackward 정확히 1회
// [v2.0] allSidesBlocked() 삭제. frontObstacleDetected에서만 stopCleaning 호출.
static bool N17(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.resetRecording();
    enterReversing(sim);
    SCHECK_EQ(sim.cleaning().stopCount,  1, "stopCleaning 정확히 1회 (중복 없음)");
    SCHECK_EQ(sim.drive().backwardCount, 1, "moveBackward 정확히 1회");
    return true;
}

// ST-N18: REVERSING → frontObstacleDetected → STOPPED → 좌측 회피 처리
static bool N18(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    enterReversing(sim);
    sim.frontObstacleDetected();
    sim.resetRecording();
    sim.sideStatus(D::LEFT, true);
    sim.sideStatus(D::RIGHT, false);
    SCHECK_EQ(sim.drive().turnCalls.size(), 1u, "STOPPED 상태에서 turn 처리됨");
    SCHECK(sim.drive().turnCalls[0] == D::LEFT, "방향 LEFT");
    SCHECK_EQ(sim.drive().forwardCount, 1, "moveForward 1회");
    return true;
}

// ST-N19: 장애물 회피 두 번 연속 → turn 총 2회
static bool N19(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.frontObstacleDetected();
    sim.sideStatus(D::LEFT, false);
    sim.sideStatus(D::RIGHT, true);
    sim.frontObstacleDetected();
    sim.sideStatus(D::LEFT, true);
    sim.sideStatus(D::RIGHT, false);
    SCHECK_EQ(sim.drive().turnCalls.size(), 2u, "turn 총 2회");
    SCHECK(sim.drive().turnCalls[0] == D::RIGHT, "1차 회피 RIGHT");
    SCHECK(sim.drive().turnCalls[1] == D::LEFT,  "2차 회피 LEFT");
    return true;
}

// ST-N20: 고출력 중 장애물 감지 → stop 1회, 마지막 호출 NORMAL
static bool N20(std::string& failMsg) {
    RVCSimulator sim;
    sim.start();
    sim.resetRecording();
    sim.dustDetected();
    sim.frontObstacleDetected();
    sim.sideStatus(D::LEFT, false);
    sim.sideStatus(D::RIGHT, true);

    SCHECK(sim.cleaning().startCalls[0] == CleaningLevel::HIGH, "첫 호출 HIGH");
    SCHECK_EQ(sim.cleaning().stopCount, 1, "stopCleaning 1회");
    SCHECK(sim.cleaning().startCalls.back() == CleaningLevel::NORMAL, "마지막 호출 NORMAL");
    SCHECK_EQ(sim.drive().stopCount, 1, "DriveMotor stop 정확히 1회");
    return true;
}

// ══════════════════════════════════════════════════════════════════════════
int main() {
    TestRunner runner;

    const std::string POS = "Positive  [UC 정상 동작 검증]";
    runner.add("ST-P01", POS, "자동 청소 시작 — NORMAL + forward",                    P01);
    runner.add("ST-P02", POS, "전방 장애물 감지 — stopCleaning + stop",               P02);
    runner.add("ST-P03", POS, "장애물 회피 우측 — 완전 시퀀스",                       P03);
    runner.add("ST-P04", POS, "장애물 회피 좌측 — 완전 시퀀스",                       P04);
    runner.add("ST-P05", POS, "장애물 좌·우 모두 여유 — LEFT 우선 선택 (v2.0)",       P05);
    runner.add("ST-P06", POS, "삼면 장애물 후 우측 전진 — 순서 포함",                 P06);
    runner.add("ST-P07", POS, "삼면 장애물 후 좌측 전진 — 순서 포함",                 P07);
    runner.add("ST-P08", POS, "삼면 장애물 — 좌우 막힘 반복 후 방향 확보",             P08);
    runner.add("ST-P09", POS, "먼지 감지 — HIGH 전환",                                P09);
    runner.add("ST-P10", POS, "먼지 감지 후 타이머 만료 — NORMAL 자동 복귀",          P10);

    const std::string NEG = "Negative  [상태·순서·격리 위반 검증]";
    runner.add("ST-N01", NEG, "IDLE에서 sideStatus(LEFT) — turn 미호출",               N01);
    runner.add("ST-N02", NEG, "IDLE에서 sideStatus(LEFT,false) — 모든 모터 미호출",    N02);
    runner.add("ST-N03", NEG, "CLEANING에서 sideStatus(LEFT) — turn 미호출",           N03);
    runner.add("ST-N04", NEG, "CLEANING에서 sideStatus(LEFT,false) — 모터 추가 없음",  N04);
    runner.add("ST-N05", NEG, "REVERSING에서 좌우 막힘 — stop/turn/forward 없음",      N05);
    runner.add("ST-N06", NEG, "REVERSING에서 좌우 막힘 3회 — forward 0회",             N06);
    runner.add("ST-N07", NEG, "dustDetected 2회 — startCleaning(HIGH) 1회만",          N07);
    runner.add("ST-N08", NEG, "dustDetected — DriveMotor 호출 전혀 없음",              N08);
    runner.add("ST-N09", NEG, "frontObstacleDetected — startCleaning 미호출",          N09);
    runner.add("ST-N10", NEG, "E1 진입 — startCleaning 미호출 (allSidesBlocked 제거)", N10);
    runner.add("ST-N11", NEG, "좌·우 모두 여유 — turn 1회, LEFT 선택",                N11);
    runner.add("ST-N12", NEG, "UC-002 순서: stopCleaning → stop",                     N12);
    runner.add("ST-N13", NEG, "UC-002 순서: turn → startCleaning(NORMAL)",             N13);
    runner.add("ST-N14", NEG, "UC-003 순서: forward → startCleaning(NORMAL)",          N14);
    runner.add("ST-N15", NEG, "start() 후 DriveMotor.stop 미호출",                    N15);
    runner.add("ST-N16", NEG, "start() 후 moveForward 정확히 1회",                    N16);
    runner.add("ST-N17", NEG, "E1 진입 — stopCleaning 1회, moveBackward 1회",         N17);
    runner.add("ST-N18", NEG, "REVERSING → frontObstacleDetected → sideStatus 처리",  N18);
    runner.add("ST-N19", NEG, "장애물 2회 연속 — turn 총 2회",                         N19);
    runner.add("ST-N20", NEG, "고출력 중 장애물 — stop 1회, 마지막 호출 NORMAL",       N20);

    return runner.run();
}
