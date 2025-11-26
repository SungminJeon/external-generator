#include "TopoLineCompact.hpp"
#include <sstream>
#include <vector>
#include <cctype>
#include <stdexcept>

static inline int kindToInt(LKind k){
    switch (k){ case LKind::g: return 0; case LKind::L: return 1; case LKind::S: return 2; case LKind::I: return 3; }
    return 0;
}
static inline LKind intToKind(int x){
    switch (x){ case 0: return LKind::g; case 1: return LKind::L; case 2: return LKind::S; case 3: return LKind::I; }
    return LKind::g;
}
static inline void trim(std::string& s){
    size_t a=0,b=s.size();
    while (a<b && std::isspace((unsigned char)s[a])) ++a;
    while (b>a && std::isspace((unsigned char)s[b-1])) --b;
    s = s.substr(a,b-a);
}
static std::vector<std::string> split(const std::string& s, char sep){
    std::vector<std::string> out; std::string cur;
    for(char c: s){ if(c==sep){ out.push_back(cur); cur.clear(); } else cur.push_back(c); }
    out.push_back(cur); return out;
}
static std::vector<int> parse_csv_ints(const std::string& s){
    std::vector<int> v;
    if (s.empty()) return v;
    for (auto& t : split(s, ',')){
        std::string u=t; trim(u);
        if (!u.empty()) v.push_back(std::stoi(u));
    }
    return v;
}
static std::vector<std::pair<int,int>> parse_pairs(const std::string& s){
    // "(u,v);(u,v)" 형태
    std::vector<std::pair<int,int>> v;
    if (s.empty()) return v;
    for (auto& t : split(s, ';')){
        std::string u=t; trim(u);
        if (u.empty()) continue;
        // 제거
        if (u.front()=='(' && u.back()==')') u = u.substr(1, u.size()-2);
        auto xy = split(u, ',');
        if (xy.size()!=2) throw std::runtime_error("bad pair: "+t);
        v.emplace_back(std::stoi(xy[0]), std::stoi(xy[1]));
    }
    return v;
}

std::string serialize_line_compact(const Topology& T){
    // kinds, bparams
    std::ostringstream kinds, bparams;
    for (size_t i=0;i<T.block.size();++i){
        if (i){ kinds<<","; bparams<<","; }
        kinds   << kindToInt(T.block[i].kind);
        bparams << T.block[i].param;
    }
    // S/I 연결
    std::ostringstream Sc, Ic, Sp, Ip;
    for (size_t i=0;i<T.s_connection.size();++i){
        if (i) Sc<<";";
        Sc<<"("<<T.s_connection[i].u<<","<<T.s_connection[i].v<<")";
    }
    for (size_t i=0;i<T.i_connection.size();++i){
        if (i) Ic<<";";
        Ic<<"("<<T.i_connection[i].u<<","<<T.i_connection[i].v<<")";
    }
    for (size_t i=0;i<T.side_links.size();++i){
        if (i) Sp<<",";
        Sp<<T.side_links[i].param;
    }
    for (size_t i=0;i<T.instantons.size();++i){
        if (i) Ip<<",";
        Ip<<T.instantons[i].param;
    }
    std::ostringstream out;
    out << kinds.str() << " | "
        << bparams.str() << " | "
        << "S=" << Sc.str() << " | "
        << "I=" << Ic.str() << " | "
        << "sp=" << Sp.str() << " | "
        << "ip=" << Ip.str();
    return out.str();
}

bool deserialize_line_compact(const std::string& line, Topology& out){
    // 6 필드 파이프 구분
    auto parts = split(line, '|');
    if (parts.size()<2) return false;
    for (auto& p: parts) trim(p);

    // kinds
    auto kinds_csv   = parts[0];
    auto bparams_csv = parts[1];
    auto kinds_vec   = parse_csv_ints(kinds_csv);
    auto bparams_vec = parse_csv_ints(bparams_csv);
    if (kinds_vec.size()!=bparams_vec.size()) return false;

    // 초기화
    out = Topology();
    for (size_t i=0;i<kinds_vec.size();++i){
        const LKind k = intToKind(kinds_vec[i]);
        // 체인 암시: 오른쪽으로만 붙인다
        if (i==0) out.addBlock(k, bparams_vec[i]);
        else      out.addBlockRight(k, bparams_vec[i]);
    }

    // 나머지 필드는 선택적
    auto rd = [&](const std::string& tag)->std::string{
        for (size_t i=2;i<parts.size();++i){
            auto s = parts[i];
            if (s.rfind(tag,0)==0){ auto t = s.substr(tag.size()); trim(t); return t; }
        }
        return std::string();
    };
    const std::string Sraw = rd("S=");
    const std::string Iraw = rd("I=");
    const std::string SPar = rd("sp=");
    const std::string IPar = rd("ip=");

    // 파라미터 컨테이너 먼저 채우기
    auto sp = parse_csv_ints(SPar);
    auto ip = parse_csv_ints(IPar);
    for (int v: sp) out.side_links.push_back(SideLinks{v});
    for (int v: ip) out.instantons.push_back(Instantons{v});

    // 연결
    for (auto [u,sid] : parse_pairs(Sraw)){
        out.s_connection.push_back({u, sid});
    }
    for (auto [u,iid] : parse_pairs(Iraw)){
        out.i_connection.push_back({u, iid});
    }
    return true;
}

