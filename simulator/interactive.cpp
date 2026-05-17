/*
 * interactive.cpp — RVC 인터랙티브 터미널 시뮬레이터
 *
 * [편집 모드]
 *   화살표키   : 커서 이동
 *   Space      : 장애물(#) 토글
 *   D          : 먼지(·) 토글
 *   R          : RVC 시작 위치 설정
 *   S / Enter  : 시뮬레이션 시작
 *   C          : 맵 초기화
 *   Q          : 종료
 *
 * [시뮬레이션 모드]
 *   P          : 일시정지 / 재개
 *   E          : 편집 모드로 돌아가기
 *   Q          : 종료
 */

#include "../src/RVCController.hpp"
#include "../src/NavigationController.hpp"
#include "../src/CleaningController.hpp"
#include "../src/IDriveMotor.hpp"
#include "../src/ICleaningMotor.hpp"
#include "../src/Direction.hpp"
#include "../src/CleaningLevel.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <deque>
#include <queue>
#include <algorithm>
#include <thread>
#include <chrono>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstring>
#include <cassert>

// ─────────────────────────────────────────────────────────
// 상수
// ─────────────────────────────────────────────────────────
static constexpr int ROWS      = 12;
static constexpr int COLS      = 20;
static constexpr int TICK_MS   = 350;   // 시뮬레이션 틱 간격
static constexpr int RENDER_MS = 50;    // 렌더링 간격
static constexpr int LOG_SIZE  = 6;     // 이벤트 로그 표시 줄 수

// ─────────────────────────────────────────────────────────
// ANSI 헬퍼
// ─────────────────────────────────────────────────────────
#define RESET    "\033[0m"
#define BOLD     "\033[1m"
#define RED      "\033[31m"
#define GREEN    "\033[32m"
#define YELLOW   "\033[33m"
#define BLUE     "\033[34m"
#define MAGENTA  "\033[35m"
#define CYAN     "\033[36m"
#define WHITE    "\033[37m"
#define GRAY     "\033[90m"
#define BGRED    "\033[41m"
#define BGGREEN  "\033[42m"
#define BGYELLOW "\033[43m"
#define BGBLUE   "\033[44m"

static void clearScreen()  { std::cout << "\033[2J\033[H"; }
static void moveCursor(int row, int col) {
    std::cout << "\033[" << row << ";" << col << "H";
}
static void hideCursor() { std::cout << "\033[?25l"; }
static void showCursor() { std::cout << "\033[?25h"; }

// ─────────────────────────────────────────────────────────
// 그리드 셀 타입
// ─────────────────────────────────────────────────────────
enum class Cell { EMPTY, WALL, DUST, CLEANED };

// ─────────────────────────────────────────────────────────
// 방향 (절대 방향: RVC 이동용)
// ─────────────────────────────────────────────────────────
enum class Heading { NORTH, EAST, SOUTH, WEST };

static Heading turnLeft(Heading h) {
    switch (h) {
        case Heading::NORTH: return Heading::WEST;
        case Heading::WEST:  return Heading::SOUTH;
        case Heading::SOUTH: return Heading::EAST;
        case Heading::EAST:  return Heading::NORTH;
    }
    return h;
}
static Heading turnRight(Heading h) {
    switch (h) {
        case Heading::NORTH: return Heading::EAST;
        case Heading::EAST:  return Heading::SOUTH;
        case Heading::SOUTH: return Heading::WEST;
        case Heading::WEST:  return Heading::NORTH;
    }
    return h;
}
static std::pair<int,int> delta(Heading h) {
    switch (h) {
        case Heading::NORTH: return {-1, 0};
        case Heading::SOUTH: return { 1, 0};
        case Heading::WEST:  return { 0,-1};
        case Heading::EAST:  return { 0, 1};
    }
    return {0,0};
}
static char headingArrow(Heading h) {
    switch (h) {
        case Heading::NORTH: return '^';
        case Heading::SOUTH: return 'v';
        case Heading::WEST:  return '<';
        case Heading::EAST:  return '>';
    }
    return '?';
}
static const char* headingName(Heading h) {
    switch (h) {
        case Heading::NORTH: return "NORTH";
        case Heading::SOUTH: return "SOUTH";
        case Heading::WEST:  return "WEST";
        case Heading::EAST:  return "EAST";
    }
    return "?";
}

