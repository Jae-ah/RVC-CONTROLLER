# RVC SW Controller 프로젝트

## 프로젝트 개요
로봇 청소기(RVC) SW Controller를 OOAD 프로세스로 개발한다.

## 요구사항
- RVC는 가정의 표면을 자동으로 청소하고 닦는다
- 청소하면서 직진한다
- 센서가 장애물을 감지하면 청소를 멈추고, 좌/우로 방향을 틀어 청소하며 전진한다
- 앞/좌/우 모두 장애물이 있으면 후진 후 좌/우로 방향을 틀어 전진한다
- 먼지를 감지하면 잠시 청소 출력을 높인다
- HW 제어의 상세 설계 및 구현은 고려하지 않는다
- 자동 청소 기능에만 집중한다

## 폴더 구조
arch/requirements/     - 요구사항 문서
arch/usecases/         - Use Case 문서
arch/analysis/ssd/     - SSD 다이어그램
arch/analysis/domain/  - 도메인 모델
arch/design/SD/        - 시퀀스 다이어그램
arch/design/class/     - 클래스 다이어그램
arch/system-tests/     - 시스템 테스트
src/                   - 소스 코드
tests/                 - 테스트 코드
simulator/             - RVC 동작 시뮬레이터 (유일하게 객체지향으로 개발하지 않아도 됨)

## 진행 단계
1. 요구사항 분석: arch/system.md, arch/requirements/fr-nfr.md
2. OOA: arch/usecases/usecases.md → arch/usecases/UC-nnn.md → arch/analysis/ssd/ssd.md → arch/analysis/domain/domain.md
3. OOD: arch/design/SD/SD.md → arch/design/class/class-diagram.md
4. OOI: src/ → tests/ → arch/system-tests/system-tests.md -> simulator/

## 설계 방향
- 구동 모터, 청소 모터는 외부 액터로 취급한다
- 시스템 내부에는 모터를 제어하는 별도 컨트롤러 객체를 둔다 (예: MotorController, CleaningController)

## 규칙
- 모든 다이어그램은 Mermaid 형식으로 작성한다
- 모든 코드는 객체지향 원칙을 철저히 준수한다 (simulator 제외)
- simulator를 통하여 system-tests를 테스트한다
- 장애물 회피 시 좌/우 방향은 랜덤으로 선택한다
- usecases, ssd, domain-model, ssd, class-diagram 은 반드시 통일성을 가질 것
- 구현 언어: C++


