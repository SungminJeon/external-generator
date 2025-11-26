// filter_P_type_LST.cpp
// Filters LST topologies to only those with P-type endpoints
// P-type endpoint = self-intersection becomes 0 after full blowdown

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <sstream>
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

    // DEBUG: Print topology info
    

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
            case LKind::E: sp = Spec{Kind::External,     b.param}; break;  // ✅ External
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
                    
                } else {
                    
                }
                break;
            case 1:  // SideLink
                if (conn.parent_id >= 0 && conn.parent_id < (int)nodeIdx_S.size()) {
                    parent_node_id = nodeIdx_S[conn.parent_id];
                    
                } else {
                    
                }
                break;
            case 2:  // Instanton
                if (conn.parent_id >= 0 && conn.parent_id < (int)nodeIdx_I.size()) {
                    parent_node_id = nodeIdx_I[conn.parent_id];
                    
                } else {
                    
                }
                break;
            default:
                
                break;
        }
        
        if (parent_node_id >= 0) {
            
            // External.Left -> Parent.port_idx with AttachmentPoint
            AttachmentPoint ap(conn.port_idx);
            try {
                R.G.connect(NodeRef{nodeIdx_E[conn.external_id]}, AttachmentPoint(-1), 
                           NodeRef{parent_node_id}, ap);
                
            } catch (const std::exception& e) {
                
            }
        }
    }

    return R;
}

// ===== Check if all endpoints are P-type =====
bool has_only_P_type_endpoints(const Topology_enhanced& T) {
    try {
        
        
        // 1. Build TheoryGraph (External 포함)
        auto R = build_graph_from_topology(T);
        
        
        
        
        // Print each node's tensor size
        for (int i = 0; i < R.G.nodeCount(); ++i) {
            try {
                auto IF_node = R.G.IF(i);
                
            } catch (const std::exception& e) {
                
            }
        }
        
        
        
        // 2. Get Intersection Form
        Eigen::MatrixXi IF;
        try {
            IF = R.G.ComposeIF_Gluing();
            
        } catch (const std::exception& e) {
            
            throw;
        }
        
        
        
        if (IF.rows() == 0 || IF.cols() == 0) {
            std::cerr << "[Debug] Empty IF for " << T.name << "\n";
            return false;
        }
        
        if (IF.rows() != IF.cols()) {
            std::cerr << "[Debug] Non-square IF for " << T.name 
                      << ": " << IF.rows() << "x" << IF.cols() << "\n";
            return false;
        }
        
        // 3. Create Tensor and perform full blowdown
        Tensor tensor;
        
        // CRITICAL: Validate matrix before SetIF
        for (int i = 0; i < IF.rows(); ++i) {
            if (IF(i, i) > 0) {
                std::cerr << "[Debug] Positive diagonal element at " << i 
                          << " in " << T.name << "\n";
                // This might be valid, but unusual for LST
            }
        }
        
        tensor.SetIF(IF);
        tensor.Setb0Q();  // ✅ Initialize b0 before blowdown
        
        // SAFETY: Check after SetIF
        if (tensor.GetT() <= 0) {
            std::cerr << "[Debug] Invalid tensor size after SetIF: " 
                      << tensor.GetT() << "\n";
            return false;
        }
        
        // Check GetIntersectionForm is valid
        Eigen::MatrixXi IF_check = tensor.GetIntersectionForm();
        if (IF_check.rows() != IF.rows()) {
            std::cerr << "[Debug] Tensor IF size mismatch: " 
                      << IF_check.rows() << " vs " << IF.rows() << "\n";
            return false;
        }
        
        tensor.ForcedBlowdown();
        
        // 4. Get final (endpoint geometry)
        Eigen::MatrixXi IF_final = tensor.GetIntersectionForm();
        
        // SAFETY: Check after blowdown
        if (IF_final.rows() == 0 || IF_final.cols() == 0) {
            return false;
        }
        
        // SAFETY: Check matrix is square
        if (IF_final.rows() != IF_final.cols()) {
            std::cerr << "[Warning] Non-square matrix after blowdown: " 
                      << IF_final.rows() << "x" << IF_final.cols() << "\n";
            return false;
        }
        
        // 5. Check endpoints in final configuration
        const int n = IF_final.rows();
        
        // Special case: 1x1 matrix (single curve remaining)
        if (n == 1) {
            return (IF_final(0, 0) == 0);  // P-type if value is 0
        }
        
        // General case: Find endpoints (degree 1 in adjacency graph)
        std::vector<int> endpoints;
        for (int i = 0; i < n; ++i) {
            int degree = 0;
            for (int j = 0; j < n; ++j) {
                if (i != j && IF_final(i, j) != 0) {
                    degree++;
                }
            }
            
            if (degree == 1) {
                endpoints.push_back(i);
            }
        }
        
        if (endpoints.empty()) {
            return false;
        }
        
        // 6. Check all endpoints have self-intersection = 0 (P-type)
        for (int ep : endpoints) {
            // SAFETY: Check bounds
            if (ep < 0 || ep >= IF_final.rows()) {
                std::cerr << "[Warning] Endpoint index out of bounds: " << ep << "\n";
                return false;
            }
            if (IF_final(ep, ep) != 0) {
                return false;
            }
        }
        
        return true;
        
    } catch (const std::out_of_range& e) {
        std::cerr << "[Error] Out of range in " << T.name << ": " << e.what() << "\n";
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[Error] Exception in " << T.name << ": " << e.what() << "\n";
        return false;
    } catch (...) {
        std::cerr << "[Error] Unknown exception in " << T.name << "\n";
        return false;
    }
}