// ─────────────────────────────────────────────────────────
// RVC 상태
// ─────────────────────────────────────────────────────────
struct SimRVC {
    int row, col;
    Heading heading;

    std::pair<int,int> ahead()  const { return step(heading); }
    std::pair<int,int> behind() const {
        auto [dr,dc] = delta(heading);
        return {row - dr, col - dc};
    }
    std::pair<int,int> lside()  const { return step(turnLeft(heading)); }
    std::pair<int,int> rside()  const { return step(turnRight(heading)); }

private:
    std::pair<int,int> step(Heading h) const {
        auto [dr,dc] = delta(h);
        return {row + dr, col + dc};
    }
};

// ─────────────────────────────────────────────────────────
// 이벤트 로그
// ─────────────────────────────────────────────────────────
struct MsgLog {
    std::deque<std::string> lines;
    void push(const std::string& s) {
        lines.push_back(s);
        while ((int)lines.size() > LOG_SIZE) lines.pop_front();
    }
};

// ─────────────────────────────────────────────────────────
// GridDriveMotor: IDriveMotor 구현 — 그리드 위 RVC 이동
// ─────────────────────────────────────────────────────────
enum class Motion { IDLE, FWD, BWD, STOPPED };

class GridDriveMotor : public IDriveMotor {
public:
    GridDriveMotor(SimRVC& rvc, const std::vector<std::vector<Cell>>& grid, MsgLog& log)
        : rvc_(rvc), grid_(grid), log_(log) {}

    void moveForward() override {
        auto [nr, nc] = rvc_.ahead();
        motion_ = Motion::FWD;
        if (inBounds(nr, nc) && grid_[nr][nc] != Cell::WALL) {
            rvc_.row = nr; rvc_.col = nc;
        }
        log_.push(GREEN "  [Drive] moveForward()" RESET);
    }
    void moveBackward() override {
        auto [nr, nc] = rvc_.behind();
        motion_ = Motion::BWD;
        if (inBounds(nr, nc) && grid_[nr][nc] != Cell::WALL) {
            rvc_.row = nr; rvc_.col = nc;
        }
        log_.push(YELLOW "  [Drive] moveBackward()" RESET);
    }
    void stop() override {
        motion_ = Motion::STOPPED;
        log_.push(GRAY "  [Drive] stop()" RESET);
    }
    void turn(Direction dir) override {
        if (dir == Direction::LEFT) {
            rvc_.heading = turnLeft(rvc_.heading);
            lastTurn_ = Direction::LEFT;
            log_.push(CYAN "  [Drive] turn(LEFT)" RESET);
        } else {
            rvc_.heading = turnRight(rvc_.heading);
            lastTurn_ = Direction::RIGHT;
            log_.push(CYAN "  [Drive] turn(RIGHT)" RESET);
        }
    }

    Motion    motion()   const { return motion_; }
    Direction lastTurn() const { return lastTurn_; }

private:
    SimRVC& rvc_;
    const std::vector<std::vector<Cell>>& grid_;
    MsgLog& log_;
    Motion    motion_   = Motion::IDLE;
    Direction lastTurn_ = Direction::RIGHT;

    bool inBounds(int r, int c) const {
        return r >= 0 && r < ROWS && c >= 0 && c < COLS;
    }
};

// ─────────────────────────────────────────────────────────
// GridCleaningMotor: ICleaningMotor 구현
// ─────────────────────────────────────────────────────────
class GridCleaningMotor : public ICleaningMotor {
public:
    explicit GridCleaningMotor(MsgLog& log) : log_(log) {}

