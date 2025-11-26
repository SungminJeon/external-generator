#pragma once
#include <string>
#include <vector>
#include <utility>
#include <cstdint>
#include <ostream>

// 블록 종류: g, L, S, I
enum class LKind : uint8_t { g, L, S, I };

// 블록: 종류 + 정수 파라미터(예: self-intersection 등)
struct Block {
    LKind kind;
    int   param; // e.g., -2, -1, 0, ...
  //  int anomaly;
};

// Decoration
struct SideLinks { 
	int param;
};

struct Instantons {
	int param;
};

// link structure of nodes and interior links
struct InteriorStructure {
    int u; // node id
    int v; // node id
};

// link structure of nodes and side links
struct SideLinkStructure {
    int u; // node id
    int v; // sidelink id (장식 노드 혹은 보조 노드)
};

// link structure of instantons and nodes
struct InstantonStructure {
	int u; // node id
	int v; //instanton id
};

// 토폴로지 데이터 세트
struct Topology {
    std::string name; // SgS, SgLgS.. like..

    std::vector<Block>     block;       // Set of Blocks(nodes, interior links
    std::vector<SideLinks>      side_links;  // Set of side links
    std::vector<Instantons>     instantons;  // Set of instantons

    // Linking structure
    std::vector<InteriorStructure> l_connection; // connected structures between nodes and interior links (e.g. (0,1) = g at 0 and L at 1 are connected)
    std::vector<SideLinkStructure> s_connection; // connected structures between nodes and sidelinks (e.g. (0,0), (0,1) = side link at 0 and 1 are connected to 0 th block(generally node g)
    std::vector<InstantonStructure> i_connection;  // connected structures between nodes and instantons (e.g. (0,0) = instanton at 0 is connected to 0 th block(generally node g)

    // === 유틸 함수들 (구현은 .cpp) ===
    int  addBlock(LKind kind, int param);
    int  addBlockRight(LKind kind, int param);
    int  addDecoration(LKind kind, int param, int blockidx);
    int  LinkingBlocks(std::vector<InteriorStructure> lcon);
    int LinkingSideLinks(std::vector<SideLinkStructure> scon);
    int LinkingInstantonStructure(std::vector<InstantonStructure> icon);

    void Initialize();
    void InitializeBlocks();
    void InitializeDecorations();
    void InitializeGluings();
    void GetBlock(int j);
    


    bool hasNode(int id) const;

    // 판정 스텁(Theory/Tensor 쪽과 연동해 실제 체크를 넣을 자리)
    bool isSCFT() const;
    bool isLST()  const;
    bool isSUGRA() const;

    // 파일 저장: 간단 텍스트 덤프
    bool saveToFile(const std::string& path) const;
};

// 출력 연산자(디버그)
std::ostream& operator<<(std::ostream& os, const Topology& T);

