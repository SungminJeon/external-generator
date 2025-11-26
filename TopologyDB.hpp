// TopologyDB.hpp
#pragma once
#include "Topology.h"
#include <string>
#include <vector>
#include <functional>

class TopologyDB {
public:
    struct Record {
        std::string name;
        Topology    topo;
    };

    explicit TopologyDB(std::string path);

    bool append(const Topology& T) const;
    std::vector<Record> loadAll() const;
    bool loadByName(const std::string& name, Topology& out) const;

    // ✅ 추가: 중복제거
    // 같은 payload(내용)인 레코드 제거. keep_last=true면 가장 마지막 것만 유지
    int  dedupeByContentHash(bool keep_last=false) const;

    // 같은 name인 레코드 제거. keep_last=true면 가장 마지막 것만 유지
    int  dedupeByName(bool keep_last=false) const;

    static std::string serializeCanonical(const Topology& T);
    static bool        deserializeCanonical(std::istream& in, int nLines, Topology& out);

private:
    std::string path_;
    static int  countLines(const std::string& s);

    // 내부 유틸 (헤더 비공개 static)
    static std::string cheapHashHex(const std::string& s);
    static bool writeFileAtomic(const std::string& path, const std::string& content);
};

