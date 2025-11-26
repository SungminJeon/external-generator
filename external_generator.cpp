// external_generator.cpp
// Enhanced generator supporting External curve attachments with comprehensive port placement

#include "Topology_enhanced.h"
#include "TopologyDB_enhanced.hpp"
#include "TopoLineCompact_enhanced.hpp"
// ❌ REMOVED: TopoLineCompact.hpp - it includes Topology.h which conflicts with Topology_enhanced.h
#include "Tensor.h"
#include "Theory_enhanced.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>

namespace fs = std::filesystem;

// Import types from Theory_enhanced.h to avoid qualification issues
using ::NodeRef;
using ::AttachmentPoint;
using ::Port;
using ::Spec;
using ::Kind;
using ::TheoryGraph;

// ============================================================================
// Configuration
// ============================================================================

struct GeneratorConfig {
    std::string input_db_path;
    std::string output_db_path;
    int max_externals_per_topo = 3;
    bool enable_block_ports = true;
    bool enable_sidelink_ports = true;
    bool enable_instanton_ports = false;  // Usually not needed
    bool enable_interior_ports = true;    // Middle curve attachments
    int max_port_index = 2;               // 0, 1, 2 for left/middle/right
    bool check_sugra = true;
    bool verbose = false;
};

// ============================================================================
// Port Strategy
// ============================================================================

struct PortPlacement {
    int parent_id;
    int parent_type;  // 0=block, 1=sidelink, 2=instanton
    int port_idx;     // 0=left/default, 1=middle, 2=right
    
    std::string describe() const {
        std::string type_str;
        switch (parent_type) {
            case 0: type_str = "Block"; break;
            case 1: type_str = "SideLink"; break;
            case 2: type_str = "Instanton"; break;
            default: type_str = "Unknown";
        }
        std::string port_str;
        switch (port_idx) {
            case 0: port_str = "left/default"; break;
            case 1: port_str = "middle"; break;
            case 2: port_str = "right"; break;
            default: port_str = std::to_string(port_idx);
        }
        return type_str + "[" + std::to_string(parent_id) + "]@" + port_str;
    }
};

// Generate all valid port placements for a topology
std::vector<PortPlacement> generatePortPlacements(
    const Topology_enhanced& base,
    const GeneratorConfig& config
) {
    std::vector<PortPlacement> placements;
    
    // Block ports
    if (config.enable_block_ports) {
        for (int i = 0; i < static_cast<int>(base.block.size()); ++i) {
            const auto& block = base.block[i];
            
            // All blocks support port 0 (default)
            placements.push_back({i, 0, 0});
            
            // Interior links (g-L connections) support middle/right ports
            if (config.enable_interior_ports && block.kind == LKind::L) {
                for (int p = 1; p <= config.max_port_index; ++p) {
                    placements.push_back({i, 0, p});
                }
            }
            
            // Node g blocks at chain ends can have additional ports
            if (block.kind == LKind::g) {
                bool is_endpoint = true;
                for (const auto& lc : base.l_connection) {
                    if (lc.u == i || lc.v == i) {
                        is_endpoint = false;
                        break;
                    }
                }
                if (!is_endpoint) {
                    for (int p = 1; p <= config.max_port_index; ++p) {
                        placements.push_back({i, 0, p});
                    }
                }
            }
        }
    }
    
    // SideLink ports
    if (config.enable_sidelink_ports) {
        for (int i = 0; i < static_cast<int>(base.side_links.size()); ++i) {
            placements.push_back({i, 1, 0});
            // SideLinks typically only use default port
        }
    }
    
    // Instanton ports
    if (config.enable_instanton_ports) {
        for (int i = 0; i < static_cast<int>(base.instantons.size()); ++i) {
            placements.push_back({i, 2, 0});
        }
    }
    
    return placements;
}

// ============================================================================
// External Generation Strategy
// ============================================================================

// Generate combinations of N externals on available ports
struct ExternalCombination {
    std::vector<PortPlacement> assignments;  // One per external
    