// ===== Get endpoint types for statistics =====
std::vector<int> get_endpoint_types(const Topology_enhanced& T) {
    std::vector<int> types;
    
    try {
        auto R = build_graph_from_topology(T);
        Eigen::MatrixXi IF = R.G.ComposeIF_Gluing();
        
        if (IF.rows() == 0 || IF.cols() == 0) return types;
        
        Tensor tensor;
        tensor.SetIF(IF);
        tensor.Setb0Q();  // ✅ Initialize b0 before blowdown
        
        if (tensor.GetT() == 0) return types;
        
        tensor.ForcedBlowdown();
        
        Eigen::MatrixXi IF_final = tensor.GetIntersectionForm();
        
        if (IF_final.rows() == 0 || IF_final.cols() == 0) return types;
        if (IF_final.rows() != IF_final.cols()) return types;
        
        const int n = IF_final.rows();
        
        if (n == 1) {
            types.push_back(IF_final(0, 0));
            return types;
        }
        
        // Find endpoints
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
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file> [--verbose]\n";
        std::cerr << "\n";
        std::cerr << "Filters LST topologies to only those with P-type endpoints.\n";
        std::cerr << "P-type endpoint = self-intersection becomes 0 after full blowdown.\n";
        std::cerr << "\n";
        std::cerr << "Input/Output formats:\n";
        std::cerr << "  - Line-compact text file (.txt)\n";
        std::cerr << "\n";
        std::cerr << "Options:\n";
        std::cerr << "  --verbose    Show detailed progress for each topology\n";
        std::cerr << "  --quiet      Show minimal output\n";
        std::cerr << "\n";
        std::cerr << "Example:\n";
        std::cerr << "  " << argv[0] << " g.txt P_type_output.txt\n";
        std::cerr << "  " << argv[0] << " g.txt P_type_output.txt --verbose\n";
        return 1;
    }
    
    const std::string input_path = argv[1];
    const std::string output_path = argv[2];
    
    // Parse options
    bool verbose = false;
    bool quiet = false;
    for (int i = 3; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--verbose" || arg == "-v") verbose = true;
        if (arg == "--quiet" || arg == "-q") quiet = true;
    }
    
    // Load topologies
    std::vector<TopologyDB_enhanced::Record> records;
    
    // Try as line-compact file first
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
            } else {
                std::cerr << "[Warning] Failed to parse line " << line_num << "\n";
            }
        }
        fin.close();
        std::cout << "Loaded " << records.size() << " topologies from line-compact file\n";
    }
    
    // If no records, try as DB
    if (records.empty()) {
        try {
            TopologyDB_enhanced in_db(input_path);
            records = in_db.loadAll();
            std::cout << "Loaded " << records.size() << " topologies from DB\n";
        } catch (const std::exception& e) {
            std::cerr << "Failed to load: " << e.what() << "\n";
            return 1;
        }
    }
    
    if (records.empty()) {
        std::cerr << "No topologies loaded!\n";
        return 1;
    }
    
    // Filter and collect statistics
    std::ofstream out_file(output_path);
    if (!out_file) {
        std::cerr << "Failed to open output file: " << output_path << "\n";
        return 1;
    }
    
    int total = 0, p_type_count = 0;
    std::map<std::vector<int>, int> endpoint_histogram;
    
    if (!quiet) {
        std::cout << "\nProcessing topologies...\n";
        if (verbose) {
            std::cout << "Verbose mode: showing details for each topology\n";
        }
        std::cout << "\n";
    }
    
    for (auto& rec : records) {
        total++;
        
        // Progress display
        if (!quiet) {
            if (verbose) {
                // Verbose: show each topology
                std::cout << "[" << total << "/" << records.size() << "] " 
                          << "Processing: " << rec.topo.name << "...";
                std::cout.flush();
            } else {
                // Normal: show every 100 or key milestones
                if (total % 100 == 0 || total == 1 || total == records.size()) {
                    std::cout << "Processed " << total << " / " << records.size() 
                              << " (" << (100.0 * total / records.size()) << "%)...\r" 
                              << std::flush;
                }
            }
        }
        
        try {
            // Get endpoint types for statistics
            auto ep_types = get_endpoint_types(rec.topo);
            if (!ep_types.empty()) {
                std::sort(ep_types.begin(), ep_types.end());
                endpoint_histogram[ep_types]++;
                
                if (verbose) {
                    std::cout << " Endpoints: [";
                    for (size_t i = 0; i < ep_types.size(); ++i) {
                        if (i > 0) std::cout << ", ";
                        std::cout << ep_types[i];
                    }
                    std::cout << "]";
                }
            }
            
            // Check if P-type only
            if (has_only_P_type_endpoints(rec.topo)) {
                // Write in line-compact format
                std::string line = TopoLineCompact_enhanced::serialize(rec.topo);
                out_file << line << "\n";
                p_type_count++;
                
                if (verbose) {
                    std::cout << " ✓ P-TYPE!\n";
                } else if (!quiet && p_type_count % 50 == 0) {
                    // Show milestone in normal mode
                    std::cout << "\n  → Found " << p_type_count << " P-type topologies so far...\n";
                }
            } else {
                if (verbose) {
                    std::cout << " (filtered)\n";
                }
            }
            
        } catch (const std::exception& e) {
            if (verbose) {
                std::cout << " ERROR: " << e.what() << "\n";
            } else {
                std::cerr << "\n[Error] " << rec.topo.name << ": " << e.what() << "\n";
            }
        }
    }
    
    // Close output file
    out_file.close();
    
    if (!quiet && !verbose) {
        std::cout << "\n";  // Final newline after progress
    }
    
    if (!quiet) {
        std::cout << "\n";
        std::cout << "========================================\n";
        std::cout << "           RESULTS\n";
        std::cout << "========================================\n";
    }
    std::cout << "Total topologies:        " << total << "\n";
    std::cout << "P-type endpoints only:   " << p_type_count 
              << " (" << (total > 0 ? 100.0 * p_type_count / total : 0) << "%)\n";
    std::cout << "Output saved to:         " << output_path << "\n";
    if (!quiet) {
        std::cout << "========================================\n\n";
    }
    
    // Print endpoint type distribution
    if (!quiet) {
        std::cout << "\nEndpoint Type Distribution (after full blowdown):\n";
        std::cout << "------------------------------------------------\n";
    
    // Convert to sorted list
    std::vector<std::pair<int, std::string>> sorted_types;
    for (const auto& [types, count] : endpoint_histogram) {
        std::ostringstream ss;
        ss << "[";
        for (size_t i = 0; i < types.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << types[i];
        }
        ss << "]";
        sorted_types.push_back({count, ss.str()});
    }
    std::sort(sorted_types.rbegin(), sorted_types.rend());
        
        int shown = 0;
        for (const auto& [count, name] : sorted_types) {
            double pct = 100.0 * count / total;
            std::cout << "  " << name << ": " << count << " (" << pct << "%)\n";
            if (++shown >= 20 && sorted_types.size() > 25) {
                std::cout << "  ... (" << (sorted_types.size() - shown) << " more types)\n";
                break;
            }
        }
        
        std::cout << "\nLegend:\n";
        std::cout << "  [0]      = Single P-type endpoint\n";
        std::cout << "  [0, 0]   = Both endpoints P-type (THESE ARE FILTERED!)\n";
        std::cout << "  [0, -1]  = One P-type, one T-type\n";
        std::cout << "  [-1, -1] = Both endpoints T-type\n";
        std::cout << "  [0, -2]  = One P-type, one E-type\n";
        std::cout << "  etc.\n";
    }  // end if (!quiet)
    
    return 0;
}
