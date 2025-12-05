// external_generator_simple.cpp
// Simplified External curve generator with LST/SCFT classification
// Supports directory input and s(0) notation for attachment specification
// ✨ ENHANCED: Gluing rules based on self-intersection of attachment curve

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>

#include "Topology_enhanced.h"
#include "TopologyDB_enhanced.hpp"
#include "TopoLineCompact_enhanced.hpp"
#include "Theory_enhanced.h"
#include "Tensor.h"
#include <sstream>
#include <unordered_set>

namespace fs = std::filesystem;

// Import types
using ::NodeRef;
using ::AttachmentPoint;
using ::Spec;
using ::TheoryGraph;

// ============================================================================
// ✨ GLUING RULES - EDIT HERE
// ============================================================================
// Maps self-intersection of attachment curve -> allowed external parameters
// Edit this function to change the gluing rules!

std::vector<int> get_allowed_external_params(int self_int) {
    // =====================================================
    // GLUING RULE TABLE
    // self_int: self-intersection of the curve where external attaches
    // Returns: list of allowed external curve parameters
    // =====================================================
    
    switch (self_int) {
        // Node curves
        case -12: return {1};            // e8
        case -11: return {1};            // e8'
        case -10: return {1};            // e8''
        case -9:  return {1};            // e8'''
        case -8:  return {1};            // e7
        case -7:  return {1};         // e7'
        case -6:  return {1};         // e6
        case -5:  return {1};      // f4
        case -4:  return {1};      // so8
        case -3:  return {1, 2};   // su2
        
        // Link curves (typically -1, -2)
        case -2:  return {1, 2, 3};
        case -1:  return {1, 2, 3, 5};
        
        // Default: allow all
        default:  return {1, 2, 3, 5};
    }
}

// Alternative: If you want even more control, use this version
// which considers both self_int and the parent type/param
/*
std::vector<int> get_allowed_external_params_advanced(
    int self_int,           // Self-intersection of attachment curve
    int parent_type,        // 0=block, 1=sidelink, 2=instanton
    int parent_param,       // Parameter of the parent (e.g., 944 for SideLink)
    int port_idx            // Port index on the parent
) {
    // Example: Different rules for different parent types
    if (parent_type == 0) {  // Block (Node)
        // Node-specific rules
        switch (self_int) {
            case -6: return {1, 2};
            case -5: return {1, 2, 3};
            // ...
        }
    } else if (parent_type == 1) {  // SideLink
        // SideLink-specific rules based on param
        if (parent_param >= 940 && parent_param <= 950) {
            // Special alkali links
            return {1, 2};
        }
    }
    
    // Default
    return {1, 2, 3, 5};
}
*/

// ============================================================================
// Configuration
// ============================================================================

struct Config {
    std::string input_path;        // File or directory
    std::string output_dir;        // Output directory
    std::vector<std::string> attachment_specs;  // e.g., "s(0)", "s(1)", "b(2)"
    int num_threads = std::thread::hardware_concurrency();
    bool verbose = false;
    bool classify_only = false;     // If true, only classify without adding externals
};

// ============================================================================
// LST/SCFT Classification
// ============================================================================

enum class TopoCategory {
    LST,
    SCFT,
    Neither,
    Error
};

const char* category_name(TopoCategory cat) {
    switch (cat) {
        case TopoCategory::LST: return "LST";
        case TopoCategory::SCFT: return "SCFT";
        case TopoCategory::Neither: return "Neither";
        case TopoCategory::Error: return "Error";
    }
    return "Unknown";
}

