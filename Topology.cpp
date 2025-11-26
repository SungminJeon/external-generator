#include "Topology.h"
#include <fstream>
#include <iomanip>   // std::quoted
#include <sstream>
#include <stdexcept>

// ========== 기본 유틸 ==========

int Topology::addBlock(LKind kind, int param) {
    block.push_back(Block{kind, param});
    return static_cast<int>(block.size()) - 1;
}

// 새 블록을 "오른쪽"에 붙인 뒤, (last, new) 내부 링크 추가
int Topology::addBlockRight(LKind kind, int param) {
    const int newId = addBlock(kind, param);
    if (newId > 0) {
        l_connection.push_back(InteriorStructure{newId - 1, newId});
    }
    return newId;
}


// === Topology.cpp — patch ===
// ... 기존 include/코드 유지 ...

int Topology::addDecoration(LKind kind, int param, int blockidx) {
    switch (kind) {
        case LKind::S: {
            side_links.push_back(SideLinks{param});
            // 방금 추가한 sidelink의 인덱스는 size()-1
            const int sid = static_cast<int>(side_links.size()) - 1;
            s_connection.push_back({blockidx, sid});
            return sid;
        }
        case LKind::I: {
            instantons.push_back(Instantons{param});
            const int iid = static_cast<int>(instantons.size()) - 1;
            i_connection.push_back({blockidx, iid});
            return iid;
        }
        default:
            return -1;
    }
}

// 헤더는 void GetBlock(int j) 이라서, 구현도 void로 맞춤 (공개필드 직접 쓰는걸 권장)
void Topology::GetBlock(int /*j*/) {
    // 헤더 시그니처를 바꿀 수 없으니 빈 구현으로 둠.
    // 실제로는 public 멤버 'block[j]'를 직접 사용하세요.
}

// 연결 정보 일괄 설정(기존 것을 덮어씀)
int Topology::LinkingBlocks(std::vector<InteriorStructure> lcon) {
    l_connection = std::move(lcon);
    return static_cast<int>(l_connection.size());
}

int Topology::LinkingSideLinks(std::vector<SideLinkStructure> scon) {
    s_connection = std::move(scon);
    return static_cast<int>(s_connection.size());
}

int Topology::LinkingInstantonStructure(std::vector<InstantonStructure> icon) {
    i_connection = std::move(icon);
    return static_cast<int>(i_connection.size());
}

// ========== 초기화 루틴 ==========

void Topology::Initialize() {
    name.clear();
    block.clear();
    side_links.clear();
    instantons.clear();
    l_connection.clear();
    s_connection.clear();
    i_connection.clear();
}

void Topology::InitializeBlocks() {
    block.clear();
}

void Topology::InitializeDecorations() {
    side_links.clear();
    instantons.clear();
}

void Topology::InitializeGluings() {
    l_connection.clear();
    s_connection.clear();
    i_connection.clear();
}

// ========== 질의 ==========

bool Topology::hasNode(int id) const {
    return (id >= 0 && id < static_cast<int>(block.size()));
}

// ========== 판정 스텁들 ==========
bool Topology::isSCFT() const { return false; }
bool Topology::isLST()  const { return false; }
bool Topology::isSUGRA() const { return false; }

// ========== 저장 ==========

static const char* to_cstr(LKind k) {
    switch (k) {
        case LKind::g: return "g";
        case LKind::L: return "L";
        case LKind::S: return "S";
        case LKind::I: return "I";
    }
    return "?";
}

bool Topology::saveToFile(const std::string& path) const {
    std::ofstream out(path);
    if (!out) return false;

    out << "name " << std::quoted(name) << "\n";

    // Blocks
    out << "blocks " << block.size() << "\n";
    for (size_t i = 0; i < block.size(); ++i) {
        out << "  [" << i << "] kind " << to_cstr(block[i].kind)
            << " param " << block[i].param << "\n";
    }

    // Side links
    out << "side_links " << side_links.size() << "\n";
    for (size_t i = 0; i < side_links.size(); ++i) {
        out << "  [" << i << "] param " << side_links[i].param << "\n";
    }

    // Instantons
    out << "instantons " << instantons.size() << "\n";
    for (size_t i = 0; i < instantons.size(); ++i) {
        out << "  [" << i << "] param " << instantons[i].param << "\n";
    }

    // Connections
    out << "l_connection " << l_connection.size() << "\n";
    for (const auto& e : l_connection) {
        out << "  (" << e.u << ", " << e.v << ")\n";
    }

    out << "s_connection " << s_connection.size() << "\n";
    for (const auto& e : s_connection) {
        out << "  (node " << e.u << ", sid " << e.v << ")\n";
    }

    out << "i_connection " << i_connection.size() << "\n";
    for (const auto& e : i_connection) {
        out << "  (node " << e.u << ", iid " << e.v << ")\n";
    }

    return true;
}

// ========== 디버그 출력 연산자 ==========

std::ostream& operator<<(std::ostream& os, const Topology& T) {
    os << "Topology " << std::quoted(T.name) << "\n";

    os << "  Blocks (" << T.block.size() << ")\n";
    for (size_t i = 0; i < T.block.size(); ++i) {
        const auto& b = T.block[i];
        os << "    [" << i << "] kind=" << to_cstr(b.kind)
           << " param=" << b.param << "\n";
    }

    os << "  SideLinks (" << T.side_links.size() << ")\n";
    for (size_t i = 0; i < T.side_links.size(); ++i) {
        os << "    [" << i << "] param=" << T.side_links[i].param << "\n";
    }

    os << "  Instantons (" << T.instantons.size() << ")\n";
    for (size_t i = 0; i < T.instantons.size(); ++i) {
        os << "    [" << i << "] param=" << T.instantons[i].param << "\n";
    }

    os << "  L-Connections (" << T.l_connection.size() << ")\n";
    for (const auto& e : T.l_connection) {
        os << "    " << e.u << " - " << e.v << "\n";
    }

    os << "  S-Connections (" << T.s_connection.size() << ")\n";
    for (const auto& e : T.s_connection) {
        os << "    node " << e.u << " ~ sid " << e.v << "\n";
    }

    os << "  I-Connections (" << T.i_connection.size() << ")\n";
    for (const auto& e : T.i_connection) {
        os << "    node " << e.u << " ~ iid " << e.v << "\n";
    }

    return os;
}

