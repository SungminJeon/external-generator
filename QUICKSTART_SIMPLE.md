# 🚀 Quick Start - External Generator Simple

## ⚡ 3분 안에 시작하기

### 1️⃣ 컴파일

```bash
# 파일 복사
cp /mnt/user-data/outputs/external_generator_simple.cpp ./
cp /mnt/user-data/outputs/Makefile_simple ./Makefile
cp /mnt/user-data/outputs/TopologyDB_enhanced.cpp ./

# 원본 파일들
cp /mnt/user-data/uploads/{Topology_enhanced.*,TopoLineCompact_enhanced.*,Tensor.*,Theory_enhanced.h,TopologyDB_enhanced.hpp} ./

# 빌드
make
```

### 2️⃣ 실행

```bash
# 가장 간단한 사용 - 분류만
./external_generator_simple -i gLg.txt -o output_dir

# SideLink 0에 External 추가
./external_generator_simple -i gLg.txt -o output_dir -a s(0) -v
```

## 📋 핵심 개념

### s(0) 표기법

```
s(0)  - SideLink 0의 모든 포트에 External 부착
s(1)  - SideLink 1의 모든 포트에 External 부착
b(2)  - Block 2의 모든 포트에 External 부착
i(0)  - Instanton 0의 모든 포트에 External 부착
```

### 예제 토폴로지

```
입력: 0,1,0 | 6,22,6 | S=(0,0);(2,1) | I= | sp=1,1 | ip=

시각화:
    S0
     |
g---L---g
        |
       S1
```

**명령어:** `./external_generator_simple -i input.txt -o out -a s(0)`

**결과:** S0에 External 추가
```
    S0
     |
g---L---g
    |   |
    E  S1
```

## 🎯 자주 쓰는 명령어

### 분류만 (External 추가 없이)
```bash
./external_generator_simple -i gLg.txt -o classified
```

### SideLink에 External 추가
```bash
./external_generator_simple -i gLg.txt -o output -a s(0) -v
```

### 여러 위치에 추가
```bash
./external_generator_simple -i gLg.txt -o output -a s(0) -a s(1) -a b(2) -v
```

### 디렉토리 처리
```bash
./external_generator_simple -i input_dir -o output_dir -a s(0) -v
```

## 📊 출력 구조

```
output_dir/
├── LST/           ← LST 조건 만족
│   └── len-3/
│       └── gLg.txt
└── SCFT/          ← SCFT 조건 만족
    └── len-3/
        └── gLg.txt
```

**자동 분류:**
- **LST**: 1개 zero eigenvalue, 나머지 음수
- **SCFT**: 모든 eigenvalue 음수 (negative definite)

## 🔍 예제 실행

```bash
# 1. 작은 테스트
head -10 gLg.txt > test.txt
./external_generator_simple -i test.txt -o test_out -a s(0) -v

# 출력:
=== External Generator (Simple) ===
Input:  test.txt
Output: test_out
Attachment specs:
  - s(0)

Processed 10 topologies...

=== Statistics ===
Input topologies:     10
Output topologies:    8
  LST:                5
  SCFT:               3
  Neither:            2
  Errors:             0

Done! Output written to: test_out
```

## 📈 성능 비교

| 프로그램 | External 수 | 시간 (2,589 입력) |
|----------|-------------|-------------------|
| Simple (s(0)) | 1개/입력 | ~20초 |
| Simple (s(0),s(1)) | 2개/입력 | ~40초 |
| Full (n=1) | ~10개/입력 | 2-5분 |
| Full (n=2) | ~100개/입력 | 20-30분 |

**Simple 버전이 훨씬 빠름!**

## 💡 핵심 차이점

| 기능 | Simple | Full |
|------|--------|------|
| 조건 | LST/SCFT | SUGRA |
| External 수 | 1개 | 여러 개 |
| 지정 방식 | s(0) | 자동 조합 |
| 속도 | 매우 빠름 | 느림 |
| 디렉토리 입력 | ✅ | ❌ |

## 🎓 언제 어떤 버전을 쓸까?

### Simple 사용 시기
- ✅ LST/SCFT 분류만 필요
- ✅ 특정 위치에만 External 추가
- ✅ 빠른 결과 필요
- ✅ 디렉토리 처리 필요

### Full 사용 시기
- ✅ SUGRA 조건 필요
- ✅ 모든 가능한 조합 탐색
- ✅ 여러 External 동시 추가

## 📝 팁

### 1. 작은 테스트로 시작
```bash
head -10 input.txt > test.txt
./external_generator_simple -i test.txt -o test_out -a s(0) -v
```

### 2. 분류만 먼저
```bash
# 입력 유효성 확인
./external_generator_simple -i input.txt -o check --classify-only -v
```

### 3. 하나씩 추가
```bash
# s(0)만
./external_generator_simple -i input.txt -o out1 -a s(0)

# 결과 좋으면 s(1) 추가
./external_generator_simple -i input.txt -o out2 -a s(0) -a s(1)
```

## 🐛 문제 해결

### 아무 출력이 없음
```bash
# 분류만 시도
./external_generator_simple -i input.txt -o output --classify-only -v
```

### 너무 느림
```bash
# spec 줄이기
./external_generator_simple -i input.txt -o output -a s(0)  # 하나만
```

### Invalid spec 오류
```bash
# 올바른 형식
-a s(0)   ✅
-a s0     ❌
-a s[0]   ❌
```

## 🎯 완전한 예제

```bash
# 1. 컴파일
make

# 2. 테스트
head -10 gLg.txt > test.txt
./external_generator_simple -i test.txt -o test_out -a s(0) -v

# 3. 전체 실행
./external_generator_simple -i gLg.txt -o final_output -a s(0) -v

# 4. 결과 확인
find final_output -name "*.txt" -exec wc -l {} +

# 5. LST 개수
find final_output/LST -name "*.txt" -exec cat {} \; | wc -l

# 6. SCFT 개수
find final_output/SCFT -name "*.txt" -exec cat {} \; | wc -l
```

---

**더 자세한 정보는 [USAGE_SIMPLE.md](USAGE_SIMPLE.md)를 참조하세요!**
