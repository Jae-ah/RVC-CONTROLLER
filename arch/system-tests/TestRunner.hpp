#pragma once
#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <iomanip>
#include <stdexcept>

// Google Test 없이 사용하는 경량 테스트 프레임워크.
// 각 테스트 함수는 (std::string& failMsg) 를 받아 bool 을 반환한다.
class TestRunner {
public:
    using TestFn = std::function<bool(std::string&)>;

    void add(const std::string& id, const std::string& category,
             const std::string& name, TestFn fn) {
        tests_.push_back({id, category, name, fn});
    }

    // 전체 실행 후 0(전부 통과) 또는 실패 수를 반환한다
    int run() {
        int passed = 0, failed = 0;
        results_.clear();

        printBanner();

        std::string lastCat;
        for (auto& t : tests_) {
            if (t.category != lastCat) {
                lastCat = t.category;
                std::cout << "\n  \033[1;33m── " << t.category
                          << " ──────────────────────────────────────\033[0m\n";
            }

            std::string msg;
            bool ok = false;
            try { ok = t.fn(msg); }
            catch (const std::exception& e) { msg = std::string("exception: ") + e.what(); }

            results_.push_back({t.id, t.category, t.name, ok, msg});
            if (ok) ++passed; else ++failed;

            // 한 줄 결과
            std::cout << (ok ? "  \033[32m✔\033[0m" : "  \033[31m✘\033[0m");
            std::cout << " \033[90m" << std::setw(6) << std::left << t.id << "\033[0m ";
            std::cout << t.name;
            if (!ok) std::cout << "\n    \033[31m→ " << msg << "\033[0m";
            std::cout << "\n";
        }

        printSummary(passed, failed);
        return failed;
    }

private:
    struct Test   { std::string id, category, name; TestFn fn; };
    struct Result { std::string id, category, name; bool ok; std::string msg; };
    std::vector<Test>   tests_;
    std::vector<Result> results_;

    static void printBanner() {
        std::cout <<
            "\n\033[1;44;37m"
            "  ╔══════════════════════════════════════════════════════════╗  \n"
            "  ║         RVC 시스템 테스트 리포트                        ║  \n"
            "  ╚══════════════════════════════════════════════════════════╝  "
            "\033[0m\n";
    }

    void printSummary(int passed, int failed) const {
        std::cout <<
            "\n  \033[1;34m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m\n";
        std::cout << "  결과   전체: " << (passed + failed)
                  << "  \033[32m통과: " << passed << "\033[0m"
                  << "  \033[31m실패: " << failed << "\033[0m\n";

        // 실패 목록
        if (failed > 0) {
            std::cout << "\n  \033[31m실패 목록:\033[0m\n";
            for (auto& r : results_)
                if (!r.ok)
                    std::cout << "    • " << r.id << " " << r.name
                              << "\n      → " << r.msg << "\n";
        }

        std::cout << "\n  \033[1;34m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m\n\n";
    }
};

// ── 검증 매크로 ────────────────────────────────────────────────────────────
// 내부적으로 failMsg 에 실패 이유를 기록하고 false 를 반환한다.
#define SCHECK(cond, msg)                                           \
    do { if (!(cond)) { failMsg = (msg); return false; } } while(0)

#define SCHECK_EQ(a, b, label)                                      \
    do { if ((a) != (b)) {                                          \
        failMsg = std::string(label) + ": expected " +              \
                  std::to_string(b) + " got " + std::to_string(a); \
        return false; } } while(0)

#define SCHECK_LT(a, b, label)                                      \
    do { if (!((a) < (b))) {                                        \
        failMsg = std::string(label) + ": " +                       \
                  std::to_string(a) + " not < " + std::to_string(b);\
        return false; } } while(0)
