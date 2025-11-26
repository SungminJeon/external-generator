// external_generator_simple.cpp
// Simplified External curve generator with LST/SCFT classification
// Supports directory input and s(0) notation for attachment specification

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
    int parent_type;  // 0=block, 1=sidelink, 2=instanton
    int port_idx;
};

std::vector<PortInfo> get_possible_ports(const Topology_enhanced& T, const AttachmentSpec& spec) {
    std::vector<PortInfo> ports;
    
    switch (spec.type) {
        case AttachmentSpec::Block:
            if (spec.index >= 0 && spec.index < (int)T.block.size()) {
                const auto& block = T.block[spec.index];
                
                // Default port
                ports.push_back({spec.index, 0, 0});
                
                // Interior links (L) have additional ports
                if (block.kind == LKind::L) {
                    ports.push_back({spec.index, 0, 1});  // Middle
                    ports.push_back({spec.index, 0, 2});  // Right
                }
            }
            break;
            
        case AttachmentSpec::SideLink:
            if (spec.index >= 0 && spec.index < (int)T.side_links.size()) {
                // SideLinks typically have only default port
                ports.push_back({spec.index, 1, 0});
            }
            break;
            
        case AttachmentSpec::Instanton:
            if (spec.index >= 0 && spec.index < (int)T.instantons.size()) {
                // Instantons typically have only default port
                ports.push_back({spec.index, 2, 0});
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
    
    // Allowed external parameters
    static const std::vector<int> allowed_ext_params = {1, 2, 3, 5};
    
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
              << "  # Process directory\n"
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
