# External Generator - 완전 가이드

두 가지 버전을 제공합니다:

## 📦 버전 비교

| 특징 | **Simple** (권장) | Full |
|------|-------------------|------|
| **목적** | LST/SCFT 분류 | SUGRA 조건 체크 |
| **External 수** | 1개 | 여러 개 (N개) |
| **지정 방식** | `s(0)` 표기법 | 자동 조합 생성 |
| **입력** | 파일 or 디렉토리 | 파일만 |
| **속도** | 매우 빠름 ⚡ | 느림 🐌 |
| **출력 구조** | LST/SCFT 분류 | 단일 파일 |
| **사용 난이도** | 쉬움 😊 | 보통 😐 |

---

## 🚀 Simple 버전 (권장)

### 📝 특징
- ✅ **LST/SCFT 자동 분류**
- ✅ **s(0) 표기법**으로 간단한 지정
- ✅ **디렉토리 입력** 지원
- ✅ **1개 External**만 추가 (빠르고 단순)
- ✅ **카테고리별 자동 출력**

### 🎯 사용법

```bash
# 컴파일
make -f Makefile_simple

# 분류만
./external_generator_simple -i gLg.txt -o output_dir

# SideLink 0에 External 추가
./external_generator_simple -i gLg.txt -o output_dir -a s(0) -v

# 여러 위치
./external_generator_simple -i gLg.txt -o output_dir -a s(0) -a s(1) -a b(2)

# 디렉토리 처리
./external_generator_simple -i input_dir -o output_dir -a s(0) -v
```

### 📊 출력 구조

```
output_dir/
├── LST/
│   └── len-3/
│       └── gLg.txt
└── SCFT/
    └── len-3/
        └── gLg.txt
```

### ⚡ 성능

| 입력 | 설정 | 예상 출력 | 시간 |
|------|------|-----------|------|
| 2,589 | 분류만 | ~2,000 | 10초 |
| 2,589 | `-a s(0)` | ~2,000 | 20초 |
| 2,589 | `-a s(0) -a s(1)` | ~4,000 | 40초 |

### 📚 문서
- [QUICKSTART_SIMPLE.md](QUICKSTART_SIMPLE.md) - 3분 시작 가이드
- [USAGE_SIMPLE.md](USAGE_SIMPLE.md) - 완전한 사용법

---

## 🔬 Full 버전 (고급)

### 📝 특징
- ✅ **SUGRA 조건** 체크 (signature, determinant)
- ✅ **모든 조합 자동 생성**
- ✅ **여러 External** 동시 추가
- ✅ **Port 시스템** (left/middle/right)

### 🎯 사용법

```bash
# 컴파일
make

# 1개 External
./external_generator -i gLg.txt -o output.txt -n 1 -v

# 2개 External
./external_generator -i gLg.txt -o output.txt -n 2 -v

# 옵션들
./external_generator -i gLg.txt -o output.txt -n 1 \
    --no-sides \      # SideLink 제외
    --no-interior \   # Interior 포트 제외
    -p 1              # 포트 0,1만 사용
```

### 📊 출력 구조

```
output.txt  (단일 파일, Enhanced format)
```

### ⚡ 성능

| 입력 | 설정 | 예상 출력 | 시간 |
|------|------|-----------|------|
| 2,589 | `-n 1` | ~20,000 | 2-5분 |
| 2,589 | `-n 2` | ~50,000 | 20-30분 |
| 2,589 | `-n 3` | ~100,000 | 3-5시간 |

### 📚 문서
- [QUICKSTART.md](QUICKSTART.md) - 5분 시작 가이드
- [USAGE_GUIDE_COMPLETE.md](USAGE_GUIDE_COMPLETE.md) - 완전한 가이드
- [README.md](README.md) - 전체 개요

---

## 🎯 어떤 버전을 써야 할까?

### Simple 사용 시기 ⭐ (대부분의 경우)

```bash
# ✅ 이런 경우 Simple 사용:

1. LST/SCFT 분류가 목적
   ./external_generator_simple -i input.txt -o output --classify-only

2. 특정 위치에만 External 추가
   ./external_generator_simple -i input.txt -o output -a s(0) -a s(1)

3. 빠른 결과 필요
   (Simple이 10-20배 빠름!)

4. 디렉토리 전체 처리
   ./external_generator_simple -i input_dir -o output -a s(0)

5. 간단하게 사용하고 싶을 때
   (s(0) 표기법이 훨씬 직관적)
```