    void startCleaning(CleaningLevel level) override {
        level_ = level;
        running_ = true;
        if (level == CleaningLevel::HIGH)
            log_.push(MAGENTA "  [Clean] startCleaning(HIGH)" RESET);
        else
            log_.push(GREEN "  [Clean] startCleaning(NORMAL)" RESET);
    }
    void stopCleaning() override {
        running_ = false;
        level_   = CleaningLevel::NORMAL;
        log_.push(GRAY "  [Clean] stopCleaning()" RESET);
    }

    bool         running() const { return running_; }
    CleaningLevel level()  const { return level_; }

private:
    MsgLog& log_;
    bool          running_ = false;
    CleaningLevel level_   = CleaningLevel::NORMAL;
};

// ─────────────────────────────────────────────────────────
// Simulation: 하나의 시뮬레이션 실행 단위
// ─────────────────────────────────────────────────────────
enum class Phase { MOVING_FWD, REVERSING };

struct Simulation {
    SimRVC                         rvc;
    std::vector<std::vector<Cell>> grid;
    MsgLog&                        log;

    GridDriveMotor    drive;
    GridCleaningMotor clean;
    NavigationController nav;
    CleaningController   cleanCtrl;
    RVCController        rvcCtrl;

    Phase   phase   = Phase::MOVING_FWD;
    bool    paused  = false;
    bool    done    = false;
    int     cleaned = 0;
    Heading sweepH_          = Heading::EAST;
    Heading advanceH_        = Heading::SOUTH;
    bool    needTurnToSweep_ = false;
    bool    inBacktrack_     = false;
    std::vector<std::pair<int,int>> backtrackPath_;

    Simulation(const SimRVC& startRvc,
               const std::vector<std::vector<Cell>>& startGrid,
               MsgLog& msgLog)
        : rvc(startRvc), grid(startGrid), log(msgLog),
          drive(rvc, grid, log),
          clean(log),
          nav(drive),
          cleanCtrl(clean, 3000),
          rvcCtrl(nav, cleanCtrl)
    {
        rvcCtrl.start();
        markCleaned();
    }

    // 틱 실행: 한 스텝 진행
    void tick() {
        if (paused || done) return;

        if (needTurnToSweep_) {
            // 한 칸 전진(행 이동) 직후: 새 스윕 방향으로 헤딩 전환 (이동 없음)
            needTurnToSweep_ = false;
            rvc.heading = sweepH_;
            log.push(std::string(CYAN) + "  [Sweep] 방향 전환→" + headingName(sweepH_) + RESET);
        } else if (phase == Phase::MOVING_FWD) {
            tickMovingFwd();
        } else {
            tickReversing();
        }

        if (grid[rvc.row][rvc.col] == Cell::DUST) {
            rvcCtrl.dustDetected();
            log.push(MAGENTA "  [Dust] dustDetected()" RESET);
            grid[rvc.row][rvc.col] = Cell::CLEANED;
        }

        markCleaned();
        checkDone();
    }

private:
    bool inBounds(int r, int c) const {
        return r >= 0 && r < ROWS && c >= 0 && c < COLS;
    }
    bool isBlocked(std::pair<int,int> pos) const {
        auto [r, c] = pos;
        if (!inBounds(r, c)) return true;
        return grid[r][c] == Cell::WALL;
    }
    void markCleaned() {
        auto& cell = grid[rvc.row][rvc.col];
        if (cell == Cell::EMPTY || cell == Cell::DUST) {
            if (cell == Cell::EMPTY) {
                cell = Cell::CLEANED;
                ++cleaned;
            }
        }
    }
    void checkDone() {
        for (auto& row : grid)
            for (auto& c : row)
                if (c == Cell::DUST || c == Cell::EMPTY) return;
        done = true;
        log.push(GREEN BOLD "  청소 완료!" RESET);
    }