    std::string describe() const {
        std::ostringstream oss;
        for (size_t i = 0; i < assignments.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << "E" << i << "@" << assignments[i].describe();
        }
        return oss.str();
    }
};

void generateCombinationsRecursive(
    const std::vector<PortPlacement>& available_ports,
    int num_externals,
    int current_external,
    std::vector<PortPlacement>& current,
    std::vector<ExternalCombination>& results
) {
    if (current_external == num_externals) {
        ExternalCombination combo;
        combo.assignments = current;
        results.push_back(combo);
        return;
    }
    
    for (const auto& port : available_ports) {
        current.push_back(port);
        generateCombinationsRecursive(
            available_ports, 
            num_externals, 
            current_external + 1, 
            current, 
            results
        );
        current.pop_back();
    }
}

std::vector<ExternalCombination> generateExternalCombinations(
    const Topology_enhanced& base,
    int num_externals,
    const GeneratorConfig& config
) {
    auto available_ports = generatePortPlacements(base, config);
    std::vector<ExternalCombination> results;
    std::vector<PortPlacement> current;
    
    generateCombinationsRecursive(
        available_ports,
        num_externals,
        0,
        current,
        results
    );
    
    return results;
}

// ============================================================================
// Topology Construction
// ============================================================================

bool constructTopologyWithExternals(
    const Topology_enhanced& base,
    const ExternalCombination& combo,
    Topology_enhanced& result,
    const GeneratorConfig& config
) {
    // Deep copy base topology
    result = base;
    
    // Add external curves
    for (size_t i = 0; i < combo.assignments.size(); ++i) {
        result.addExternal(0);  // External parameter = 0
    }
    
    // Connect externals according to combo
    for (size_t i = 0; i < combo.assignments.size(); ++i) {
        const auto& placement = combo.assignments[i];
        if (!result.attachExternal(
            static_cast<int>(i),
            placement.parent_id,
            placement.parent_type,
            placement.port_idx
        )) {
            return false;
        }
    }
    
    // Validate connection structure
    if (!TopoLineCompact_enhanced::validate(result)) {
        if (config.verbose) {
            std::cerr << "Validation failed: " 
                      << TopoLineCompact_enhanced::getValidationErrors(result) << "\n";
        }
        return false;
    }
    
    return true;
}

// ============================================================================
// Theory Validation
// ============================================================================

