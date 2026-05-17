#include "RVCSimulator.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <string>

static void pause(int ms = 400) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

static void header(const std::string& title) {
    const int W = 60;
    std::string bar(W, '-');
    std::cout << "\n\033[1;44;37m  " << title;
    for (int i = (int)title.size(); i < W - 2; ++i) std::cout << ' ';
    std::cout << "\033[0m\n" << "\033[34m" << bar << "\033[0m\n";
}

static void stateBox(const std::string& state, const std::string& drive, const std::string& clean) {
    std::cout << "\n  \033[1;30m┌─ 시스템 상태 ";
    std::cout << "─────────────────────────────────┐\033[0m\n";
    std::cout << "  \033[1;30m│\033[0m  상태: \033[1;32m" << state << "\033[0m";
    std::cout << "  구동: \033[1;36m" << drive << "\033[0m";
    std::cout << "  청소: \033[1;35m" << clean << "\033[0m\n";
    std::cout << "  \033[1;30m└───────────────────────────────────────────────┘\033[0m\n";
}

static void section(const std::string& s) {
    std::cout << "\n\033[33m  〔 " << s << " 〕\033[0m\n";
}

int main() {
    std::cout << "\033[1;37m\n"
              << "  ╔══════════════════════════════════════════════════════╗\n"
              << "  ║        RVC SW Controller 시뮬레이터                 ║\n"
              << "  ╚══════════════════════════════════════════════════════╝\n"
              << "\033[0m";

    // ──────────────────────────────────────────────────────────
    header("시나리오 1: 자동 청소 시작 (UC-001)");
    {
        RVCSimulator sim(true);
        sim.start();
        stateBox("CLEANING", "전진 ▶", "NORMAL ◎");
    }

    pause();

    // ──────────────────────────────────────────────────────────
    header("시나리오 2: 전방 장애물 — 우측 방향 전환 (UC-002)");
    {
        RVCSimulator sim(true);
        section("UC-001 실행 중");
        sim.start();
        pause(300);

        section("전방 장애물 감지");
        sim.frontObstacleDetected();
        pause(300);

        section("우측 여유 공간 확인 → 우측 전환");
        sim.sideStatus({Direction::RIGHT});
        stateBox("CLEANING", "전진 ▶", "NORMAL ◎");
    }

    pause();

    // ──────────────────────────────────────────────────────────
    header("시나리오 3: 전방 장애물 — 좌측 방향 전환 (UC-002)");
    {
        RVCSimulator sim(true);
        section("UC-001 실행 중");
        sim.start();
        pause(300);

        section("전방 장애물 감지");
        sim.frontObstacleDetected();
        pause(300);

        section("좌측 여유 공간 확인 → 좌측 전환");
        sim.sideStatus({Direction::LEFT});
        stateBox("CLEANING", "전진 ▶", "NORMAL ◎");
    }

    pause();

    // ──────────────────────────────────────────────────────────
    header("시나리오 4: 삼면 장애물 — 후진 후 우측 전진 (UC-003)");
    {
        RVCSimulator sim(true);
        section("UC-001 실행 중");
        sim.start();
        pause(300);

        section("전·좌·우 모두 장애물 감지");
        sim.allSidesBlocked();
        stateBox("REVERSING", "후진 ◀", "OFF ✗");
        pause(300);

        section("후진 중 — 우측 여유 확보");
        sim.sideStatus({Direction::RIGHT});
        stateBox("CLEANING", "전진 ▶", "NORMAL ◎");
    }

    pause();

    // ──────────────────────────────────────────────────────────
    header("시나리오 5: 삼면 장애물 — 반복 후진 후 방향 확보 (UC-003)");
    {
        RVCSimulator sim(true);
        section("UC-001 실행 중");
        sim.start();
        pause(200);

        sim.allSidesBlocked();
        stateBox("REVERSING", "후진 ◀", "OFF ✗");
        pause(200);

        section("후진 중 — 여전히 삼면 막힘 (×2)");
        sim.sideStatus({});
        pause(200);
        sim.sideStatus({});
        pause(200);

        section("후진 중 — 좌측 확보");
        sim.sideStatus({Direction::LEFT});
        stateBox("CLEANING", "전진 ▶", "NORMAL ◎");
    }

    pause();

    // ──────────────────────────────────────────────────────────
    header("시나리오 6: 먼지 감지 — 고출력 후 자동 복귀 (UC-004)");
    {
        RVCSimulator sim(true, 500); // 500ms 타이머
        section("UC-001 실행 중");
        sim.start();
        pause(200);

        section("먼지 감지 → HIGH 출력 전환");
        sim.dustDetected();
        stateBox("CLEANING", "전진 ▶", "HIGH ★");
        pause(200);

        section("타이머 만료 대기 (500ms)...");
        std::this_thread::sleep_for(std::chrono::milliseconds(700));
        stateBox("CLEANING", "전진 ▶", "NORMAL ◎");
        std::cout << "  \033[34m← [CleaningMotor] startCleaning(NORMAL)  [타이머 만료]\033[0m\n";
    }

    pause();

    // ──────────────────────────────────────────────────────────
    header("시나리오 7: 먼지 재감지 — 타이머 리셋 (UC-004 A2)");
    {
        RVCSimulator sim(true, 600);
        section("UC-001 실행 중");
        sim.start();
        pause(200);

        section("첫 번째 먼지 감지 → HIGH + 타이머 시작");
        sim.dustDetected();
        pause(300);

        section("타이머 만료 전 두 번째 먼지 감지 → 타이머 리셋 (motor 재호출 없음)");
        sim.dustDetected();
        pause(200);

        section("타이머 만료 대기...");
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
        std::cout << "  \033[34m← [CleaningMotor] startCleaning(NORMAL)  [타이머 만료]\033[0m\n";
        stateBox("CLEANING", "전진 ▶", "NORMAL ◎");
    }

    std::cout << "\n\033[1;32m  ✔ 모든 시뮬레이션 시나리오 완료\033[0m\n\n";
    return 0;
}
