#include <spdlog/spdlog.h>

#include <csignal>
#include <cstdlib>
#include <iostream>

#include "server.hpp"
#include "vectorvault/version.hpp"

namespace vectorvault {

void VectorVaultServer::setup_routes() {
    // POST /add - Add a vector
    server_.Post("/add", [this](const httplib::Request& req, httplib::Response& res) {
        handle_add(req, res);
    });

    // POST /query - Search for nearest neighbors
    server_.Post("/query", [this](const httplib::Request& req, httplib::Response& res) {
        handle_query(req, res);
    });

    // POST /save - Save index to disk
    server_.Post("/save", [this](const httplib::Request& req, httplib::Response& res) {
        handle_save(req, res);
    });

    // POST /load - Load index from disk
    server_.Post("/load", [this](const httplib::Request& req, httplib::Response& res) {
        handle_load(req, res);
    });

    // GET /stats - Get index statistics
    server_.Get("/stats", [this](const httplib::Request& req, httplib::Response& res) {
        handle_stats(req, res);
    });

    // GET /health - Health check
    server_.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });
}

void VectorVaultServer::handle_add(const httplib::Request& req, httplib::Response& res) {
    try {
        auto j = json::parse(req.body);

        if (!j.contains("id") || !j.contains("vec")) {
            res.status = 400;
            res.set_content("{\"error\":\"Missing 'id' or 'vec' field\"}", "application/json");
            return;
        }

        int id = j["id"].get<int>();
        std::vector<float> vec = j["vec"].get<std::vector<float>>();

        if (static_cast<int>(vec.size()) != dim_) {
            res.status = 400;
            json error_response = {
                {"error", "Vector dimension mismatch"}, {"expected", dim_}, {"got", vec.size()}};
            res.set_content(error_response.dump(), "application/json");
            return;
        }

        index_.add(id, vec);

        json response = {{"status", "ok"}, {"id", id}};
        res.set_content(response.dump(), "application/json");

    } catch (const json::exception& e) {
        res.status = 400;
        json error = {{"error", e.what()}};
        res.set_content(error.dump(), "application/json");
    } catch (const std::exception& e) {
        res.status = 500;
        json error = {{"error", e.what()}};
        res.set_content(error.dump(), "application/json");
    }
}

void VectorVaultServer::handle_query(const httplib::Request& req, httplib::Response& res) {
    try {
        auto start_time = std::chrono::high_resolution_clock::now();

        auto j = json::parse(req.body);

        if (!j.contains("vec")) {
            res.status = 400;
            res.set_content("{\"error\":\"Missing 'vec' field\"}", "application/json");
            return;
        }

        std::vector<float> query = j["vec"].get<std::vector<float>>();

        if (static_cast<int>(query.size()) != dim_) {
            res.status = 400;
            json error_response = {
                {"error", "Query dimension mismatch"}, {"expected", dim_}, {"got", query.size()}};
            res.set_content(error_response.dump(), "application/json");
            return;
        }

        // Parse query parameters
        int k = 10;
        int ef = 50;

        if (req.has_param("k")) {
            k = std::stoi(req.get_param_value("k"));
        }
        if (req.has_param("ef")) {
            ef = std::stoi(req.get_param_value("ef"));
        }

        // Search
        auto results = index_.search(query, k, ef);

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

        // Build response
        json response;
        response["results"] = json::array();

        for (const auto& result : results) {
            response["results"].push_back({{"id", result.id}, {"distance", result.distance}});
        }

        response["latency_us"] = duration.count();
        response["latency_ms"] = duration.count() / 1000.0;

        res.set_content(response.dump(), "application/json");

    } catch (const json::exception& e) {
        res.status = 400;
        json error = {{"error", e.what()}};
        res.set_content(error.dump(), "application/json");
    } catch (const std::exception& e) {
        res.status = 500;
        json error = {{"error", e.what()}};
        res.set_content(error.dump(), "application/json");
    }
}