    // 특정 방향으로 회전 후 직선상 미청소 셀 수 (많을수록 우선)
    int scoreDir(Direction d) const {
        Heading newH = (d == Direction::RIGHT) ? ::turnRight(rvc.heading)
                                               : ::turnLeft(rvc.heading);
        auto [dr, dc] = ::delta(newH);
        int r = rvc.row + dr, c = rvc.col + dc;
        int score = 0;
        for (int i = 0; i < ROWS + COLS; ++i) {
            if (!inBounds(r, c) || grid[r][c] == Cell::WALL) break;
            if (grid[r][c] == Cell::EMPTY || grid[r][c] == Cell::DUST) ++score;
            r += dr; c += dc;
        }
        return score;
    }

    // 현재 헤딩 기준 target 방향으로 90° 회전하는 Direction 반환
    Direction turnDirTo(Heading target) const {
        return (::turnRight(rvc.heading) == target) ? Direction::RIGHT : Direction::LEFT;
    }

    // 바운스트로펜(S자) 전체 공간 커버리지 방향 선택
    // 핵심: 스윕 끝에서 한 행만 전진 후 needTurnToSweep_ 플래그로 복귀
    Direction pickBestTurn(const std::vector<Direction>& avail) {
        auto inAvail = [&](Direction d) {
            return std::find(avail.begin(), avail.end(), d) != avail.end();
        };

        // ① 스윕 방향으로 이동 중 막힘 → 한 행 전진 + sweepH_ 반전 (S자 핵심)
        if (rvc.heading == sweepH_) {
            Direction advDir = turnDirTo(advanceH_);
            if (inAvail(advDir)) {
                sweepH_          = (sweepH_ == Heading::EAST) ? Heading::WEST : Heading::EAST;
                needTurnToSweep_ = true;
                return advDir;
            }
            // advanceH_ 반대 방향 시도 (예: 남쪽 막혔을 때 북쪽으로)
            Heading oppAdv = (advanceH_ == Heading::SOUTH) ? Heading::NORTH : Heading::SOUTH;
            Direction oppDir = turnDirTo(oppAdv);
            if (inAvail(oppDir)) {
                advanceH_        = oppAdv;
                sweepH_          = (sweepH_ == Heading::EAST) ? Heading::WEST : Heading::EAST;
                needTurnToSweep_ = true;
                return oppDir;
            }
        }

        // ② 전진(행 이동) 중 막힘 → sweepH_ 방향으로 조기 전환
        if (rvc.heading == advanceH_) {
            Direction toSweep = turnDirTo(sweepH_);
            if (inAvail(toSweep)) return toSweep;
        }

        // ③ 폴백: 미청소 셀이 많은 방향 선택
        bool rOk = inAvail(Direction::RIGHT);
        bool lOk = inAvail(Direction::LEFT);
        int  rs  = rOk ? scoreDir(Direction::RIGHT) : -1;
        int  ls  = lOk ? scoreDir(Direction::LEFT)  : -1;
        if (rs != ls) return (rs > ls) ? Direction::RIGHT : Direction::LEFT;
        Direction toSweepDir = turnDirTo(sweepH_);
        return inAvail(toSweepDir) ? toSweepDir : avail[0];
    }

    void tickMovingFwd() {
        if (inBacktrack_) { tickBacktrack(); return; }

        if (isBlocked(rvc.ahead())) {
            std::vector<Direction> avail;
            if (!isBlocked(rvc.rside())) avail.push_back(Direction::RIGHT);
            if (!isBlocked(rvc.lside())) avail.push_back(Direction::LEFT);

            if (!avail.empty()) {
                // 모든 avail 방향에 미청소 셀이 없으면 BFS 백트래킹 시작
                bool allCleaned = std::all_of(avail.begin(), avail.end(),
                    [&](Direction d) { return scoreDir(d) == 0; });
                if (allCleaned) {
                    backtrackPath_ = bfsToNearestUnvisited();
                    if (!backtrackPath_.empty()) {
                        inBacktrack_ = true;
                        log.push(CYAN "  [BFS] 백트래킹 시작" RESET);
                        return;
                    }
                }
                std::vector<Direction> chosen = {pickBestTurn(avail)};
                rvcCtrl.frontObstacleDetected();
                rvcCtrl.sideStatus(chosen);
            } else {
                rvcCtrl.allSidesBlocked();
                phase = Phase::REVERSING;
            }
        } else {
            auto [nr, nc] = rvc.ahead();
            if (inBounds(nr, nc) && grid[nr][nc] != Cell::WALL) {
                rvc.row = nr; rvc.col = nc;
            }
        }
    }

