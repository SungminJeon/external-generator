// filter_P_type_LST_DEBUG.cpp
// DEBUG VERSION - 상세 출력으로 어디서 실패하는지 확인
// P-type endpoint = self-intersection becomes 0 after full blowdown

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <Eigen/Dense>

#include "Tensor.h"
#include "Topology_enhanced.h"
#include "TopologyDB_enhanced.hpp"
#include "TopoLineCompact_enhanced.hpp"
#include "Theory_enhanced.h"

// ===== Build TheoryGraph from Topology =====
struct GraphBuildResult {
    TheoryGraph G;
};

static inline void ensure_linear_chain(const Topology_enhanced& T,
                                       std::vector<InteriorStructure>& out_chain){
    if (!T.l_connection.empty()) { out_chain = T.l_connection; return; }
    const int n = (int)T.block.size();
    if (n <= 1) return;
    out_chain.reserve(n-1);
    for (int i=1; i<n; ++i) out_chain.push_back({i-1, i});
}

static GraphBuildResult build_graph_from_topology(const Topology_enhanced& T){
    GraphBuildResult R;

    // 1) Main block nodes
    std::vector<int> nodeIdx_block(T.block.size(), -1);
    for (size_t i=0; i<T.block.size(); ++i){
        const auto& b = T.block[i];
        Spec sp;
        switch (b.kind){
            case LKind::g: sp = Spec{Kind::Node,         b.param}; break;
            case LKind::L: sp = Spec{Kind::InteriorLink, b.param}; break;
            case LKind::S: sp = Spec{Kind::SideLink,     b.param}; break;
            case LKind::I: sp = Spec{Kind::SideLink,     b.param}; break;
            case LKind::E: sp = Spec{Kind::External,     b.param}; break;
        }
        
        nodeIdx_block[i] = R.G.add(sp).id;
    }

    // 2) Decoration nodes (SideLinks)
    std::vector<int> nodeIdx_S(T.side_links.size(), -1);
    for (size_t i=0; i<T.side_links.size(); ++i) {
        nodeIdx_S[i] = R.G.add(Spec{Kind::SideLink, T.side_links[i].param}).id;
    }

    // 3) Decoration nodes (Instantons)
    std::vector<int> nodeIdx_I(T.instantons.size(), -1);
    for (size_t i=0; i<T.instantons.size(); ++i) {
        nodeIdx_I[i] = R.G.add(Spec{Kind::SideLink, T.instantons[i].param}).id;
    }

    // 4) External curve nodes
    std::vector<int> nodeIdx_E(T.externals.size(), -1);
    for (size_t i=0; i<T.externals.size(); ++i) {
        nodeIdx_E[i] = R.G.add(Spec{Kind::External, T.externals[i].param}).id;
    }

    // 5) Interior connections
    std::vector<InteriorStructure> chain;
    ensure_linear_chain(T, chain);
    for (auto e : chain)
        R.G.connect(NodeRef{nodeIdx_block.at(e.u)}, NodeRef{nodeIdx_block.at(e.v)});

    // 6) SideLink connections
    for (auto e : T.s_connection)
        R.G.connect(NodeRef{nodeIdx_S.at(e.v)}, NodeRef{nodeIdx_block.at(e.u)});

    // 7) Instanton connections
    for (auto e : T.i_connection)
        R.G.connect(NodeRef{nodeIdx_I.at(e.v)}, NodeRef{nodeIdx_block.at(e.u)});

    // 8) External connections with AttachmentPoint support
    for (size_t idx = 0; idx < T.e_connection.size(); ++idx) {
        const auto& conn = T.e_connection[idx];
        
        if (conn.external_id < 0 || conn.external_id >= (int)nodeIdx_E.size()) {
            continue;
        }
        
        int parent_node_id = -1;
        switch (conn.parent_type) {
            case 0:  // Block
                if (conn.parent_id >= 0 && conn.parent_id < (int)nodeIdx_block.size()) {
                    parent_node_id = nodeIdx_block[conn.parent_id];
                }
                break;
            case 1:  // SideLink
                if (conn.parent_id >= 0 && conn.parent_id < (int)nodeIdx_S.size()) {
                    parent_node_id = nodeIdx_S[conn.parent_id];
                }
                break;
            case 2:  // Instanton
                if (conn.parent_id >= 0 && conn.parent_id < (int)nodeIdx_I.size()) {
                    parent_node_id = nodeIdx_I[conn.parent_id];
                }
                break;
            default:
                break;
        }
        
        if (parent_node_id >= 0) {
            AttachmentPoint ap(conn.port_idx);
            try {
                R.G.connect(NodeRef{nodeIdx_E[conn.external_id]}, AttachmentPoint(-1), 
                           NodeRef{parent_node_id}, ap);
            } catch (const std::exception& e) {
                // Silently skip failed connections
            }
        }
    }

    return R;
}