void VectorVaultServer::handle_save(const httplib::Request& req, httplib::Response& res) {
    try {
        auto j = json::parse(req.body);

        if (!j.contains("path")) {
            res.status = 400;
            res.set_content("{\"error\":\"Missing 'path' field\"}", "application/json");
            return;
        }

        std::string path = j["path"].get<std::string>();

        bool success = index_.save(path);

        if (success) {
            json response = {{"status", "ok"}, {"path", path}};
            res.set_content(response.dump(), "application/json");
        } else {
            res.status = 500;
            json error = {{"error", "Failed to save index"}};
            res.set_content(error.dump(), "application/json");
        }

    } catch (const json::exception& e) {
        res.status = 400;
        json error = {{"error", e.what()}};
        res.set_content(error.dump(), "application/json");
    } catch (const std::exception& e) {
        res.status = 500;
        json error = {{"error", e.what()}};
        res.set_content(error.dump(), "application/json");
    }
}

void VectorVaultServer::handle_load(const httplib::Request& req, httplib::Response& res) {
    try {
        auto j = json::parse(req.body);

        if (!j.contains("path")) {
            res.status = 400;
            res.set_content("{\"error\":\"Missing 'path' field\"}", "application/json");
            return;
        }

        std::string path = j["path"].get<std::string>();

        bool success = index_.load(path);

        if (success) {
            json response = {{"status", "ok"},
                             {"path", path},
                             {"size", index_.size()},
                             {"dimension", index_.dimension()}};
            res.set_content(response.dump(), "application/json");
        } else {
            res.status = 500;
            json error = {{"error", "Failed to load index"}};
            res.set_content(error.dump(), "application/json");
        }

    } catch (const json::exception& e) {
        res.status = 400;
        json error = {{"error", e.what()}};
        res.set_content(error.dump(), "application/json");
    } catch (const std::exception& e) {
        res.status = 500;
        json error = {{"error", e.what()}};
        res.set_content(error.dump(), "application/json");
    }
}

void VectorVaultServer::handle_stats(const httplib::Request&, httplib::Response& res) {
    try {
        const auto& params = index_.params();

        json response = {{"size", index_.size()},
                         {"dimension", index_.dimension()},
                         {"max_level", index_.max_level()},
                         {"params",
                          {{"M", params.M},
                           {"ef_construction", params.ef_construction},
                           {"max_M", params.max_M},
                           {"max_M0", params.max_M0},
                           {"metric", params.metric == DistanceMetric::L2 ? "L2" : "COSINE"}}},
                         {"version", std::string(VERSION)}};

        res.set_content(response.dump(), "application/json");

    } catch (const std::exception& e) {
        res.status = 500;
        json error = {{"error", e.what()}};
        res.set_content(error.dump(), "application/json");
    }
}

void VectorVaultServer::start(const std::string& host, int port) {
    setup_routes();

    spdlog::info("VectorVault v{} starting...", VERSION);
    spdlog::info("Index dimension: {}", dim_);
    spdlog::info("Server listening on {}:{}", host, port);

    server_.listen(host, port);
}

}  // namespace vectorvault

int main(int argc, char* argv[]) {
    // Default configuration
    int port = 8080;
    int dimension = 384;
    std::string host = "0.0.0.0";

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--dim" && i + 1 < argc) {
            dimension = std::stoi(argv[++i]);
        } else if (arg == "--host" && i + 1 < argc) {
            host = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "VectorVault Server v" << vectorvault::VERSION << "\n\n";
            std::cout << "Usage: " << argv[0] << " [options]\n\n";
            std::cout << "Options:\n";
            std::cout << "  --port PORT    Server port (default: 8080)\n";
            std::cout << "  --dim DIM      Vector dimension (default: 384)\n";
            std::cout << "  --host HOST    Host address (default: 0.0.0.0)\n";
            std::cout << "  --help, -h     Show this help message\n";
            return 0;
        }
    }

    try {
        vectorvault::HNSWParams params;
        params.M = 16;
        params.ef_construction = 200;
        params.metric = vectorvault::DistanceMetric::L2;

        vectorvault::VectorVaultServer server(dimension, params);
        server.start(host, port);

    } catch (const std::exception& e) {
        spdlog::error("Fatal error: {}", e.what());
        return 1;
    }

    return 0;
}
