# Quickstart: 重力カスケードオセロ — build, test, run

## Prerequisites
- Windows 10/11, Visual Studio 2022 (MSVC), CMake ≥ 3.15
- DxLib 3.25a VC++ package extracted to `third_party/dxlib/`:
  ```bash
  curl -L -o third_party/dxlib/DxLib_VC3_25a.zip https://dxlib.xsrv.jp/DxLib/DxLib_VC3_25a.zip
  # extract; keep DxLib.h, DxLib.lib, DxLib.dll in third_party/dxlib/
  ```

## Build (verified on this machine)
```bash
cmake -B build-ninja -G Ninja
cmake --build build-ninja
```
Expected: zero errors AND zero warnings (AGENT.md gate). If Japanese glyphs garble, switch literals to Shift-JIS per research R3. Note: CMake 4.x requires `enable_testing()` before `add_test` — already in CMakeLists.txt.

## Run tests
```bash
ctest --test-dir build-ninja --output-on-failure
```
Expected suites: board/gravity/flip, chain/anchor, cascade fixed-point, all 10 card effects incl. edge cases, AI legality + timing, seeded e2e self-play (Hard vs Easy completes with valid result < 60 s).

## Play
```bash
build\Release\gravitycascade.exe
```
Expected flow: title → mode/difficulty → board with initial stacks settled in columns 3/4 → first move unrestricted, then chain-constrained. Verify acceptance scenarios 1–5 of User Story 1 and the card scenarios of User Story 2.