bool checkSupergravityConditions(
    const Topology_enhanced& T,
    const GeneratorConfig& config
) {
    if (!config.check_sugra) return true;
    
    try {
        // Build TheoryGraph
        TheoryGraph G;
        
        // Add nodes for blocks
        std::vector<NodeRef> blockNodes;
        for (size_t idx = 0; idx < T.block.size(); ++idx) {
            auto node_ref = G.add(n(T.block[idx].param));
            blockNodes.push_back(node_ref);
        }
        
        // Add nodes for sidelinks
        std::vector<NodeRef> sidelinkNodes;
        for (size_t idx = 0; idx < T.side_links.size(); ++idx) {
            auto node_ref = G.add(s(T.side_links[idx].param));
            sidelinkNodes.push_back(node_ref);
        }
        
        // Add nodes for instantons
        std::vector<NodeRef> instantonNodes;
        for (size_t idx = 0; idx < T.instantons.size(); ++idx) {
            auto node_ref = G.add(i(T.instantons[idx].param));
            instantonNodes.push_back(node_ref);
        }
        
        // Add nodes for externals
        std::vector<NodeRef> extNodes;
        for (size_t idx = 0; idx < T.externals.size(); ++idx) {
            auto node_ref = G.add(e(T.externals[idx].param));
            extNodes.push_back(node_ref);
        }
        
        // Connect interior links using default AttachmentPoints (left-to-left connections)
        for (const auto& lc : T.l_connection) {
            if (lc.u < static_cast<int>(blockNodes.size()) && 
                lc.v < static_cast<int>(blockNodes.size())) {
                G.connect(blockNodes[lc.u], AttachmentPoint(-2),  // Right end of first
                         blockNodes[lc.v], AttachmentPoint(-1));  // Left end of second
            }
        }
        
        // Connect sidelinks (default to left/right ends)
        for (const auto& sc : T.s_connection) {
            if (sc.u < static_cast<int>(blockNodes.size()) && 
                sc.v < static_cast<int>(sidelinkNodes.size())) {
                G.connect(blockNodes[sc.u], AttachmentPoint(-1), 
                         sidelinkNodes[sc.v], AttachmentPoint(-1));
            }
        }
        
        // Connect instantons (default to left/right ends)
        for (const auto& ic : T.i_connection) {
            if (ic.u < static_cast<int>(blockNodes.size()) && 
                ic.v < static_cast<int>(instantonNodes.size())) {
                G.connect(blockNodes[ic.u], AttachmentPoint(-1),
                         instantonNodes[ic.v], AttachmentPoint(-1));
            }
        }
        
        // Connect externals with port-aware attachment using AttachmentPoint
        for (const auto& conn : T.e_connection) {
            if (conn.external_id >= static_cast<int>(extNodes.size())) continue;
            
            NodeRef* parentNodeRef = nullptr;
            switch (conn.parent_type) {
                case 0:  // Block
                    if (conn.parent_id < static_cast<int>(blockNodes.size())) {
                        parentNodeRef = &blockNodes[conn.parent_id];
                    }
                    break;
                case 1:  // SideLink
                    if (conn.parent_id < static_cast<int>(sidelinkNodes.size())) {
                        parentNodeRef = &sidelinkNodes[conn.parent_id];
                    }
                    break;
                case 2:  // Instanton
                    if (conn.parent_id < static_cast<int>(instantonNodes.size())) {
                        parentNodeRef = &instantonNodes[conn.parent_id];
                    }
                    break;
            }
            
            if (parentNodeRef) {
                // Use port_idx from ExternalStructure as attachment point
                AttachmentPoint parentAP(conn.port_idx);
                AttachmentPoint externalAP(-1);  // External curves attach at default port
                G.connect(extNodes[conn.external_id], externalAP,
                         *parentNodeRef, parentAP);
            }
        }
        
        // Build intersection form and check SUGRA conditions
        Tensor tensor;
        tensor.SetIF(G.ComposeIF_Gluing());
        
        if (!tensor.IsSUGRA()) {
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        if (config.verbose) {
            std::cerr << "Theory validation error: " << e.what() << "\n";
        }
        return false;
    }
}

// ============================================================================
// Database Processing
// ============================================================================

struct GenerationStats {
    int base_topologies = 0;
    int attempted = 0;
    int successful = 0;
    int failed_construction = 0;
    int failed_validation = 0;
    int failed_sugra = 0;
    
    void print() const {
        std::cout << "\n=== Generation Statistics ===\n";
        std::cout << "Base topologies:     " << base_topologies << "\n";
        std::cout << "Attempted:           " << attempted << "\n";
        std::cout << "Successful:          " << successful << "\n";
        std::cout << "Failed construction: " << failed_construction << "\n";
        std::cout << "Failed validation:   " << failed_validation << "\n";
        std::cout << "Failed SUGRA:        " << failed_sugra << "\n";
        std::cout << "Success rate:        " 
                  << (attempted > 0 ? (100.0 * successful / attempted) : 0.0) 
                  << "%\n";
    }
};

void processDatabase(const GeneratorConfig& config, GenerationStats& stats) {
    // Open input database
    std::ifstream infile(config.input_db_path);
    if (!infile) {
        throw std::runtime_error("Cannot open input database: " + config.input_db_path);
    }
    
    // Open output database
    TopologyDB_enhanced outDB(config.output_db_path);
    
    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty()) continue;
        
        // Try to deserialize as enhanced topology first
        Topology_enhanced base;
        bool is_enhanced = TopoLineCompact_enhanced::deserialize(line, base);
        
        if (!is_enhanced) {
            // Try basic Topology format (forward compatibility)
            // This requires the basic TopoLineCompact deserializer
            continue;  // Skip basic format for now
        }
        
        stats.base_topologies++;
        
        if (config.verbose && stats.base_topologies % 100 == 0) {
            std::cout << "Processed " << stats.base_topologies << " base topologies...\n";
        }
        
        // Skip if already has externals
        if (base.hasExternalCurves()) {
            continue;
        }
        
        // Generate variants with different numbers of externals
        for (int n_ext = 1; n_ext <= config.max_externals_per_topo; ++n_ext) {
            auto combinations = generateExternalCombinations(base, n_ext, config);
            
            for (const auto& combo : combinations) {
                stats.attempted++;
                
                Topology_enhanced result;
                if (!constructTopologyWithExternals(base, combo, result, config)) {
                    stats.failed_construction++;
                    continue;
                }
                
                if (!TopoLineCompact_enhanced::validate(result)) {
                    stats.failed_validation++;
                    continue;
                }
                
                if (!checkSupergravityConditions(result, config)) {
                    stats.failed_sugra++;
                    continue;
                }
                
                // Generate unique name
                std::ostringstream name;
                name << base.name << "_ext" << n_ext << "_" << stats.successful;
                result.name = name.str();
                
                // Save to database
                if (!outDB.append(result)) {
                    std::cerr << "Warning: Failed to append to database\n";
                }
                
                stats.successful++;
                
                if (config.verbose) {
                    std::cout << "Generated: " << result.name 
                              << " - " << combo.describe() << "\n";
                }
            }
        }
    }
}

