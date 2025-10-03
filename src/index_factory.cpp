#include "vectorvault/index.hpp"

namespace vectorvault {

class HNSWIndexAdapter : public Index {
public:
    explicit HNSWIndexAdapter(int dim, const HNSWParams& params)
        : hnsw_(dim, params) {}
    
    void add(int id, std::span<const float> vec) override {
        hnsw_.add(id, vec);
    }
    
    std::vector<HNSWIndex::SearchResult> search(
        std::span<const float> query, int k, int ef) const override {
        return hnsw_.search(query, k, ef);
    }
    
    bool save(const std::string& path) const override {
        return hnsw_.save(path);
    }
    
    bool load(const std::string& path) override {
        return hnsw_.load(path);
    }
    
    int dimension() const override {
        return hnsw_.dimension();
    }
    
    size_t size() const override {
        return hnsw_.size();
    }
    
private:
    HNSWIndex hnsw_;
};

std::unique_ptr<Index> make_hnsw_index(int dim, const HNSWParams& params) {
    return std::make_unique<HNSWIndexAdapter>(dim, params);
}

} // namespace vectorvault