    void tickReversing() {
        std::vector<Direction> avail;
        if (!isBlocked(rvc.rside())) avail.push_back(Direction::RIGHT);
        if (!isBlocked(rvc.lside())) avail.push_back(Direction::LEFT);

        if (!avail.empty()) {
            rvcCtrl.sideStatus({pickBestTurn(avail)});
            phase = Phase::MOVING_FWD;
        } else {
            rvcCtrl.sideStatus({});
            auto [nr, nc] = rvc.behind();
            if (inBounds(nr, nc) && grid[nr][nc] != Cell::WALL) {
                rvc.row = nr; rvc.col = nc;
            }
        }
    }

    // BFS로 현재 위치에서 가장 가까운 미청소(EMPTY/DUST) 셀까지의 경로를 반환
    std::vector<std::pair<int,int>> bfsToNearestUnvisited() const {
        using P = std::pair<int,int>;
        std::vector<std::vector<bool>> vis(ROWS, std::vector<bool>(COLS, false));
        std::vector<std::vector<P>>    par(ROWS, std::vector<P>(COLS, {-1,-1}));
        std::queue<P> q;
        q.push({rvc.row, rvc.col});
        vis[rvc.row][rvc.col] = true;

        static constexpr int DR[] = {-1, 1, 0, 0};
        static constexpr int DC[] = {0, 0, -1, 1};

        P target = {-1, -1};
        while (!q.empty() && target.first == -1) {
            auto [r, c] = q.front(); q.pop();
            for (int d = 0; d < 4; ++d) {
                int nr = r + DR[d], nc = c + DC[d];
                if (!inBounds(nr, nc) || vis[nr][nc] || grid[nr][nc] == Cell::WALL) continue;
                vis[nr][nc] = true;
                par[nr][nc] = {r, c};
                if (grid[nr][nc] == Cell::EMPTY || grid[nr][nc] == Cell::DUST) {
                    target = {nr, nc};
                    break;
                }
                q.push({nr, nc});
            }
        }

        if (target.first == -1) return {};

        std::vector<P> path;
        for (P cur = target; cur != P{rvc.row, rvc.col}; cur = par[cur.first][cur.second])
            path.push_back(cur);
        std::reverse(path.begin(), path.end());
        return path;
    }

    // BFS 경로를 한 스텝씩 따라가는 틱
    void tickBacktrack() {
        if (backtrackPath_.empty()) { inBacktrack_ = false; return; }

        auto [tr, tc] = backtrackPath_.front();

        // 이미 해당 위치에 도달했으면 다음 목표로
        if (rvc.row == tr && rvc.col == tc) {
            backtrackPath_.erase(backtrackPath_.begin());
            if (backtrackPath_.empty()) {
                inBacktrack_ = false;
                log.push(GREEN "  [BFS] 미청소 구역 도달" RESET);
            }
            return;
        }

        // 목표 셀 방향 결정
        Heading needed;
        int dr = tr - rvc.row, dc = tc - rvc.col;
        if      (dr == -1) needed = Heading::NORTH;
        else if (dr ==  1) needed = Heading::SOUTH;
        else if (dc ==  1) needed = Heading::EAST;
        else               needed = Heading::WEST;

        // 방향이 다르면 이번 틱은 회전만 (이동 없음)
        if (rvc.heading != needed) {
            rvc.heading = needed;
            return;
        }

        // 경로가 막혔으면 재계산
        if (isBlocked(rvc.ahead())) {
            log.push(YELLOW "  [BFS] 경로 재계산" RESET);
            backtrackPath_ = bfsToNearestUnvisited();
            if (backtrackPath_.empty()) inBacktrack_ = false;
            return;
        }

        // 전진
        rvc.row = tr; rvc.col = tc;
        backtrackPath_.erase(backtrackPath_.begin());
        if (backtrackPath_.empty()) {
            inBacktrack_ = false;
            log.push(GREEN "  [BFS] 미청소 구역 도달" RESET);
        }
    }
};

