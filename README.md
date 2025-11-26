# External Generator - F-theory Topology Tool

F-theory 토폴로지에 External curve를 추가하여 새로운 토폴로지를 자동 생성하는 도구입니다.

## 🚀 빠른 시작

```bash
# 컴파일
make

# 실행
./external_generator -i gLg.txt -o output.txt -n 1 -v
```

**이게 전부입니다!** 자세한 내용은 [Quick Start Guide](QUICKSTART.md) 참조.

## 📚 문서

| 문서 | 설명 |
|------|------|
| [QUICKSTART.md](QUICKSTART.md) | 5분 안에 시작하기 |
| [USAGE_GUIDE_COMPLETE.md](USAGE_GUIDE_COMPLETE.md) | 완전한 사용 가이드 |
| [BUILD_INSTRUCTIONS_FINAL.md](BUILD_INSTRUCTIONS_FINAL.md) | 빌드 가이드 |
| [MAKEFILE_GUIDE.md](MAKEFILE_GUIDE.md) | Makefile 사용법 |

## 🎯 주요 기능

- ✅ **Enhanced Line-Compact Format** 지원
- ✅ **Port-based Attachment**: Left/Middle/Right 정밀 배치
- ✅ **SUGRA 조건 자동 검증**
- ✅ **대규모 조합 생성 및 필터링**
- ✅ **병렬 처리 준비** (향후 확장 가능)

## 📋 기본 사용법

### 입력 파일 형식

```
kinds | bparams | S=(u,v);... | I=(u,v);... | sp=... | ip=...
```

예제:
```
0,1,0 | 6,22,6 | S=(0,0);(0,1);(2,2);(2,3) | I= | sp=1,1,1,1 | ip=
```

### 출력 파일 형식

Enhanced format with External info:
```
0,1,0 | 6,22,6 | S=(0,0);(0,1);(2,2);(2,3) | I= | sp=1,1,1,1 | ip= | E=(1,0,1,0) | ep=0
```

## 🎨 사용 예제

### 기본 사용
```bash
./external_generator -i gLg.txt -o output.txt -n 1 -v
```

### 2개 External
```bash
./external_generator -i gLg.txt -o output.txt -n 2 -v
```

### 빠른 생성 (검증 없음)
```bash
./external_generator -i gLg.txt -o output.txt -n 1 --no-sugra -v
```

### Block에만 부착
```bash
./external_generator -i gLg.txt -o output.txt -n 1 --no-sides -v
```

## ⚙️ 옵션

```
  -i PATH       입력 파일 (필수)
  -o PATH       출력 파일 (필수)
  -n N          각 토폴로지당 최대 External 개수 (기본값: 3)
  -p N          최대 포트 인덱스 (기본값: 2)
  --no-blocks   Block 포트 비활성화
  --no-sides    SideLink 포트 비활성화
  --no-interior Interior 포트 비활성화
  --no-sugra    SUGRA 검증 비활성화
  -v            상세 출력
  -h            도움말
```

## 🔧 빌드 요구사항

- C++17 이상
- Eigen3 라이브러리
- g++ 또는 clang++

### 빌드

```bash
# 기본 빌드
make

# 디버그 빌드
make debug

# 클린 빌드
make distclean
make
```

자세한 내용은 [BUILD_INSTRUCTIONS_FINAL.md](BUILD_INSTRUCTIONS_FINAL.md) 참조.

## 📊 알고리즘 개요

```
1. 입력 파일 읽기 (Enhanced line-compact format)
   ↓
2. 각 토폴로지에 대해 가능한 Port Placement 생성
   ↓
3. N개 External의 모든 배치 조합 생성
   ↓
4. 각 조합에 대해 Topology 구성
   ↓
5. 구조 검증 + SUGRA 조건 검증
   ↓
6. 검증 통과한 것만 출력
```

### Port 시스템

```
-1: Left end   (첫 번째 곡선)
-2: Right end  (마지막 곡선)
 0: Default    (일반 포트)
 1: Middle     (중간 곡선)
 2+: 추가 포트
```

### SUGRA 조건

- Signature: (n-1, 1) - 정확히 하나의 +1 eigenvalue
- Determinant: ±1 (unimodular)

## 📈 성능

### 예상 처리 시간 (gLg.txt, 2,589개 입력)

| External 수 | 조합 수 | SUGRA 통과 | 시간 |
|-------------|---------|------------|------|
| N=1 | ~25,000 | ~20,000 | 2-5분 |
| N=2 | ~250,000 | ~50,000 | 20-30분 |
| N=3 | ~2,500,000 | ~100,000 | 3-5시간 |