// ============================================================================
// Main
// ============================================================================

void printUsage(const char* prog) {
    std::cout << "Usage: " << prog << " [options]\n"
              << "Options:\n"
              << "  -i PATH       Input database path (required)\n"
              << "  -o PATH       Output database path (required)\n"
              << "  -n N          Max externals per topology (default: 3)\n"
              << "  -p N          Max port index (default: 2)\n"
              << "  --no-blocks   Disable block port attachments\n"
              << "  --no-sides    Disable sidelink port attachments\n"
              << "  --no-interior Disable interior port attachments\n"
              << "  --no-sugra    Disable SUGRA checking\n"
              << "  -v            Verbose output\n"
              << "  -h            Show this help\n";
}

int main(int argc, char* argv[]) {
    GeneratorConfig config;
    
    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-i" && i + 1 < argc) {
            config.input_db_path = argv[++i];
        } else if (arg == "-o" && i + 1 < argc) {
            config.output_db_path = argv[++i];
        } else if (arg == "-n" && i + 1 < argc) {
            config.max_externals_per_topo = std::stoi(argv[++i]);
        } else if (arg == "-p" && i + 1 < argc) {
            config.max_port_index = std::stoi(argv[++i]);
        } else if (arg == "--no-blocks") {
            config.enable_block_ports = false;
        } else if (arg == "--no-sides") {
            config.enable_sidelink_ports = false;
        } else if (arg == "--no-interior") {
            config.enable_interior_ports = false;
        } else if (arg == "--no-sugra") {
            config.check_sugra = false;
        } else if (arg == "-v") {
            config.verbose = true;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }
    
    // Validate configuration
    if (config.input_db_path.empty() || config.output_db_path.empty()) {
        std::cerr << "Error: Input and output paths are required\n";
        printUsage(argv[0]);
        return 1;
    }
    
    std::cout << "=== External Curve Generator (Enhanced) ===\n";
    std::cout << "Input:  " << config.input_db_path << "\n";
    std::cout << "Output: " << config.output_db_path << "\n";
    std::cout << "Max externals per topology: " << config.max_externals_per_topo << "\n";
    std::cout << "Max port index: " << config.max_port_index << "\n";
    std::cout << "Block ports: " << (config.enable_block_ports ? "yes" : "no") << "\n";
    std::cout << "SideLink ports: " << (config.enable_sidelink_ports ? "yes" : "no") << "\n";
    std::cout << "Interior ports: " << (config.enable_interior_ports ? "yes" : "no") << "\n";
    std::cout << "SUGRA checking: " << (config.check_sugra ? "yes" : "no") << "\n";
    std::cout << "\n";
    
    try {
        GenerationStats stats;
        processDatabase(config, stats);
        stats.print();
        
        std::cout << "\nDone! Output written to: " << config.output_db_path << "\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