// ─────────────────────────────────────────────────────────
// termios 유틸리티
// ─────────────────────────────────────────────────────────
struct RawTerm {
    termios orig;
    RawTerm() {
        tcgetattr(STDIN_FILENO, &orig);
        termios raw = orig;
        raw.c_lflag &= ~(ECHO | ICANON);
        raw.c_cc[VMIN]  = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    }
    ~RawTerm() {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig);
        showCursor();
        std::cout << RESET "\n";
    }
};

// 논블로킹 키 읽기. -1 = 키 없음, 그 외 = 키코드
static int readKey() {
    unsigned char buf[4] = {};
    int n = (int)read(STDIN_FILENO, buf, 4);
    if (n <= 0) return -1;
    if (n >= 3 && buf[0] == 27 && buf[1] == '[') {
        switch (buf[2]) {
            case 'A': return 1000; // UP
            case 'B': return 1001; // DOWN
            case 'C': return 1002; // RIGHT
            case 'D': return 1003; // LEFT
        }
    }
    if (buf[0] == 13 || buf[0] == 10) return '\n';
    return (int)buf[0];
}

// ─────────────────────────────────────────────────────────
// InteractiveSimulator
// ─────────────────────────────────────────────────────────
class InteractiveSimulator {
public:
    InteractiveSimulator() {
        grid_.assign(ROWS, std::vector<Cell>(COLS, Cell::EMPTY));
        startRVC_ = {ROWS / 2, COLS / 2, Heading::NORTH};
    }

    void run() {
        RawTerm rawTerm;
        hideCursor();
        editMode();
    }

private:
    std::vector<std::vector<Cell>> grid_;
    SimRVC  startRVC_;
    int     curRow_ = ROWS / 2;
    int     curCol_ = COLS / 2;
    MsgLog  log_;

