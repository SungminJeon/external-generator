# External Generator 완벽 가이드 📚

## 🎯 프로그램 개요

`external_generator`는 기존 F-theory 토폴로지에 **External curve**를 추가하여 새로운 토폴로지를 생성하는 도구입니다.

### 핵심 기능
- 기존 토폴로지의 다양한 위치에 External curve 부착
- 포트 기반 정밀한 attachment point 지정 (left/middle/right)
- SUGRA 조건 자동 검증
- 대규모 조합 생성 및 필터링

## 📋 입력 파일 형식

### Enhanced Line-Compact Format

입력 파일은 한 줄에 하나의 토폴로지를 표현합니다:

```
kinds | bparams | S=(u,v);... | I=(u,v);... | sp=... | ip=...
```

### 예제 분석

```
0,1,0 | 6,22,6 | S=(0,0);(0,1);(2,2);(2,3) | I= | sp=1,1,1,1 | ip=
```

**해석:**
- `kinds`: `0,1,0` = g-L-g (node-interior-node)
- `bparams`: `6,22,6` = 각 블록의 파라미터 (self-intersection 등)
- `S=(0,0);(0,1);(2,2);(2,3)` = SideLink 연결
  - `(0,0)`: Block#0에 SideLink#0 연결
  - `(0,1)`: Block#0에 SideLink#1 연결
  - `(2,2)`: Block#2에 SideLink#2 연결
  - `(2,3)`: Block#2에 SideLink#3 연결
- `I=` = Instanton 없음
- `sp=1,1,1,1` = 4개 SideLink의 파라미터
- `ip=` = Instanton 파라미터 없음

### 시각화

```
        S0(1)
         |
    g(6)---L(22)---g(6)
    |               |
   S1(1)          S2(1)
                    |
                  S3(1)
```

## 🚀 기본 사용법

### 1. 가장 간단한 사용

```bash
./external_generator -i input.txt -o output.txt -n 1
```

- `-i input.txt`: 입력 파일 (gLg.txt 같은 파일)
- `-o output.txt`: 출력 파일
- `-n 1`: 각 토폴로지당 최대 1개 External 추가

### 2. Verbose 모드로 진행 상황 확인

```bash
./external_generator -i gLg.txt -o gLg_with_ext.txt -n 1 -v
```

출력 예시:
```
=== External Curve Generator (Enhanced) ===
Input:  gLg.txt
Output: gLg_with_ext.txt
Max externals per topology: 1
Max port index: 2
...

Processed 100 base topologies...
Generated: base_0_ext1_0 - E0@Block[0]@left/default
Generated: base_0_ext1_1 - E0@Block[0]@middle
Generated: base_0_ext1_2 - E0@Block[2]@left/default
...
```

### 3. 더 많은 External 추가

```bash
# 각 토폴로지당 최대 2개 External
./external_generator -i gLg.txt -o gLg_2ext.txt -n 2 -v

# 각 토폴로지당 최대 3개 External
./external_generator -i gLg.txt -o gLg_3ext.txt -n 3 -v
```

## 🎨 고급 옵션

### 포트 인덱스 제한

```bash
# 포트 0, 1만 사용 (left, middle만)
./external_generator -i gLg.txt -o output.txt -n 2 -p 1

# 포트 0, 1, 2, 3 사용 (더 많은 위치)
./external_generator -i gLg.txt -o output.txt -n 2 -p 3
```

**포트 인덱스 의미:**
- `0`: Left/default port (첫 번째 곡선)
- `1`: Middle port (두 번째 곡선)
- `2`: Right port (마지막 곡선 또는 세 번째 곡선)
- `3+`: 추가 포트 (복잡한 구조용)

### Attachment 위치 제어

```bash
# Block에만 External 부착 (SideLink 제외)
./external_generator -i gLg.txt -o output.txt -n 2 --no-sides

# SideLink에만 External 부착 (Block 제외)
./external_generator -i gLg.txt -o output.txt -n 2 --no-blocks

# Interior link 중간에 부착 비활성화
./external_generator -i gLg.txt -o output.txt -n 2 --no-interior
```

### SUGRA 검증 제어

```bash
# SUGRA 조건 검사 없이 모든 조합 생성 (빠름, 많은 결과)
./external_generator -i gLg.txt -o output.txt -n 2 --no-sugra

# SUGRA 검사 활성화 (기본값, 느림, 물리적으로 타당한 결과만)
./external_generator -i gLg.txt -o output.txt -n 2
```

## 📊 알고리즘 상세 설명

### Step 1: 입력 파일 읽기
```
gLg.txt 파일에서 한 줄씩 읽기
  └─> Enhanced format 파싱
       ├─> kinds, bparams 추출
       ├─> S/I connections 복원
       └─> Topology_enhanced 객체 생성
```