TopoCategory classify_topology(const Topology_enhanced& T) {
    try {
        // Build TheoryGraph
        TheoryGraph G;
        
        std::vector<NodeRef> blockNodes;
        for (const auto& b : T.block) {
            Spec sp;
            switch(b.kind) {
                case LKind::g: sp = n(b.param); break;
                case LKind::L: sp = i(b.param); break;
                case LKind::S: sp = s(b.param); break;
                case LKind::I: sp = s(b.param); break;
                case LKind::E: sp = e(b.param); break;
                default: sp = n(b.param); break;
            }
            blockNodes.push_back(G.add(sp));
        }
        
        std::vector<NodeRef> sideNodes;
        for (const auto& sl : T.side_links) {
            sideNodes.push_back(G.add(s(sl.param)));
        }
        
        std::vector<NodeRef> instNodes;
        for (const auto& inst : T.instantons) {
            instNodes.push_back(G.add(s(inst.param)));
        }
        
        std::vector<NodeRef> extNodes;
        for (const auto& ext : T.externals) {
            extNodes.push_back(G.add(e(ext.param)));
        }
        
        // Connect interior links (Right-to-Left by default)
        for (const auto& conn : T.l_connection) {
            if (conn.u >= 0 && conn.u < (int)blockNodes.size() &&
                conn.v >= 0 && conn.v < (int)blockNodes.size()) {
                // Default connect: blockNodes[u].Right -> blockNodes[v].Left
                G.connect(blockNodes[conn.u], blockNodes[conn.v]);
            }
        }
        
        // Connect sidelinks (SideLink.Right -> Block.Left)
        for (const auto& conn : T.s_connection) {
            if (conn.u >= 0 && conn.u < (int)blockNodes.size() &&
                conn.v >= 0 && conn.v < (int)sideNodes.size()) {
                // sideNodes[v].Right -> blockNodes[u].Left
                G.connect(sideNodes[conn.v], blockNodes[conn.u]);
            }
        }
        
        // Connect instantons (Instanton.Right -> Block.Left)
        for (const auto& conn : T.i_connection) {
            if (conn.u >= 0 && conn.u < (int)blockNodes.size() &&
                conn.v >= 0 && conn.v < (int)instNodes.size()) {
                // instNodes[v].Right -> blockNodes[u].Left
                G.connect(instNodes[conn.v], blockNodes[conn.u]);
            }
        }
        
        // Connect externals with AttachmentPoint
        for (const auto& conn : T.e_connection) {
            if (conn.external_id < 0 || conn.external_id >= (int)extNodes.size()) 
                continue;
            
            NodeRef* parentRef = nullptr;
            switch (conn.parent_type) {
                case 0:  // Block
                    if (conn.parent_id >= 0 && conn.parent_id < (int)blockNodes.size())
                        parentRef = &blockNodes[conn.parent_id];
                    break;
                case 1:  // SideLink
                    if (conn.parent_id >= 0 && conn.parent_id < (int)sideNodes.size())
                        parentRef = &sideNodes[conn.parent_id];
                    break;
                case 2:  // Instanton
                    if (conn.parent_id >= 0 && conn.parent_id < (int)instNodes.size())
                        parentRef = &instNodes[conn.parent_id];
                    break;
            }
            
            if (parentRef) {
                AttachmentPoint ap(conn.port_idx);
                G.connect(extNodes[conn.external_id], AttachmentPoint(-1), *parentRef, ap);
            }
        }
        
        // Get intersection form
        auto IF = G.ComposeIF_Gluing();
        if (IF.rows() == 0) return TopoCategory::Error;
        
        Tensor tensor;
        tensor.SetIF(IF);
        
        auto eigenvalues = tensor.GetEigenvalues();
        if (eigenvalues.size() == 0) return TopoCategory::Error;
        
        const double tol = 1e-8;
        int zero_count = 0;
        int negative_count = 0;
        int positive_count = 0;
        
        for (int idx = 0; idx < eigenvalues.size(); idx++) {
            double ev = eigenvalues(idx);
            if (std::abs(ev) < tol) {
                zero_count++;
            } else if (ev < -tol) {
                negative_count++;
            } else {
                positive_count++;
            }
        }
        
        // LST: Exactly one zero eigenvalue, rest negative
        if (zero_count == 1 && negative_count == eigenvalues.size() - 1 && positive_count == 0) {
            return TopoCategory::LST;
        }
        
        // SCFT: All eigenvalues negative (negative definite)
        if (zero_count == 0 && negative_count == eigenvalues.size() && positive_count == 0) {
            return TopoCategory::SCFT;
        }
        
        return TopoCategory::Neither;
        
    } catch (const std::exception& e) {
        return TopoCategory::Error;
    }
}

// ============================================================================
// Attachment Specification Parsing
// ============================================================================

struct AttachmentSpec {
    enum Type { Block, SideLink, Instanton } type;
    int index;
    
    std::string describe() const {
        std::string type_str;
        switch (type) {
            case Block: type_str = "b"; break;
            case SideLink: type_str = "s"; break;
            case Instanton: type_str = "i"; break;
        }
        return type_str + "(" + std::to_string(index) + ")";
    }
};

