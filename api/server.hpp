#pragma once

#include "vectorvault/hnsw.hpp"
#include "vectorvault/thread_pool.hpp"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <memory>
#include <mutex>
#include <chrono>

namespace vectorvault {

using json = nlohmann::json;

class VectorVaultServer {
public:
    explicit VectorVaultServer(int dim, const HNSWParams& params = HNSWParams{})
        : index_(dim, params)
        , thread_pool_(std::thread::hardware_concurrency())
        , dim_(dim)
    {}
    
    void start(const std::string& host = "0.0.0.0", int port = 8080);
    
private:
    void setup_routes();
    
    // Route handlers
    void handle_add(const httplib::Request& req, httplib::Response& res);
    void handle_query(const httplib::Request& req, httplib::Response& res);
    void handle_save(const httplib::Request& req, httplib::Response& res);
    void handle_load(const httplib::Request& req, httplib::Response& res);
    void handle_stats(const httplib::Request& req, httplib::Response& res);
    
    HNSWIndex index_;
    ThreadPool thread_pool_;
    httplib::Server server_;
    int dim_;
    std::mutex server_mutex_;
};

} // namespace vectorvault
