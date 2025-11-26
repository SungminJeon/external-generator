#pragma once
#include "Topology.h"
#include <string>

// 포맷(한 줄):
// kinds | bparams | S=(u,v);... | I=(u,v);... | sp=... | ip=...
// - kinds: 0,1,2,3 (= g,L,S,I) CSV
// - bparams: 블록 파라미터 CSV
// - S/I 연결: (nodeIdx,sideOrInstId) 세미콜론 분리. 없으면 비움.
// - sp(사이드 파라미터), ip(인스턴톤 파라미터): CSV. 없으면 비움.
// - l_connection은 저장하지 않음(체인 암시).
//
// 예) "0,1,0,1 | -2,-3,-2,-1 | S=(0,0);(1,1) | I= | sp=22,33 | ip="

std::string serialize_line_compact(const Topology& T);
bool        deserialize_line_compact(const std::string& line, Topology& out);