### Step 2: Port Placement 생성
각 토폴로지에 대해 가능한 모든 attachment point 찾기:

```cpp
For each block in topology:
    If block is interior link (L):
        - Add port 0 (left)
        - Add port 1 (middle)  // --no-interior로 비활성화 가능
        - Add port 2 (right)
    If block is node (g):
        - Add port 0 (default)
        
For each sidelink:
    - Add port 0 (default)  // --no-sides로 비활성화 가능
```

**예제: g-L-g 토폴로지**
```
가능한 Attachment Points:
- Block[0] (g):  port 0
- Block[1] (L):  port 0, 1, 2  (left, middle, right)
- Block[2] (g):  port 0
- SideLink[0]:   port 0
- SideLink[1]:   port 0
- SideLink[2]:   port 0
- SideLink[3]:   port 0

총 10개의 가능한 위치
```

### Step 3: 조합 생성

N개 External을 배치하는 모든 조합 생성:

**N=1인 경우:**
```
10개 위치 중 1개 선택 = 10가지 조합

예:
1. E0@Block[0]@port0
2. E0@Block[1]@port0
3. E0@Block[1]@port1
4. E0@Block[1]@port2
...
```

**N=2인 경우:**
```
10개 위치에서 중복 허용하여 2개 선택 = 10^2 = 100가지 조합

예:
1. E0@Block[0]@port0, E1@Block[0]@port0  (같은 위치 중복)
2. E0@Block[0]@port0, E1@Block[1]@port0
3. E0@Block[0]@port0, E1@Block[1]@port1
...
```

### Step 4: Topology 구성

각 조합에 대해:
```cpp
1. Base topology 복사
2. N개의 External curve 추가
3. 각 External을 지정된 포트에 연결
   - parent_id, parent_type, port_idx 설정
4. ExternalStructure에 연결 정보 저장
```

**생성된 Topology 예제:**
```
Base: g(6)---L(22)---g(6)
       |               |
      S(1)            S(1)

After: g(6)---L(22)---g(6)
       |      |        |
      S(1)   E(0)     S(1)
      
E-connection: (parent_id=1, parent_type=0, port_idx=1, external_id=0)
```

### Step 5: 검증

각 생성된 토폴로지에 대해:

#### 5.1 구조 검증
```cpp
- 모든 연결이 유효한 객체 참조하는지 확인
- parent_id가 범위 내에 있는지 확인
- external_id가 유효한지 확인
```

#### 5.2 SUGRA 조건 검증 (--no-sugra 없을 때)

```cpp
1. TheoryGraph 구성
   - 모든 Block, SideLink, Instanton, External을 노드로 추가
   - Spec 헬퍼 사용: n(), s(), i(), e()

2. 연결 추가
   - Interior links: connect(block_u, block_v)
   - SideLinks: connect(block_u, sidelink_v)
   - Instantons: connect(block_u, instanton_v)
   - Externals: connect(external, parent, AttachmentPoint(port_idx))

3. Intersection Form 계산
   - G.ComposeIF_Gluing() 호출
   - 모든 곡선들의 교차 행렬 생성

4. SUGRA 조건 체크
   - Tensor.SetIF(intersection_form)
   - Tensor.IsSUGRA() 확인
   - 조건: Signature (n-1, 1), Determinant = ±1
```

### Step 6: 출력

검증 통과한 토폴로지만 출력:

```
kinds | bparams | S=... | I=... | sp=... | ip=... | E=(parent,type,port,eid);... | ep=...
```

## 📈 성능 고려사항

### 조합 폭발

- **N=1**: 위치 수만큼 (예: 10개)
- **N=2**: 위치^2 (예: 100개)
- **N=3**: 위치^3 (예: 1,000개)

**입력 2,589개 토폴로지 × N=2 external:**
```
예상 출력: 2,589 × 100 = ~260,000개 토폴로지
SUGRA 필터 후: 약 10-20% 통과 = ~26,000-52,000개
```

### 속도 최적화 옵션

```bash
# 가장 빠름 (검증 없음, 주의!)
./external_generator -i gLg.txt -o output.txt -n 1 --no-sugra

# 빠름 (attachment 위치 제한)
./external_generator -i gLg.txt -o output.txt -n 1 --no-sides --no-interior

# 보통 (기본값)
./external_generator -i gLg.txt -o output.txt -n 1

# 느림 (많은 External)
./external_generator -i gLg.txt -o output.txt -n 3
```

## 🔍 실전 예제

### 예제 1: 작은 테스트

```bash
# 처음 10줄만 추출하여 테스트
head -10 gLg.txt > test_small.txt

# 1개 External, verbose
./external_generator -i test_small.txt -o test_output.txt -n 1 -v

# 결과 확인
wc -l test_output.txt
head test_output.txt
```

