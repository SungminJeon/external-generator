# External Generator Simple - 사용 가이드

## 🎯 개요

단순화된 External curve generator로 다음 기능을 제공합니다:

- ✅ **LST/SCFT 자동 분류**
- ✅ **디렉토리 입력 지원**
- ✅ **s(0) 표기법**으로 간단한 attachment 지정
- ✅ **1개 External만 추가** (단순하고 빠름)
- ✅ **자동 카테고리별 출력** (LST/, SCFT/ 디렉토리 생성)

## 🚀 빠른 시작

### 컴파일

```bash
cp /mnt/user-data/outputs/external_generator_simple.cpp ./
cp /mnt/user-data/outputs/Makefile_simple ./Makefile
cp /mnt/user-data/outputs/TopologyDB_enhanced.cpp ./

# 원본 파일들
cp /mnt/user-data/uploads/{Topology_enhanced.*,TopoLineCompact_enhanced.*,Tensor.*,Theory_enhanced.h,TopologyDB_enhanced.hpp} ./

make
```

### 기본 사용법

```bash
# 분류만 (External 추가 없이)
./external_generator_simple -i gLg.txt -o output_dir

# SideLink 0에 External 추가
./external_generator_simple -i gLg.txt -o output_dir -a s(0)

# 여러 위치에 추가
./external_generator_simple -i gLg.txt -o output_dir -a s(0) -a s(1) -a b(2)

# 디렉토리 입력
./external_generator_simple -i input_dir -o output_dir -a s(0) -v
```

## 📋 명령어 옵션

```
-i PATH         입력 파일 또는 디렉토리 (필수)
-o DIR          출력 디렉토리 (필수)
-a SPEC         Attachment 지정 (여러 번 사용 가능)
--classify-only 분류만 수행 (External 추가 안 함)
-v              상세 출력
-h              도움말
```

## 🎨 Attachment 표기법

### 기본 형식

```
s(N)  - SideLink N의 모든 가능한 포트에 External 부착
b(N)  - Block N의 모든 가능한 포트에 External 부착
i(N)  - Instanton N의 모든 가능한 포트에 External 부착
```

### 포트 생성 규칙

**SideLink s(0):**
- 포트 0 (기본) → 1개 위치

**Block b(N) - Node (g):**
- 포트 0 (기본) → 1개 위치

**Block b(N) - Interior Link (L):**
- 포트 0 (left)
- 포트 1 (middle)
- 포트 2 (right)
→ 3개 위치

**Instanton i(0):**
- 포트 0 (기본) → 1개 위치

### 예제

**입력 토폴로지:**
```
0,1,0 | 6,22,6 | S=(0,0);(2,1) | I= | sp=1,1 | ip=

시각화:
    S0
     |
g(6)---L(22)---g(6)
                |
               S1
```

**명령어:**
```bash
./external_generator_simple -i input.txt -o output -a s(0) -a b(1)
```

**결과:**
- `s(0)`: SideLink 0에 External → 1개 조합
- `b(1)`: Block 1 (L)의 3개 포트에 External → 3개 조합
- **총 4개 토폴로지 생성**

**생성된 토폴로지들:**
```
1. S0에 External 부착
    S0
     |
g---L---g
    |   |
    E  S1

2. L의 left에 External 부착
    S0  E
     |  |
g---L---g
        |
       S1

3. L의 middle에 External 부착
    S0
     |
g---L---g
    |   |
    E  S1

4. L의 right에 External 부착
    S0
     |
g---L---g
      | |
      E S1
```

## 📊 LST/SCFT 분류

### LST (Little String Theory)
- 정확히 **1개의 0 eigenvalue**
- 나머지는 모두 **음수**

### SCFT (Superconformal Field Theory)
- **모든 eigenvalue가 음수** (negative definite)

### 출력 구조

```
output_dir/
├── LST/
│   ├── len-3/
│   │   ├── gLg.txt
│   │   └── ggg.txt
│   └── len-4/
│       └── gLgL.txt
├── SCFT/
│   ├── len-3/
│   │   └── gLg.txt
│   └── len-4/
│       └── gLgL.txt
└── Neither/
    └── ...
```