bool parse_attachment_spec(const std::string& spec, AttachmentSpec& out) {
    // Format: "s(0)", "b(1)", "i(2)" etc.
    if (spec.size() < 4) return false;
    
    char type_char = spec[0];
    if (spec[1] != '(' || spec.back() != ')') return false;
    
    std::string num_str = spec.substr(2, spec.size() - 3);
    try {
        int idx = std::stoi(num_str);
        if (idx < 0) return false;
        
        switch (type_char) {
            case 's':
            case 'S':
                out.type = AttachmentSpec::SideLink;
                out.index = idx;
                return true;
            case 'b':
            case 'B':
                out.type = AttachmentSpec::Block;
                out.index = idx;
                return true;
            case 'i':
            case 'I':
                out.type = AttachmentSpec::Instanton;
                out.index = idx;
                return true;
            default:
                return false;
        }
    } catch (...) {
        return false;
    }
}

// ============================================================================
// External Attachment
// ============================================================================

struct PortInfo {
    int parent_id;
    int parent_type;    // 0=block, 1=sidelink, 2=instanton
    int port_idx;
    int self_int;       // ✨ Self-intersection of the curve at this port
    int parent_param;   // ✨ Parameter of the parent (for advanced rules)
};

// ✨ Helper: Get intersection form diagonal for a spec
std::vector<int> get_spec_diagonal(const Spec& sp) {
    Tensor t = build_tensor(sp);
    auto IF = t.GetIntersectionForm();
    std::vector<int> diag;
    for (int i = 0; i < IF.rows(); ++i) {
        diag.push_back(IF(i, i));
    }
    return diag;
}

std::vector<PortInfo> get_possible_ports(const Topology_enhanced& T, const AttachmentSpec& spec) {
    std::vector<PortInfo> ports;
    
    switch (spec.type) {
        case AttachmentSpec::Block:
            if (spec.index >= 0 && spec.index < (int)T.block.size()) {
                const auto& block = T.block[spec.index];
                
                // Build spec to get diagonal
                Spec sp;
                switch(block.kind) {
                    case LKind::g: sp = n(block.param); break;
                    case LKind::L: sp = i(block.param); break;
                    case LKind::S: sp = s(block.param); break;
                    case LKind::I: sp = s(block.param); break;
                    case LKind::E: sp = e(block.param); break;
                    default: sp = n(block.param); break;
                }
                
                auto diag = get_spec_diagonal(sp);
                
                // Add all possible ports with their self-intersections
                for (int port_idx = 0; port_idx < (int)diag.size(); ++port_idx) {
                    ports.push_back({
                        spec.index,         // parent_id
                        0,                  // parent_type (Block)
                        port_idx,           // port_idx
                        diag[port_idx],     // self_int
                        block.param         // parent_param
                    });
                }
            }
            break;
            
        case AttachmentSpec::SideLink:
            if (spec.index >= 0 && spec.index < (int)T.side_links.size()) {
                int param = T.side_links[spec.index].param;
                Spec sp = s(param);
                auto diag = get_spec_diagonal(sp);
                
                // Add all possible ports
                for (int port_idx = 0; port_idx < (int)diag.size(); ++port_idx) {
                    ports.push_back({
                        spec.index,         // parent_id
                        1,                  // parent_type (SideLink)
                        port_idx,           // port_idx
                        diag[port_idx],     // self_int
                        param               // parent_param
                    });
                }
            }
            break;
            
        case AttachmentSpec::Instanton:
            if (spec.index >= 0 && spec.index < (int)T.instantons.size()) {
                int param = T.instantons[spec.index].param;
                Spec sp = s(param);  // Instantons use SideLink spec
                auto diag = get_spec_diagonal(sp);
                
                // Add all possible ports
                for (int port_idx = 0; port_idx < (int)diag.size(); ++port_idx) {
                    ports.push_back({
                        spec.index,         // parent_id
                        2,                  // parent_type (Instanton)
                        port_idx,           // port_idx
                        diag[port_idx],     // self_int
                        param               // parent_param
                    });
                }
            }
            break;
    }
    
    return ports;
}

bool add_external_at_port(Topology_enhanced& T, const PortInfo& port, int ext_param) {
    // Add one External curve with specified parameter
    int ext_id = T.addExternal(ext_param);
    
    // Connect it to the specified port
    return T.attachExternal(ext_id, port.parent_id, port.parent_type, port.port_idx);
}

// ============================================================================
// Output Management
// ============================================================================

struct OutputBuffer {
    std::unordered_map<std::string, std::string> buffers;
    std::mutex mtx;
    
    void append(const std::string& path, const std::string& line) {
        std::lock_guard<std::mutex> lock(mtx);
        buffers[path] += line + '\n';
    }
    