**예상 출력:**
```
=== Generation Statistics ===
Base topologies:     10
Attempted:           100  (10 base × ~10 positions)
Successful:          80
Failed construction: 0
Failed validation:   0
Failed SUGRA:        20
Success rate:        80%
```

### 예제 2: 전체 파일, 1개 External

```bash
./external_generator -i gLg.txt -o gLg_1ext.txt -n 1 -v
```

**처리 시간:** 약 2-5분 (SUGRA 검증 포함)

### 예제 3: 2개 External, 제한된 위치

```bash
# Block에만 부착, 포트 0,1만 사용
./external_generator \
    -i gLg.txt \
    -o gLg_2ext_blocks.txt \
    -n 2 \
    -p 1 \
    --no-sides \
    --no-interior \
    -v
```

**효과:**
- 조합 수 감소 → 속도 향상
- 특정 구조에 집중 가능

### 예제 4: 빠른 생성 (검증 없음)

```bash
# 모든 조합 생성, SUGRA 체크 생략
./external_generator -i gLg.txt -o gLg_all.txt -n 2 --no-sugra -v
```

⚠️ **주의:** SUGRA 조건 없이 생성된 토폴로지는 물리적으로 타당하지 않을 수 있습니다.

## 📊 출력 파일 분석

### 출력 형식

Enhanced format with External info:

```
0,1,0 | 6,22,6 | S=(0,0);(0,1);(2,2);(2,3) | I= | sp=1,1,1,1 | ip= | E=(1,0,1,0) | ep=0
```

**새로 추가된 부분:**
- `E=(1,0,1,0)`: External connection
  - `parent_id=1`: Block#1에 연결
  - `parent_type=0`: 0=Block, 1=SideLink, 2=Instanton
  - `port_idx=1`: 포트 1 (middle)
  - `external_id=0`: External#0
- `ep=0`: External parameter = 0

### 통계 확인

```bash
# 총 생성된 토폴로지 수
wc -l gLg_1ext.txt

# External 있는 것만 카운트
grep -c "| E=" gLg_1ext.txt

# 특정 포트에 부착된 것 찾기
grep "E=([0-9]*,0,1," gLg_1ext.txt | wc -l  # Block에 port 1로 부착
```

## 🐛 문제 해결

### 문제 1: 아무 결과도 나오지 않음

**원인:** SUGRA 조건이 너무 엄격
**해결:**
```bash
# SUGRA 체크 없이 실행
./external_generator -i input.txt -o output.txt -n 1 --no-sugra
```

### 문제 2: 너무 느림

**원인:** 조합 수가 너무 많음
**해결:**
```bash
# 옵션 1: External 수 줄이기
./external_generator -i input.txt -o output.txt -n 1

# 옵션 2: 위치 제한
./external_generator -i input.txt -o output.txt -n 2 --no-sides -p 1

# 옵션 3: 작은 입력으로 테스트
head -100 input.txt > small.txt
./external_generator -i small.txt -o output.txt -n 2
```

### 문제 3: 메모리 부족

**원인:** 너무 많은 조합 생성
**해결:**
```bash
# 배치 처리
split -l 500 gLg.txt gLg_part_
for f in gLg_part_*; do
    ./external_generator -i $f -o ${f}_out.txt -n 1
done
cat gLg_part_*_out.txt > final_output.txt
```

## 📚 추가 정보

### 생성된 Topology 시각화 예제

**입력:**
```
g(6)---L(22)---g(6)
 |               |
S(1)            S(1)
```

**출력 (1개 External, Block[1] port 1에 부착):**
```
g(6)---L(22)---g(6)
 |      |        |
S(1)   E(0)     S(1)
```

**Line-compact 표현:**
```
0,1,0 | 6,22,6 | S=(0,0);(2,1) | I= | sp=1,1 | ip= | E=(1,0,1,0) | ep=0
```

### 이름 규칙

생성된 토폴로지 이름:
```
{base_name}_ext{N}_{count}

예:
base_topology_0_ext1_0
base_topology_0_ext1_1
base_topology_0_ext2_0
```

## 🎓 완전한 워크플로우

```bash
# 1. 입력 확인
head gLg.txt
wc -l gLg.txt

# 2. 작은 테스트
head -10 gLg.txt > test.txt
./external_generator -i test.txt -o test_out.txt -n 1 -v

# 3. 전체 실행 (1 External)
./external_generator -i gLg.txt -o gLg_1ext.txt -n 1 -v

# 4. 결과 확인
wc -l gLg_1ext.txt
head gLg_1ext.txt

# 5. 통계 분석
grep -c "| E=" gLg_1ext.txt

# 6. 필요시 2 External 생성
./external_generator -i gLg.txt -o gLg_2ext.txt -n 2 -v
```

성공적인 생성을 기원합니다! 🚀