    // ─── 편집 모드 ─────────────────────────────────────────
    void editMode() {
        bool dirty = true;  // 진입 시 첫 렌더링

        while (true) {
            int key = readKey();
            if (key != -1) {
                if (key == 'q' || key == 'Q') return;
                handleEditKey(key);
                dirty = true;   // 키 입력 시에만 재렌더링
            }

            if (dirty) {
                renderEdit();
                dirty = false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void handleEditKey(int key) {
        switch (key) {
            case 1000: if (curRow_ > 0)       --curRow_; break; // UP
            case 1001: if (curRow_ < ROWS - 1) ++curRow_; break; // DOWN
            case 1002: if (curCol_ < COLS - 1) ++curCol_; break; // RIGHT
            case 1003: if (curCol_ > 0)        --curCol_; break; // LEFT
            case ' ': {
                auto& c = grid_[curRow_][curCol_];
                c = (c == Cell::WALL) ? Cell::EMPTY : Cell::WALL;
                break;
            }
            case 'd': case 'D': {
                auto& c = grid_[curRow_][curCol_];
                c = (c == Cell::DUST) ? Cell::EMPTY : Cell::DUST;
                break;
            }
            case 'r': case 'R':
                startRVC_ = {curRow_, curCol_, Heading::NORTH};
                grid_[curRow_][curCol_] = Cell::EMPTY;
                break;
            case 'c': case 'C':
                grid_.assign(ROWS, std::vector<Cell>(COLS, Cell::EMPTY));
                startRVC_ = {ROWS / 2, COLS / 2, Heading::NORTH};
                curRow_ = ROWS / 2; curCol_ = COLS / 2;
                break;
            case '\n': case 's': case 'S':
                log_.lines.clear();
                simulateMode();
                break;
        }
    }

    // ─── 시뮬레이션 모드 ───────────────────────────────────
    void simulateMode() {
        // 편집 그리드 복사 (시뮬 중 그리드 변경; 원본 보존 위해 복사본 사용)
        auto gridCopy = grid_;
        // RVC 출발 셀은 빈칸으로
        gridCopy[startRVC_.row][startRVC_.col] = Cell::EMPTY;

        Simulation sim(startRVC_, gridCopy, log_);

        auto lastTick = std::chrono::steady_clock::now();
        bool dirty    = true;   // 진입 시 첫 렌더링

        while (true) {
            int key = readKey();
            if (key != -1) {
                char ch = (char)key;
                if (ch == 'q' || ch == 'Q') return;
                if (ch == 'e' || ch == 'E') return;
                if (ch == 'p' || ch == 'P') { sim.paused = !sim.paused; dirty = true; }
            }

            auto now    = std::chrono::steady_clock::now();
            auto tickMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTick).count();
            if (!sim.paused && !sim.done && tickMs >= TICK_MS) {
                sim.tick();
                lastTick = now;
                dirty = true;   // 틱 발생 시에만 재렌더링
            }

            if (dirty) {
                renderSim(sim);
                dirty = false;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    // ─── 렌더링: 편집 모드 ────────────────────────────────
    void renderEdit() {
        clearScreen();
        std::cout << BOLD BGBLUE WHITE
                  << "  RVC 인터랙티브 시뮬레이터 — 편집 모드                    "
                  << RESET "\n";

        printGrid(nullptr, curRow_, curCol_);
        printEditHelp();
        std::cout.flush();
    }

    // ─── 렌더링: 시뮬레이션 모드 ─────────────────────────
    void renderSim(const Simulation& sim) {
        clearScreen();

        // 상태 결정
        const char* stateStr = "CLEANING";
        if (sim.phase == Phase::REVERSING) stateStr = "REVERSING";
        if (sim.done) stateStr = "DONE";
        if (sim.paused) stateStr = "PAUSED";

        const char* levelStr = sim.clean.level() == CleaningLevel::HIGH ? "HIGH ★" : "NORMAL ◎";

        std::cout << BOLD BGBLUE WHITE
                  << "  RVC 인터랙티브 시뮬레이터 — 시뮬레이션 모드              "
                  << RESET "\n";

        printGrid(&sim, -1, -1);
        printSimStatus(sim, stateStr, levelStr);
        printSimHelp();
        printLog();
        std::cout.flush();
    }

    // ─── 그리드 출력 ──────────────────────────────────────
    void printGrid(const Simulation* sim, int curR, int curC) {
        // 테두리 상단
        std::cout << "  " GRAY "┌";
        for (int c = 0; c < COLS; ++c) std::cout << "──";
        std::cout << "┐" RESET "\n";

        const auto& grid = sim ? sim->grid : grid_;
        const SimRVC* rvc = sim ? &sim->rvc : nullptr;

        for (int r = 0; r < ROWS; ++r) {
            std::cout << "  " GRAY "│" RESET;
            for (int c = 0; c < COLS; ++c) {
                bool isRVC    = rvc && rvc->row == r && rvc->col == c;
                bool isCursor = (curR == r && curC == c);
                bool isStart  = !sim && startRVC_.row == r && startRVC_.col == c;

                if (isRVC) {
                    // RVC 표시
                    const char* rvcClr = GREEN;
                    if (sim->clean.level() == CleaningLevel::HIGH) rvcClr = MAGENTA;
                    if (sim->phase == Phase::REVERSING) rvcClr = YELLOW;
                    std::cout << rvcClr << BOLD << headingArrow(rvc->heading) << " " << RESET;
                } else if (isStart && !sim) {
                    std::cout << GREEN BOLD "@ " RESET;
                } else {
                    switch (grid[r][c]) {
                        case Cell::WALL:    std::cout << RED BOLD "# " RESET;     break;
                        case Cell::DUST:    std::cout << YELLOW "· " RESET;       break;
                        case Cell::CLEANED: std::cout << GRAY "░ " RESET;         break;
                        case Cell::EMPTY:
                            if (isCursor)  std::cout << BGGREEN " " RESET " ";
                            else           std::cout << WHITE "· " RESET;
                            break;
                    }
                }
            }
            std::cout << GRAY "│" RESET "\n";
        }

        // 테두리 하단
        std::cout << "  " GRAY "└";
        for (int c = 0; c < COLS; ++c) std::cout << "──";
        std::cout << "┘" RESET "\n";
    }

    // ─── 편집 도움말 ──────────────────────────────────────
    void printEditHelp() {
        std::cout << "\n";
        std::cout << "  " BOLD "범례" RESET
                  << "  " RED BOLD "#" RESET " 장애물"
                  << "  " YELLOW "·" RESET " 먼지"
                  << "  " GREEN BOLD "@" RESET " RVC 시작"
                  << "  " BGGREEN " " RESET " 커서"
                  << "\n";
        std::cout << "\n";
        std::cout << "  " CYAN "화살표" RESET " 커서이동  "
                  << CYAN "Space" RESET " 장애물토글  "
                  << CYAN "D" RESET " 먼지토글  "
                  << CYAN "R" RESET " RVC위치설정\n";
        std::cout << "  " CYAN "S/Enter" RESET " 시뮬시작  "
                  << CYAN "C" RESET " 초기화  "
                  << CYAN "Q" RESET " 종료\n";
        std::cout << "\n";
        std::cout << "  커서: (" << curRow_ << ", " << curCol_ << ")"
                  << "  RVC 시작: (" << startRVC_.row << ", " << startRVC_.col << ")\n";
    }

    // ─── 시뮬 상태 패널 ───────────────────────────────────
    void printSimStatus(const Simulation& sim, const char* state, const char* level) {
        std::cout << "\n";
        std::cout << "  " BOLD "┌─ 상태 ──────────────────────────────────────────┐" RESET "\n";
        std::cout << "  │  상태: " BOLD << state << RESET
                  << "   방향: " CYAN << headingName(sim.rvc.heading) << RESET
                  << "   위치: (" << sim.rvc.row << "," << sim.rvc.col << ")"
                  << "   청소출력: " MAGENTA << level << RESET "\n";

        // 청소 진행률
        int total = 0, cleanedCount = 0;
        for (auto& row : sim.grid)
            for (auto& c : row) {
                if (c != Cell::WALL) ++total;
                if (c == Cell::CLEANED) ++cleanedCount;
            }
        int pct = total > 0 ? (cleanedCount * 100 / total) : 0;
        std::cout << "  │  청소: " GREEN << cleanedCount << RESET "/" << total
                  << " (" << pct << "%)\n";
        std::cout << "  " BOLD "└─────────────────────────────────────────────────┘" RESET "\n";
    }

    // ─── 시뮬 도움말 ──────────────────────────────────────
    void printSimHelp() {
        std::cout << "  " CYAN "P" RESET " 일시정지/재개  "
                  << CYAN "E" RESET " 편집모드  "
                  << CYAN "Q" RESET " 종료\n";
    }

    // ─── 이벤트 로그 ──────────────────────────────────────
    void printLog() {
        std::cout << "\n  " GRAY "── 이벤트 로그 ──" RESET "\n";
        for (auto& line : log_.lines) {
            std::cout << line << "\n";
        }
    }
};

// ─────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────
int main() {
    InteractiveSimulator app;
    app.run();
    return 0;
}