    void flush_to_disk() {
        std::lock_guard<std::mutex> lock(mtx);
        for (const auto& [path, content] : buffers) {
            if (content.empty()) continue;
            
            fs::create_directories(fs::path(path).parent_path());
            std::ofstream out(path, std::ios::app);
            if (out) {
                out << content;
            }
        }
        buffers.clear();
    }
};

std::string get_output_path(const std::string& output_dir, TopoCategory category, 
                            const Topology_enhanced& T) {
    // Create path: output_dir/CATEGORY/len-N/prefix.txt
    std::string cat_str = category_name(category);
    int len = (int)T.block.size();
    
    // Generate prefix from first few blocks
    std::string prefix;
    for (int idx = 0; idx < std::min(4, (int)T.block.size()); idx++) {
        switch (T.block[idx].kind) {
            case LKind::g: prefix += 'g'; break;
            case LKind::L: prefix += 'L'; break;
            case LKind::S: prefix += 'S'; break;
            case LKind::I: prefix += 'I'; break;
            case LKind::E: prefix += 'E'; break;
        }
    }
    if (prefix.empty()) prefix = "empty";
    
    std::string dir = output_dir + "/" + cat_str + "/len-" + std::to_string(len);
    fs::create_directories(dir);
    return dir + "/" + prefix + ".txt";
}

// ============================================================================
// Processing
// ============================================================================

struct Stats {
    std::atomic<int> total_input{0};
    std::atomic<int> total_output{0};
    std::atomic<int> lst_count{0};
    std::atomic<int> scft_count{0};
    std::atomic<int> neither_count{0};
    std::atomic<int> error_count{0};
    
    void print() const {
        std::cout << "\n=== Statistics ===\n";
        std::cout << "Input topologies:     " << total_input << "\n";
        std::cout << "Output topologies:    " << total_output << "\n";
        std::cout << "  LST:                " << lst_count << "\n";
        std::cout << "  SCFT:               " << scft_count << "\n";
        std::cout << "  Neither:            " << neither_count << "\n";
        std::cout << "  Errors:             " << error_count << "\n";
    }
};

void process_topology(const Topology_enhanced& base, const Config& config,
                     OutputBuffer& output, Stats& stats) {
    stats.total_input++;
    
    // If no attachment specs, just classify the base topology
    if (config.attachment_specs.empty() || config.classify_only) {
        TopoCategory cat = classify_topology(base);
        
        switch (cat) {
            case TopoCategory::LST: stats.lst_count++; break;
            case TopoCategory::SCFT: stats.scft_count++; break;
            case TopoCategory::Neither: stats.neither_count++; break;
            case TopoCategory::Error: stats.error_count++; return;
        }
        
        std::string path = get_output_path(config.output_dir, cat, base);
        std::string line = TopoLineCompact_enhanced::serialize(base);
        output.append(path, line);
        stats.total_output++;
        return;
    }
    
    // Process each attachment specification
    for (const auto& spec_str : config.attachment_specs) {
        AttachmentSpec spec;
        if (!parse_attachment_spec(spec_str, spec)) {
            if (config.verbose) {
                std::cerr << "Invalid spec: " << spec_str << "\n";
            }
            continue;
        }
        
        // Get all possible ports for this specification
        auto ports = get_possible_ports(base, spec);
        
        // Try each port
        for (const auto& port : ports) {
            // ✨ Get allowed external params based on self-intersection
            std::vector<int> allowed_ext_params = get_allowed_external_params(port.self_int);
            
            if (config.verbose) {
                std::cout << "  Port " << spec.describe() << "[" << port.port_idx << "]"
                          << " (self-int=" << port.self_int << ")"
                          << " allows: {";
                for (size_t i = 0; i < allowed_ext_params.size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << allowed_ext_params[i];
                }
                std::cout << "}\n";
            }
            
            // Try each allowed external parameter
            for (int ext_param : allowed_ext_params) {
                Topology_enhanced T = base;  // Copy
                
                if (!add_external_at_port(T, port, ext_param)) {
                    continue;
                }
                
                // Classify
                TopoCategory cat = classify_topology(T);
                
                switch (cat) {
                    case TopoCategory::LST: stats.lst_count++; break;
                    case TopoCategory::SCFT: stats.scft_count++; break;
                    case TopoCategory::Neither: stats.neither_count++; break;
                    case TopoCategory::Error: stats.error_count++; continue;
                }
                
                // Only output LST or SCFT
                if (cat == TopoCategory::LST || cat == TopoCategory::SCFT) {
                    std::string path = get_output_path(config.output_dir, cat, T);
                    std::string line = TopoLineCompact_enhanced::serialize(T);
                    output.append(path, line);
                    stats.total_output++;
                }
            }
        }
    }
}