### Full 사용 시기

```bash
# ✅ 이런 경우 Full 사용:

1. SUGRA 조건 체크가 필요
   ./external_generator -i input.txt -o output.txt -n 1

2. 모든 가능한 조합 탐색
   (자동으로 모든 위치 시도)

3. 여러 External 동시 추가 (N=2, 3, ...)
   ./external_generator -i input.txt -o output.txt -n 2

4. 연구용으로 완전한 탐색 필요
   (모든 포트의 모든 조합)
```

---

## 🎓 실전 워크플로우

### Workflow 1: 빠른 분류 (Simple)

```bash
# Step 1: 컴파일
make -f Makefile_simple

# Step 2: 작은 테스트
head -10 gLg.txt > test.txt
./external_generator_simple -i test.txt -o test_out -a s(0) -v

# Step 3: 전체 실행
./external_generator_simple -i gLg.txt -o final_output -a s(0) -v

# Step 4: 결과 확인
find final_output/LST -name "*.txt" -exec cat {} \; | wc -l
find final_output/SCFT -name "*.txt" -exec cat {} \; | wc -l
```

### Workflow 2: 완전한 탐색 (Full)

```bash
# Step 1: 컴파일
make

# Step 2: 작은 테스트
head -10 gLg.txt > test.txt
./external_generator -i test.txt -o test_out.txt -n 1 -v

# Step 3: 전체 실행 (1 External)
./external_generator -i gLg.txt -o gLg_1ext.txt -n 1 -v

# Step 4: 2 External (시간 오래 걸림)
./external_generator -i gLg.txt -o gLg_2ext.txt -n 2 -v
```

---

## 📋 입력 파일 형식 (공통)

### Enhanced Line-Compact Format

```
kinds | bparams | S=(u,v);... | I=(u,v);... | sp=... | ip=...
```

**예제:**
```
0,1,0 | 6,22,6 | S=(0,0);(2,1) | I= | sp=1,1 | ip=
```

**시각화:**
```
    S0
     |
g(6)---L(22)---g(6)
                |
               S1
```

---

## 🔍 출력 형식 (공통)

### External 추가된 형식

```
0,1,0 | 6,22,6 | S=(0,0);(2,1) | I= | sp=1,1 | ip= | E=(0,1,0,0) | ep=0
```

**External 정보:**
- `E=(0,1,0,0)`:
  - `parent_id=0`: SideLink 0
  - `parent_type=1`: 1=SideLink
  - `port_idx=0`: Port 0
  - `external_id=0`: External 0
- `ep=0`: External parameter

---

## 💡 추천 사항

### 초보자
```bash
# Simple 버전으로 시작!
./external_generator_simple -i gLg.txt -o output -a s(0) -v
```

### 숙련자
```bash
# 목적에 따라 선택
# - 빠른 분류 → Simple
# - 완전한 탐색 → Full
```

### 대규모 처리
```bash
# Simple로 디렉토리 처리
./external_generator_simple -i input_dir -o output_dir -a s(0) -v
```

---

## 📞 도움말

### Simple 버전
- [QUICKSTART_SIMPLE.md](QUICKSTART_SIMPLE.md) - 빠른 시작
- [USAGE_SIMPLE.md](USAGE_SIMPLE.md) - 상세 가이드

### Full 버전
- [QUICKSTART.md](QUICKSTART.md) - 빠른 시작
- [USAGE_GUIDE_COMPLETE.md](USAGE_GUIDE_COMPLETE.md) - 상세 가이드
- [README.md](README.md) - 전체 개요

### 공통
- [BUILD_INSTRUCTIONS_FINAL.md](BUILD_INSTRUCTIONS_FINAL.md) - 빌드 가이드
- [MAKEFILE_GUIDE.md](MAKEFILE_GUIDE.md) - Makefile 사용법

---

## 🎉 시작하기

**대부분의 경우 Simple 버전을 추천합니다!**

```bash
# 1. Simple 컴파일
make -f Makefile_simple

# 2. 실행
./external_generator_simple -i gLg.txt -o output_dir -a s(0) -v

# 3. 결과 확인
find output_dir -name "*.txt" -exec wc -l {} +
```

성공을 기원합니다! 🚀
