#pragma once

#include "hnsw.hpp"
#include <memory>

namespace vectorvault {

// Abstract index interface (for future extensions: IVF, PQ, etc.)
class Index {
public:
    virtual ~Index() = default;
    
    virtual void add(int id, std::span<const float> vec) = 0;
    virtual std::vector<HNSWIndex::SearchResult> search(
        std::span<const float> query, int k, int ef = 50) const = 0;
    
    virtual bool save(const std::string& path) const = 0;
    virtual bool load(const std::string& path) = 0;
    
    virtual int dimension() const = 0;
    virtual size_t size() const = 0;
};

// Factory function
std::unique_ptr<Index> make_hnsw_index(int dim, const HNSWParams& params = HNSWParams{});

} // namespace vectorvault