### 최적화 팁

- `--no-sugra`: SUGRA 검증 생략 (10배 빠름)
- `--no-sides`: SideLink 제외 (조합 수 감소)
- `-p 1`: 포트 제한 (조합 수 감소)
- 작은 입력으로 테스트: `head -100 input.txt > test.txt`

## 🐛 문제 해결

### 결과가 없음
```bash
# SUGRA 조건을 끄고 시도
./external_generator -i input.txt -o output.txt -n 1 --no-sugra
```

### 너무 느림
```bash
# External 수 줄이기
./external_generator -i input.txt -o output.txt -n 1

# 위치 제한
./external_generator -i input.txt -o output.txt -n 2 --no-sides -p 1
```

### 컴파일 오류
```bash
make distclean
make
```

자세한 문제 해결은 [USAGE_GUIDE_COMPLETE.md](USAGE_GUIDE_COMPLETE.md) 참조.

## 📦 파일 구조

```
external_generator/
├── external_generator.cpp      # 메인 프로그램
├── Topology_enhanced.{h,cpp}   # Enhanced 토폴로지 시스템
├── TopologyDB_enhanced.{hpp,cpp} # Enhanced 데이터베이스
├── TopoLineCompact_enhanced.{hpp,cpp} # Enhanced 직렬화
├── Theory_enhanced.h           # TheoryGraph & AttachmentPoint
├── Tensor.{h,C}               # Intersection form & SUGRA
├── Makefile                   # 빌드 시스템
└── docs/
    ├── QUICKSTART.md          # 빠른 시작
    ├── USAGE_GUIDE_COMPLETE.md # 완전한 가이드
    ├── BUILD_INSTRUCTIONS_FINAL.md # 빌드 가이드
    └── MAKEFILE_GUIDE.md      # Makefile 가이드
```

## 🎓 워크플로우 예제

```bash
# 1. 작은 테스트
head -10 gLg.txt > test.txt
./external_generator -i test.txt -o test_out.txt -n 1 -v

# 2. 결과 확인
wc -l test_out.txt
head test_out.txt

# 3. 전체 실행 (1 External)
./external_generator -i gLg.txt -o gLg_1ext.txt -n 1 -v

# 4. 통계 확인
wc -l gLg_1ext.txt
grep -c "| E=" gLg_1ext.txt

# 5. 필요시 2 External
./external_generator -i gLg.txt -o gLg_2ext.txt -n 2 -v
```

## 🔍 출력 분석

```bash
# 총 생성된 토폴로지 수
wc -l output.txt

# External 포함된 것만 카운트
grep -c "| E=" output.txt

# 특정 포트에 부착된 것 찾기
grep "E=([0-9]*,0,1," output.txt | wc -l  # Block port 1

# 처음 10개 확인
head -10 output.txt
```

## 📖 이론적 배경

### External Curve란?

F-theory에서 External curve는:
- 비동적 (non-dynamical) 곡선
- Gauge symmetry에 기여하지 않음
- 토폴로지 구조를 풍부하게 함
- SCFT/LST 분류에 영향

### AttachmentPoint System

```cpp
AttachmentPoint(-1)  // Left end
AttachmentPoint(-2)  // Right end
AttachmentPoint(0)   // First curve (default)
AttachmentPoint(1)   // Second curve
AttachmentPoint(2)   // Third curve
```

### TheoryGraph

```cpp
TheoryGraph G;
NodeRef node = G.add(n(-2));           // Node with param -2
NodeRef side = G.add(s(22));           // SideLink with param 22
NodeRef ext = G.add(e(0));             // External with param 0

G.connect(ext, AttachmentPoint(-1),    // External at default
         node, AttachmentPoint(1));    // Node at port 1

Eigen::MatrixXi IF = G.ComposeIF_Gluing();
```

## 🤝 기여

버그 리포트 및 개선 제안은 환영합니다!

## 📄 라이선스

이 프로젝트는 연구 목적으로 제공됩니다.

## 🙏 감사의 말

- Eigen library
- F-theory community
- String theory research group

---

**처음 시작하는 분들은 [QUICKSTART.md](QUICKSTART.md)를 먼저 읽어주세요!**

**문제가 있으신가요?** [USAGE_GUIDE_COMPLETE.md](USAGE_GUIDE_COMPLETE.md)의 문제 해결 섹션을 확인해주세요.
