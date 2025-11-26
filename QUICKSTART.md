# 🚀 Quick Start Guide - External Generator

## ⚡ 5분 안에 시작하기

### 1️⃣ 가장 간단한 사용법

```bash
./external_generator -i gLg.txt -o output.txt -n 1 -v
```

이게 전부입니다! 

**결과:**
- gLg.txt의 2,589개 토폴로지 읽기
- 각각에 1개씩 External curve 추가
- SUGRA 조건 만족하는 것만 output.txt에 저장

### 2️⃣ 옵션 설명

```bash
-i gLg.txt      # 입력 파일 (필수)
-o output.txt   # 출력 파일 (필수)
-n 1            # 각 토폴로지당 최대 External 개수
-v              # 진행 상황 출력 (권장)
```

### 3️⃣ 실행 예제

```bash
# 터미널에서:
$ ./external_generator -i gLg.txt -o gLg_with_externals.txt -n 1 -v

# 출력:
=== External Curve Generator (Enhanced) ===
Input:  gLg.txt
Output: gLg_with_externals.txt
Max externals per topology: 1
...
Processed 100 base topologies...
Processed 200 base topologies...
...

=== Generation Statistics ===
Base topologies:     2589
Attempted:           25890
Successful:          20712
Failed SUGRA:        5178
Success rate:        80.0%

Done! Output written to: gLg_with_externals.txt
```

## 📊 결과 확인

```bash
# 생성된 파일 크기 확인
ls -lh gLg_with_externals.txt

# 줄 수 확인
wc -l gLg_with_externals.txt

# 처음 몇 줄 보기
head -5 gLg_with_externals.txt
```

## 🎨 자주 쓰는 명령어

### 작은 테스트 (처음 10개만)
```bash
head -10 gLg.txt > test.txt
./external_generator -i test.txt -o test_out.txt -n 1 -v
```

### 2개 External 추가
```bash
./external_generator -i gLg.txt -o gLg_2ext.txt -n 2 -v
```

### 빠른 생성 (검증 없음)
```bash
./external_generator -i gLg.txt -o gLg_fast.txt -n 1 --no-sugra -v
```

### Block에만 부착
```bash
./external_generator -i gLg.txt -o gLg_blocks.txt -n 1 --no-sides -v
```

## 🔍 입력 파일 이해하기

`gLg.txt`의 한 줄:
```
0,1,0 | 6,22,6 | S=(0,0);(0,1);(2,2);(2,3) | I= | sp=1,1,1,1 | ip=
```

**의미:**
- `0,1,0`: g-L-g 구조 (node-interior-node)
- `6,22,6`: 각 곡선의 파라미터
- `S=(0,0)...`: SideLink들이 어디에 붙어있는지
- `sp=1,1,1,1`: 4개 SideLink의 파라미터

**시각화:**
```
    S0
     |
g---L---g
|       |
S1     S2
        |
       S3
```

## 📈 출력 파일 이해하기

생성된 파일의 한 줄:
```
0,1,0 | 6,22,6 | S=(0,0);(0,1);(2,2);(2,3) | I= | sp=1,1,1,1 | ip= | E=(1,0,1,0) | ep=0
```

**새로 추가된 부분:**
- `E=(1,0,1,0)`: External 연결 정보
  - Block#1에 연결
  - 포트 1 (middle)
  - External#0
- `ep=0`: External 파라미터

**시각화:**
```
    S0
     |
g---L---g
|   |   |
S1  E  S2    ← External이 L의 중간에 추가됨!
        |
       S3
```

## ⚙️ 전체 옵션 목록

```bash
./external_generator -h
```

```
Options:
  -i PATH       Input database path (required)
  -o PATH       Output database path (required)
  -n N          Max externals per topology (default: 3)
  -p N          Max port index (default: 2)
  --no-blocks   Disable block port attachments
  --no-sides    Disable sidelink port attachments
  --no-interior Disable interior port attachments
  --no-sugra    Disable SUGRA checking
  -v            Verbose output
  -h            Show this help
```

## 💡 팁

### 처음 사용할 때
```bash
# 1. 작은 테스트로 시작
head -10 gLg.txt > test.txt
./external_generator -i test.txt -o test_out.txt -n 1 -v

# 2. 결과 확인
cat test_out.txt

# 3. 문제없으면 전체 실행
./external_generator -i gLg.txt -o gLg_1ext.txt -n 1 -v
```

### 속도가 느릴 때
```bash
# External 수 줄이기: -n 2 → -n 1
# 위치 제한: --no-sides --no-interior
# 포트 제한: -p 1 (대신 -p 2)
```

### 결과가 너무 적을 때
```bash
# SUGRA 체크 끄기 (주의: 물리적으로 유효하지 않을 수 있음)
./external_generator -i input.txt -o output.txt -n 1 --no-sugra
```

## 🎯 실전 워크플로우

```bash
# Step 1: 입력 확인
wc -l gLg.txt
# → 2589 gLg.txt

# Step 2: 작은 테스트
head -100 gLg.txt > test.txt
./external_generator -i test.txt -o test_out.txt -n 1 -v
# → 약 800-1000줄 생성

# Step 3: 전체 실행
./external_generator -i gLg.txt -o gLg_1ext.txt -n 1 -v
# → 약 20,000-25,000줄 생성 (2-5분 소요)

# Step 4: 결과 확인
wc -l gLg_1ext.txt
head gLg_1ext.txt

# Step 5: 필요시 2개 External
./external_generator -i gLg.txt -o gLg_2ext.txt -n 2 -v
# → 약 200,000-250,000줄 생성 (20-30분 소요)
```

## 🐛 문제 해결

### "No output generated"
```bash
# SUGRA 조건이 너무 엄격함
./external_generator -i input.txt -o output.txt -n 1 --no-sugra -v
```

### "Too slow"
```bash
# External 수 줄이기
./external_generator -i input.txt -o output.txt -n 1 -v

# 또는 위치 제한
./external_generator -i input.txt -o output.txt -n 1 --no-sides -v
```

### "Compilation errors"
```bash
# 클린 빌드
make distclean
make
```

## 📞 도움말

더 자세한 정보:
```bash
# 전체 가이드 보기
cat USAGE_GUIDE_COMPLETE.md

# 컴파일 문제
cat BUILD_INSTRUCTIONS_FINAL.md

# 알고리즘 상세
cat COMPILATION_FIX_SUMMARY.md
```

---

**시작하기 좋은 명령어:**
```bash
./external_generator -i gLg.txt -o output.txt -n 1 -v
```

이게 전부입니다! 🎉