// ===== DEBUG VERSION: Check if all endpoints are P-type =====
bool has_only_P_type_endpoints(const Topology_enhanced& T) {
    std::cerr << "\n[DEBUG] ========== Processing: " << T.name << " ==========\n";
    
    try {
        // 1. Build TheoryGraph
        auto R = build_graph_from_topology(T);
        std::cerr << "[DEBUG] Step 1: Built graph with " << R.G.nodeCount() << " nodes\n";
        
        // 2. Get Intersection Form
        Eigen::MatrixXi IF;
        try {
            IF = R.G.ComposeIF_Gluing();
            std::cerr << "[DEBUG] Step 2: IF size = " << IF.rows() << "x" << IF.cols() << "\n";
        } catch (const std::exception& e) {
            std::cerr << "[DEBUG] Step 2 FAILED: ComposeIF_Gluing exception: " << e.what() << "\n";
            return false;
        }
        
        // Check 1: Empty IF
        if (IF.rows() == 0 || IF.cols() == 0) {
            std::cerr << "[DEBUG] FAIL at Check 1: Empty IF\n";
            return false;
        }
        
        // Check 2: Non-square IF
        if (IF.rows() != IF.cols()) {
            std::cerr << "[DEBUG] FAIL at Check 2: Non-square IF " << IF.rows() << "x" << IF.cols() << "\n";
            return false;
        }
        
        // Print IF diagonal before blowdown
        std::cerr << "[DEBUG] IF diagonal BEFORE blowdown: [";
        for (int i = 0; i < IF.rows(); ++i) {
            if (i > 0) std::cerr << ", ";
            std::cerr << IF(i, i);
        }
        std::cerr << "]\n";
        
        // Print full IF matrix (for small matrices)
        if (IF.rows() <= 15) {
            std::cerr << "[DEBUG] Full IF matrix BEFORE blowdown:\n";
            for (int i = 0; i < IF.rows(); ++i) {
                std::cerr << "  [";
                for (int j = 0; j < IF.cols(); ++j) {
                    if (j > 0) std::cerr << ", ";
                    std::cerr << std::setw(3) << IF(i, j);
                }
                std::cerr << "]\n";
            }
        }
        
        // 3. Create Tensor and perform full blowdown
        Tensor tensor;
        tensor.SetIF(IF);
        tensor.Setb0Q();
        
        std::cerr << "[DEBUG] Step 3: Tensor created, T = " << tensor.GetT() << "\n";
        
        // Check 3: Invalid tensor size
        if (tensor.GetT() <= 0) {
            std::cerr << "[DEBUG] FAIL at Check 3: Invalid tensor size " << tensor.GetT() << "\n";
            return false;
        }
        
        // Check 4: Size mismatch
        Eigen::MatrixXi IF_check = tensor.GetIntersectionForm();
        if (IF_check.rows() != IF.rows()) {
            std::cerr << "[DEBUG] FAIL at Check 4: Tensor IF size mismatch: " 
                      << IF_check.rows() << " vs " << IF.rows() << "\n";
            return false;
        }
        
        // Perform blowdown
        std::cerr << "[DEBUG] Step 4: Calling ForcedBlowdown()...\n";
        tensor.ForcedBlowdown();
        std::cerr << "[DEBUG] Step 4: ForcedBlowdown() completed\n";
        
        // 4. Get final matrix
        Eigen::MatrixXi IF_final = tensor.GetIntersectionForm();
        std::cerr << "[DEBUG] Step 5: IF_final size = " << IF_final.rows() << "x" << IF_final.cols() << "\n";
        
        // Check 5: Empty after blowdown
        if (IF_final.rows() == 0 || IF_final.cols() == 0) {
            std::cerr << "[DEBUG] FAIL at Check 5: Empty matrix after blowdown\n";
            return false;
        }
        
        // Check 6: Non-square after blowdown
        if (IF_final.rows() != IF_final.cols()) {
            std::cerr << "[DEBUG] FAIL at Check 6: Non-square after blowdown: "
                      << IF_final.rows() << "x" << IF_final.cols() << "\n";
            return false;
        }
        
        const int n = IF_final.rows();
        
        // Print full IF_final matrix
        std::cerr << "[DEBUG] Full IF_final matrix AFTER blowdown:\n";
        for (int i = 0; i < n; ++i) {
            std::cerr << "  [";
            for (int j = 0; j < n; ++j) {
                if (j > 0) std::cerr << ", ";
                std::cerr << std::setw(3) << IF_final(i, j);
            }
            std::cerr << "]\n";
        }
        
        // Print diagonal after blowdown
        std::cerr << "[DEBUG] IF_final diagonal AFTER blowdown: [";
        for (int i = 0; i < n; ++i) {
            if (i > 0) std::cerr << ", ";
            std::cerr << IF_final(i, i);
        }
        std::cerr << "]\n";
        
        // Special case: 1x1 matrix
        if (n == 1) {
            bool result = (IF_final(0, 0) == 0);
            std::cerr << "[DEBUG] Special case n=1: IF(0,0) = " << IF_final(0, 0) 
                      << " -> " << (result ? "P-TYPE ✓" : "NOT P-TYPE ✗") << "\n";
            return result;
        }
        
        // Find endpoints (degree 1)
        std::vector<int> endpoints;
        std::cerr << "[DEBUG] Finding endpoints (curves with degree == 1):\n";
        for (int i = 0; i < n; ++i) {
            int degree = 0;
            for (int j = 0; j < n; ++j) {
                if (i != j && IF_final(i, j) != 0) {
                    degree++;
                }
            }
            std::cerr << "  Curve " << i << ": self-int=" << std::setw(3) << IF_final(i, i) 
                      << ", degree=" << degree;
            
            if (degree == 1) {
                endpoints.push_back(i);
                std::cerr << " <- ENDPOINT";
            } else if (degree == 0) {
                std::cerr << " <- ISOLATED (degree=0)";
            }
            std::cerr << "\n";
        }
        
        // Check 7: No endpoints
        if (endpoints.empty()) {
            std::cerr << "[DEBUG] FAIL at Check 7: No endpoints found! (all curves have degree != 1)\n";
            std::cerr << "[DEBUG] This could mean: cycle topology, all isolated, or graph structure issue\n";
            return false;
        }
        
        std::cerr << "[DEBUG] Found " << endpoints.size() << " endpoint(s): [";
        for (size_t i = 0; i < endpoints.size(); ++i) {
            if (i > 0) std::cerr << ", ";
            std::cerr << endpoints[i];
        }
        std::cerr << "]\n";
        
        // Check all endpoints have self-int = 0
        std::cerr << "[DEBUG] Checking endpoint self-intersections:\n";
        for (int ep : endpoints) {
            int self_int = IF_final(ep, ep);
            std::cerr << "  Endpoint " << ep << ": self-int = " << self_int;
            if (self_int != 0) {
                std::cerr << " -> NOT P-TYPE ✗\n";
                std::cerr << "[DEBUG] FAIL at Check 8: Endpoint " << ep << " has self-int " << self_int << " != 0\n";
                return false;
            }
            std::cerr << " -> P-TYPE ✓\n";
        }
        
        std::cerr << "[DEBUG] SUCCESS: All " << endpoints.size() << " endpoint(s) are P-type!\n";
        return true;
        
    } catch (const std::out_of_range& e) {
        std::cerr << "[DEBUG] EXCEPTION (out_of_range): " << e.what() << "\n";
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[DEBUG] EXCEPTION: " << e.what() << "\n";
        return false;
    } catch (...) {
        std::cerr << "[DEBUG] UNKNOWN EXCEPTION\n";
        return false;
    }
}