- **LST/**: LST로 분류된 토폴로지
- **SCFT/**: SCFT로 분류된 토폴로지
- **Neither/**: 둘 다 아닌 경우 (보통 출력하지 않음)
- `len-N/`: 토폴로지 길이별 분류
- `prefix.txt`: 시작 패턴별 분류

## 🎯 사용 예제

### 예제 1: 분류만 수행

```bash
# External 추가 없이 기존 토폴로지만 분류
./external_generator_simple -i gLg.txt -o classified -v
```

**결과:**
```
output_dir/
├── LST/
│   └── len-3/
│       └── gLg.txt  (LST 조건 만족하는 것들)
└── SCFT/
    └── len-3/
        └── gLg.txt  (SCFT 조건 만족하는 것들)
```

### 예제 2: SideLink 0에 External 추가

```bash
./external_generator_simple -i gLg.txt -o output -a s(0) -v
```

**처리:**
1. 각 입력 토폴로지 읽기
2. SideLink 0의 가능한 포트 찾기 (보통 1개)
3. 각 포트에 External 추가
4. LST/SCFT 분류
5. 카테고리별로 저장

### 예제 3: 여러 위치에 External 추가

```bash
./external_generator_simple -i gLg.txt -o output -a s(0) -a s(1) -a b(2) -v
```

**처리:**
- 각 입력 토폴로지당:
  - s(0)의 모든 포트 시도
  - s(1)의 모든 포트 시도
  - b(2)의 모든 포트 시도
- LST 또는 SCFT인 것만 출력

### 예제 4: 디렉토리 처리

```bash
# input_dir 안의 모든 .txt 파일 처리
./external_generator_simple -i input_dir -o output_dir -a s(0) -v
```

**입력 구조:**
```
input_dir/
├── gLg.txt
├── ggg.txt
└── subdir/
    └── more.txt
```

**모든 파일이 재귀적으로 처리됩니다.**

## 📈 성능

### 예상 처리 속도 (gLg.txt, 2,589개)

| 설정 | 조합/입력 | 예상 출력 | 시간 |
|------|-----------|-----------|------|
| 분류만 | 1 | ~2,000 | 10초 |
| `-a s(0)` | ~1 | ~2,000 | 20초 |
| `-a s(0) -a s(1)` | ~2 | ~4,000 | 40초 |
| `-a s(0) -a s(1) -a b(1)` | ~5 | ~10,000 | 2분 |

**훨씬 빠른 이유:**
- 1개 External만 추가
- SUGRA 대신 LST/SCFT만 체크 (더 단순)
- 조합 폭발 없음

## 🔍 출력 형식

### Enhanced Format with External

```
0,1,0 | 6,22,6 | S=(0,0);(2,1) | I= | sp=1,1 | ip= | E=(0,1,0,0) | ep=0
```

**External 정보:**
- `E=(0,1,0,0)`: External 연결
  - `parent_id=0`: SideLink 0
  - `parent_type=1`: SideLink
  - `port_idx=0`: 포트 0
  - `external_id=0`: External 0
- `ep=0`: External parameter

## 🛠️ 고급 사용

### 특정 길이만 처리

```bash
# 입력에서 길이 3인 것만 추출
grep "^0,1,0" gLg.txt > len3.txt

# 처리
./external_generator_simple -i len3.txt -o output -a s(0)
```

### 배치 처리

```bash
# 여러 입력 디렉토리
for dir in input_*; do
    ./external_generator_simple -i $dir -o output_${dir} -a s(0) -v
done
```

### 결과 분석

```bash
# LST 개수
find output_dir/LST -name "*.txt" -exec wc -l {} + | tail -1

# SCFT 개수
find output_dir/SCFT -name "*.txt" -exec wc -l {} + | tail -1

# 특정 길이의 LST
cat output_dir/LST/len-3/*.txt | wc -l
```

## 📊 통계 출력

실행 후 자동으로 표시:

```
=== Statistics ===
Input topologies:     2589
Output topologies:    2134
  LST:                1245
  SCFT:               889
  Neither:            234
  Errors:             221
```

## 🐛 문제 해결

### "No output generated"

**원인:** 모든 조합이 LST/SCFT가 아님

**해결:**
```bash
# 분류만 시도 (External 없이)
./external_generator_simple -i input.txt -o output --classify-only -v
```

### "Too slow"

**원인:** 너무 많은 attachment spec

**해결:**
```bash
# Spec 줄이기
./external_generator_simple -i input.txt -o output -a s(0) -v
```

### "Invalid spec"

**원인:** 잘못된 표기법

**해결:**
```bash
# 올바른 형식
-a s(0)  # ✅
-a S(0)  # ✅ (대소문자 구분 안 함)
-a s0    # ❌
-a s[0]  # ❌
```

## 💡 팁

1. **작은 테스트로 시작**
   ```bash
   head -10 gLg.txt > test.txt
   ./external_generator_simple -i test.txt -o test_out -a s(0) -v
   ```

2. **분류만 먼저 실행**
   ```bash
   # 입력이 유효한지 확인
   ./external_generator_simple -i input.txt -o check --classify-only -v
   ```

3. **하나씩 추가**
   ```bash
   # s(0)만 먼저
   ./external_generator_simple -i input.txt -o out1 -a s(0)
   
   # 결과 확인 후 s(1) 추가
   ./external_generator_simple -i input.txt -o out2 -a s(0) -a s(1)
   ```

## 🎓 전체 워크플로우

```bash
# 1. 컴파일
make

# 2. 테스트
head -10 gLg.txt > test.txt
./external_generator_simple -i test.txt -o test_out -a s(0) -v

# 3. 분류 확인
./external_generator_simple -i gLg.txt -o classified --classify-only -v

# 4. External 추가
./external_generator_simple -i gLg.txt -o with_ext -a s(0) -v

# 5. 결과 확인
find with_ext -name "*.txt" -exec wc -l {} +

# 6. 추가 처리
./external_generator_simple -i gLg.txt -o with_ext2 -a s(0) -a s(1) -v
```

성공적인 생성을 기원합니다! 🚀