void process_file(const std::string& filepath, const Config& config,
                 OutputBuffer& output, Stats& stats) {
    std::ifstream infile(filepath);
    if (!infile) {
        std::cerr << "Cannot open: " << filepath << "\n";
        return;
    }
    
    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty()) continue;
        
        Topology_enhanced T;
        if (!TopoLineCompact_enhanced::deserialize(line, T)) {
            continue;
        }
        
        process_topology(T, config, output, stats);
        
        if (config.verbose && stats.total_input % 100 == 0) {
            std::cout << "Processed " << stats.total_input << " topologies...\r" << std::flush;
        }
    }
}

// ============================================================================
// Main
// ============================================================================

void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " [options]\n"
              << "\nOptions:\n"
              << "  -i PATH         Input file or directory (required)\n"
              << "  -o DIR          Output directory (required)\n"
              << "  -a SPEC         Attachment specification (e.g., s(0), b(1))\n"
              << "                  Can be specified multiple times\n"
              << "                  If omitted, only classifies without adding externals\n"
              << "  --classify-only Only classify existing topologies\n"
              << "  -v              Verbose output\n"
              << "  -h              Show this help\n"
              << "\nAttachment Specifications:\n"
              << "  s(N)  - Attach External to all ports of SideLink N\n"
              << "  b(N)  - Attach External to all ports of Block N\n"
              << "  i(N)  - Attach External to all ports of Instanton N\n"
              << "\nGluing Rules:\n"
              << "  External parameters are filtered based on the self-intersection\n"
              << "  of the attachment curve. Edit get_allowed_external_params() in\n"
              << "  the source code to modify the rules.\n"
              << "\nExamples:\n"
              << "  # Classify only\n"
              << "  " << prog << " -i input.txt -o output_dir\n"
              << "\n"
              << "  # Add External to SideLink 0\n"
              << "  " << prog << " -i input.txt -o output_dir -a s(0)\n"
              << "\n"
              << "  # Add External to multiple locations\n"
              << "  " << prog << " -i input.txt -o output_dir -a s(0) -a s(1) -a b(2)\n"
              << "\n"
              << "  # Process directory with verbose output\n"
              << "  " << prog << " -i input_dir -o output_dir -a s(0) -v\n";
}

int main(int argc, char* argv[]) {
    Config config;
    
    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-i" && i + 1 < argc) {
            config.input_path = argv[++i];
        } else if (arg == "-o" && i + 1 < argc) {
            config.output_dir = argv[++i];
        } else if (arg == "-a" && i + 1 < argc) {
            config.attachment_specs.push_back(argv[++i]);
        } else if (arg == "--classify-only") {
            config.classify_only = true;
        } else if (arg == "-v") {
            config.verbose = true;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }
    
    // Validate
    if (config.input_path.empty() || config.output_dir.empty()) {
        std::cerr << "Error: Input and output paths required\n";
        print_usage(argv[0]);
        return 1;
    }
    
    std::cout << "=== External Generator (Simple) ===\n";
    std::cout << "Input:  " << config.input_path << "\n";
    std::cout << "Output: " << config.output_dir << "\n";
    
    if (config.classify_only || config.attachment_specs.empty()) {
        std::cout << "Mode: Classification only\n";
    } else {
        std::cout << "Attachment specs:\n";
        for (const auto& spec : config.attachment_specs) {
            std::cout << "  - " << spec << "\n";
        }
        std::cout << "\nGluing rules active (see get_allowed_external_params)\n";
    }
    std::cout << "\n";
    
    OutputBuffer output;
    Stats stats;
    
    // Process input
    if (fs::is_directory(config.input_path)) {
        // Process all .txt files in directory
        for (const auto& entry : fs::recursive_directory_iterator(config.input_path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".txt") {
                if (config.verbose) {
                    std::cout << "Processing: " << entry.path().filename() << "\n";
                }
                process_file(entry.path().string(), config, output, stats);
            }
        }
    } else {
        // Process single file
        process_file(config.input_path, config, output, stats);
    }
    
    // Flush output
    if (config.verbose) {
        std::cout << "\nFlushing output...\n";
    }
    output.flush_to_disk();
    
    // Print statistics
    stats.print();
    
    std::cout << "\nDone! Output written to: " << config.output_dir << "\n";
    
    return 0;
}