// ===== Get endpoint types for statistics =====
std::vector<int> get_endpoint_types(const Topology_enhanced& T) {
    std::vector<int> types;
    
    try {
        auto R = build_graph_from_topology(T);
        Eigen::MatrixXi IF = R.G.ComposeIF_Gluing();
        
        if (IF.rows() == 0 || IF.rows() != IF.cols()) {
            return types;
        }
        
        Tensor tensor;
        tensor.SetIF(IF);
        tensor.Setb0Q();
        tensor.ForcedBlowdown();
        
        Eigen::MatrixXi IF_final = tensor.GetIntersectionForm();
        
        if (IF_final.rows() == 0) {
            return types;
        }
        
        const int n = IF_final.rows();
        
        if (n == 1) {
            types.push_back(IF_final(0, 0));
            return types;
        }
        
        for (int i = 0; i < n; ++i) {
            int degree = 0;
            for (int j = 0; j < n; ++j) {
                if (i != j && IF_final(i, j) != 0) degree++;
            }
            
            if (degree == 1) {
                if (i >= 0 && i < IF_final.rows()) {
                    types.push_back(IF_final(i, i));
                }
            }
        }
        
    } catch (...) {
        // Silently fail for statistics
    }
    
    return types;
}

// ===== Main =====
int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file> [--verbose] [--limit N]\n";
        std::cerr << "\n";
        std::cerr << "DEBUG VERSION - Shows detailed output for each topology\n";
        std::cerr << "\n";
        std::cerr << "Options:\n";
        std::cerr << "  --verbose    Show even more details\n";
        std::cerr << "  --limit N    Process only first N topologies\n";
        std::cerr << "\n";
        std::cerr << "Example:\n";
        std::cerr << "  " << argv[0] << " g.txt output.txt --limit 10\n";
        return 1;
    }
    
    const std::string input_path = argv[1];
    const std::string output_path = argv[2];
    
    // Parse options
    int limit = -1;
    for (int i = 3; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--limit" && i + 1 < argc) {
            limit = std::stoi(argv[++i]);
        }
    }
    
    // Load topologies
    std::vector<TopologyDB_enhanced::Record> records;
    
    std::ifstream fin(input_path);
    if (fin) {
        std::string line;
        int line_num = 0;
        while (std::getline(fin, line)) {
            line_num++;
            if (line.empty()) continue;
            
            Topology_enhanced T;
            if (TopoLineCompact_enhanced::deserialize(line, T)) {
                if (T.name.empty()) {
                    T.name = "line_" + std::to_string(line_num);
                }
                records.push_back({T.name, std::move(T)});
            }
        }
        fin.close();
        std::cout << "Loaded " << records.size() << " topologies\n";
    }
    
    if (records.empty()) {
        std::cerr << "No topologies loaded!\n";
        return 1;
    }
    
    std::ofstream out_file(output_path);
    if (!out_file) {
        std::cerr << "Failed to open output file\n";
        return 1;
    }
    
    int total = 0, p_type_count = 0, failed_count = 0;
    
    std::cout << "\n========== DEBUG MODE ==========\n";
    std::cout << "Processing " << (limit > 0 ? std::min(limit, (int)records.size()) : records.size()) 
              << " topologies...\n\n";
    
    for (auto& rec : records) {
        total++;
        
        if (limit > 0 && total > limit) {
            std::cout << "\n[Reached limit of " << limit << " topologies]\n";
            break;
        }
        
        std::cout << "\n################################################################\n";
        std::cout << "# Topology " << total << "/" << records.size() << ": " << rec.topo.name << "\n";
        std::cout << "################################################################\n";
        
        try {
            if (has_only_P_type_endpoints(rec.topo)) {
                std::string line = TopoLineCompact_enhanced::serialize(rec.topo);
                out_file << line << "\n";
                p_type_count++;
                std::cout << "[RESULT] ✓ P-TYPE - Added to output\n";
            } else {
                failed_count++;
                std::cout << "[RESULT] ✗ NOT P-TYPE - Filtered out\n";
            }
        } catch (const std::exception& e) {
            failed_count++;
            std::cout << "[RESULT] ✗ ERROR: " << e.what() << "\n";
        }
    }
    
    out_file.close();
    
    std::cout << "\n\n========================================\n";
    std::cout << "           SUMMARY\n";
    std::cout << "========================================\n";
    std::cout << "Total processed:     " << total << "\n";
    std::cout << "P-type (passed):     " << p_type_count << "\n";
    std::cout << "Not P-type/errors:   " << failed_count << "\n";
    std::cout << "Output saved to:     " << output_path << "\n";
    std::cout << "========================================\n";
    
    return 0;
}
