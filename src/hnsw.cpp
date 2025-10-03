#include "vectorvault/hnsw.hpp"
#include "vectorvault/version.hpp"
#include "vectorvault/mmap_io.hpp"
#include <algorithm>
#include <queue>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <mutex>
#include <thread>

namespace vectorvault {

DistanceCalculator::DistanceCalculator(DistanceMetric metric) 
    : metric_(metric), uses_simd_(false) {
    
#ifdef VECTORVAULT_ENABLE_AVX2
    if (cpu_supports_avx2()) {
        uses_simd_ = true;
        if (metric == DistanceMetric::L2) {
            func_ = l2_simd;
        } else {
            func_ = cosine_simd;
        }
    } else
#endif
    {
        if (metric == DistanceMetric::L2) {
            func_ = l2_naive;
        } else {
            func_ = cosine_naive;
        }
    }
}

HNSWIndex::HNSWIndex(int dim, const HNSWParams& params)
    : dim_(dim)
    , params_(params)
    , dist_calc_(params.metric)
    , entry_point_(-1)
    , max_level_(-1)
    , rng_(params.seed)
    , level_dist_(0.0, 1.0)
{
    if (dim <= 0) {
        throw std::invalid_argument("Dimension must be positive");
    }
    if (params.M <= 0) {
        throw std::invalid_argument("M must be positive");
    }
    
    // Pre-allocate thread-local visited sets
    size_t num_threads = std::thread::hardware_concurrency();
    visited_sets_.resize(num_threads);
}

HNSWIndex::~HNSWIndex() = default;

int HNSWIndex::random_level() {
    double ml = 1.0 / std::log(2.0);
    double r = level_dist_(rng_);
    return static_cast<int>(-std::log(r) * ml);
}

void HNSWIndex::reserve(size_t n) {
    std::unique_lock lock(mutex_);
    nodes_.reserve(n);
}

void HNSWIndex::add(int id, std::span<const float> vec) {
    if (static_cast<int>(vec.size()) != dim_) {
        throw std::invalid_argument("Vector dimension mismatch");
    }
    
    std::unique_lock lock(mutex_);
    
    // Check if ID already exists
    if (id_to_index_.find(id) != id_to_index_.end()) {
        throw std::invalid_argument("ID already exists");
    }
    
    // Create new node
    auto node = std::make_unique<Node>();
    node->id = id;
    node->level = random_level();
    node->vector.assign(vec.begin(), vec.end());
    node->neighbors.resize(node->level + 1);
    
    size_t node_idx = nodes_.size();
    id_to_index_[id] = node_idx;
    
    // If this is the first node
    if (entry_point_ == -1) {
        entry_point_ = static_cast<int>(node_idx);
        max_level_ = node->level;
        nodes_.push_back(std::move(node));
        return;
    }
    
    // Search for nearest neighbors at each layer
    std::vector<int> entry_points = {entry_point_};
    
    // Greedy search from top to target layer
    for (int lc = max_level_; lc > node->level; --lc) {
        auto results = search_layer(vec, entry_points[0], 1, lc);
        if (!results.empty()) {
            entry_points[0] = results[0].id;
        }
    }
    
    // Insert at all layers from top to 0
    for (int lc = node->level; lc >= 0; --lc) {
        int ef = std::max(params_.ef_construction, params_.M);
        auto candidates = search_layer(vec, entry_points[0], ef, lc);
        
        // Select M neighbors
        int M = (lc == 0) ? params_.max_M0 : params_.max_M;
        auto neighbors = select_neighbors_heuristic(vec, candidates, M);
        
        // Add bidirectional links
        for (int neighbor_id : neighbors) {
            size_t neighbor_idx = id_to_index_[neighbor_id];
            nodes_[node_idx]->neighbors[lc].push_back(neighbor_id);
            nodes_[neighbor_idx]->neighbors[lc].push_back(id);
            
            // Prune if necessary
            int max_conn = (lc == 0) ? params_.max_M0 : params_.max_M;
            if (static_cast<int>(nodes_[neighbor_idx]->neighbors[lc].size()) > max_conn) {
                auto& neighbor_vec = nodes_[neighbor_idx]->vector;
                std::vector<SearchResult> neighbor_candidates;
                for (int conn_id : nodes_[neighbor_idx]->neighbors[lc]) {
                    if (conn_id == id) continue;
                    size_t conn_idx = id_to_index_[conn_id];
                    float dist = distance(neighbor_vec, nodes_[conn_idx]->vector);
                    neighbor_candidates.push_back({conn_id, dist});
                }
                neighbor_candidates.push_back({id, distance(neighbor_vec, vec)});
                
                auto pruned = select_neighbors_heuristic(neighbor_vec, neighbor_candidates, max_conn);
                nodes_[neighbor_idx]->neighbors[lc] = pruned;
            }
        }
        
        // Update entry point for next layer
        if (!candidates.empty()) {
            entry_points[0] = candidates[0].id;
        }
    }
    
    nodes_.push_back(std::move(node));
    
    // Update entry point if necessary
    if (nodes_[node_idx]->level > max_level_) {
        max_level_ = nodes_[node_idx]->level;
        entry_point_ = static_cast<int>(node_idx);
    }
}

std::vector<HNSWIndex::SearchResult> HNSWIndex::search_layer(
    std::span<const float> query,
    int entry_point,
    int ef,
    int layer
) const {
    std::priority_queue<SearchResult> candidates;  // Max heap
    std::priority_queue<SearchResult> results;     // Max heap
    std::unordered_set<int> visited;
    
    auto entry_it = id_to_index_.find(entry_point);
    if (entry_it == id_to_index_.end()) {
        return {};
    }
    
    size_t entry_idx = entry_it->second;
    float dist = distance(query, nodes_[entry_idx]->vector);
    
    candidates.push({entry_point, -dist});  // Negative for min heap behavior
    results.push({entry_point, dist});
    visited.insert(entry_point);
    
    while (!candidates.empty()) {
        SearchResult current = candidates.top();
        candidates.pop();
        
        if (-current.distance > results.top().distance) {
            break;
        }
        
        auto current_it = id_to_index_.find(current.id);
        if (current_it == id_to_index_.end()) {
            continue;
        }
        
        size_t current_idx = current_it->second;
        if (layer >= static_cast<int>(nodes_[current_idx]->neighbors.size())) {
            continue;
        }
        
        for (int neighbor_id : nodes_[current_idx]->neighbors[layer]) {
            if (visited.find(neighbor_id) != visited.end()) {
                continue;
            }
            visited.insert(neighbor_id);
            
            auto neighbor_it = id_to_index_.find(neighbor_id);
            if (neighbor_it == id_to_index_.end()) {
                continue;
            }
            
            size_t neighbor_idx = neighbor_it->second;
            float neighbor_dist = distance(query, nodes_[neighbor_idx]->vector);
            
            if (neighbor_dist < results.top().distance || static_cast<int>(results.size()) < ef) {
                candidates.push({neighbor_id, -neighbor_dist});
                results.push({neighbor_id, neighbor_dist});
                
                if (static_cast<int>(results.size()) > ef) {
                    results.pop();
                }
            }
        }
    }
    
    std::vector<SearchResult> result_vec;
    while (!results.empty()) {
        result_vec.push_back(results.top());
        results.pop();
    }
    std::reverse(result_vec.begin(), result_vec.end());
    
    return result_vec;
}

std::vector<int> HNSWIndex::select_neighbors_heuristic(
    std::span<const float> base_vec,
    const std::vector<SearchResult>& candidates,
    int M
) const {
    if (static_cast<int>(candidates.size()) <= M) {
        std::vector<int> result;
        result.reserve(candidates.size());
        for (const auto& c : candidates) {
            result.push_back(c.id);
        }
        return result;
    }
    
    // Simple heuristic: select M closest
    auto sorted = candidates;
    std::sort(sorted.begin(), sorted.end());
    
    std::vector<int> result;
    result.reserve(M);
    for (int i = 0; i < M && i < static_cast<int>(sorted.size()); ++i) {
        result.push_back(sorted[i].id);
    }
    
    return result;
}

std::vector<HNSWIndex::SearchResult> HNSWIndex::search(
    std::span<const float> query,
    int k,
    int ef_search
) const {
    if (static_cast<int>(query.size()) != dim_) {
        throw std::invalid_argument("Query dimension mismatch");
    }
    
    std::shared_lock lock(mutex_);
    
    if (entry_point_ == -1) {
        return {};
    }
    
    ef_search = std::max(ef_search, k);
    
    // Greedy search from top to layer 0
    std::vector<int> entry_points = {entry_point_};
    
    for (int lc = max_level_; lc > 0; --lc) {
        auto results = search_layer(query, entry_points[0], 1, lc);
        if (!results.empty()) {
            entry_points[0] = results[0].id;
        }
    }
    
    // Search at layer 0
    auto results = search_layer(query, entry_points[0], ef_search, 0);
    
    // Return top k
    if (static_cast<int>(results.size()) > k) {
        results.resize(k);
    }
    
    return results;
}

bool HNSWIndex::save(const std::string& path) const {
    std::shared_lock lock(mutex_);
    
    std::vector<uint8_t> buffer;
    BinaryWriter writer(buffer);
    
    // Write header
    writer.write_uint32(FILE_MAGIC);
    writer.write_uint32(FILE_FORMAT_VERSION);
    writer.write_int32(dim_);
    writer.write_int32(params_.M);
    writer.write_int32(params_.ef_construction);
    writer.write_int32(params_.max_M);
    writer.write_int32(params_.max_M0);
    writer.write_uint32(static_cast<uint32_t>(params_.metric));
    writer.write_int32(entry_point_);
    writer.write_int32(max_level_);
    writer.write_uint64(nodes_.size());
    
    // Write nodes
    for (const auto& node : nodes_) {
        writer.write_int32(node->id);
        writer.write_int32(node->level);
        writer.write_uint64(node->vector.size());
        writer.write_bytes(node->vector.data(), node->vector.size() * sizeof(float));
        
        // Write neighbors
        writer.write_uint64(node->neighbors.size());
        for (const auto& layer_neighbors : node->neighbors) {
            writer.write_uint64(layer_neighbors.size());
            writer.write_bytes(layer_neighbors.data(), layer_neighbors.size() * sizeof(int));
        }
    }
    
    // Compute CRC
    uint32_t crc = compute_crc32(buffer.data(), buffer.size());
    writer.write_uint32(crc);
    
    // Write to file
    MMapFile file;
    if (!file.open_write(path, buffer.size())) {
        return false;
    }
    
    std::memcpy(file.data(), buffer.data(), buffer.size());
    return true;
}

bool HNSWIndex::load(const std::string& path) {
    std::unique_lock lock(mutex_);
    
    MMapFile file;
    if (!file.open_read(path)) {
        return false;
    }
    
    BinaryReader reader(file.data(), file.size());
    
    try {
        // Read and validate header
        uint32_t magic = reader.read_uint32();
        if (magic != FILE_MAGIC) {
            return false;
        }
        
        uint32_t version = reader.read_uint32();
        if (version != FILE_FORMAT_VERSION) {
            return false;
        }
        
        int dim = reader.read_int32();
        if (dim != dim_) {
            return false;
        }
        
        params_.M = reader.read_int32();
        params_.ef_construction = reader.read_int32();
        params_.max_M = reader.read_int32();
        params_.max_M0 = reader.read_int32();
        params_.metric = static_cast<DistanceMetric>(reader.read_uint32());
        entry_point_ = reader.read_int32();
        max_level_ = reader.read_int32();
        
        uint64_t num_nodes = reader.read_uint64();
        
        // Clear existing data
        nodes_.clear();
        id_to_index_.clear();
        nodes_.reserve(num_nodes);
        
        // Read nodes
        for (uint64_t i = 0; i < num_nodes; ++i) {
            auto node = std::make_unique<Node>();
            node->id = reader.read_int32();
            node->level = reader.read_int32();
            
            uint64_t vec_size = reader.read_uint64();
            node->vector.resize(vec_size);
            reader.read_bytes(node->vector.data(), vec_size * sizeof(float));
            
            uint64_t num_layers = reader.read_uint64();
            node->neighbors.resize(num_layers);
            for (uint64_t j = 0; j < num_layers; ++j) {
                uint64_t num_neighbors = reader.read_uint64();
                node->neighbors[j].resize(num_neighbors);
                reader.read_bytes(node->neighbors[j].data(), num_neighbors * sizeof(int));
            }
            
            id_to_index_[node->id] = i;
            nodes_.push_back(std::move(node));
        }
        
        // Verify CRC (last 4 bytes)
        size_t data_size = reader.position();
        uint32_t stored_crc = reader.read_uint32();
        uint32_t computed_crc = compute_crc32(file.data(), data_size);
        
        if (stored_crc != computed_crc) {
            return false;
        }
        
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

} // namespace vectorvault
