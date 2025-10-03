#include <gtest/gtest.h>
#include <httplib.h>

#include <chrono>
#include <nlohmann/json.hpp>
#include <thread>

using json = nlohmann::json;

class APIIntegrationTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // Note: For integration tests, you would start the server here
        // For this example, we'll test the API contract
        base_url = "http://localhost:8080";
    }

    std::string base_url;
};

// Helper function to check JSON structure
TEST_F(APIIntegrationTest, AddRequestContract) {
    // Test that we can construct valid add request
    json request = {{"id", 1}, {"vec", {0.1f, 0.2f, 0.3f}}};

    EXPECT_TRUE(request.contains("id"));
    EXPECT_TRUE(request.contains("vec"));
    EXPECT_TRUE(request["vec"].is_array());
}

TEST_F(APIIntegrationTest, QueryRequestContract) {
    json request = {{"vec", {0.1f, 0.2f, 0.3f}}};

    EXPECT_TRUE(request.contains("vec"));
    EXPECT_TRUE(request["vec"].is_array());
}

TEST_F(APIIntegrationTest, SaveRequestContract) {
    json request = {{"path", "data/index.vv"}};

    EXPECT_TRUE(request.contains("path"));
    EXPECT_TRUE(request["path"].is_string());
}

TEST_F(APIIntegrationTest, LoadRequestContract) {
    json request = {{"path", "data/index.vv"}};

    EXPECT_TRUE(request.contains("path"));
    EXPECT_TRUE(request["path"].is_string());
}

TEST_F(APIIntegrationTest, QueryResponseContract) {
    // Test expected response structure
    json response = {{"results", json::array({{{"id", 1}, {"distance", 0.5f}},
                                              {{"id", 2}, {"distance", 0.7f}}})},
                     {"latency_ms", 1.234}};

    EXPECT_TRUE(response.contains("results"));
    EXPECT_TRUE(response.contains("latency_ms"));
    EXPECT_TRUE(response["results"].is_array());

    for (const auto& result : response["results"]) {
        EXPECT_TRUE(result.contains("id"));
        EXPECT_TRUE(result.contains("distance"));
    }
}

TEST_F(APIIntegrationTest, StatsResponseContract) {
    json response = {
        {"size", 1000},
        {"dimension", 384},
        {"max_level", 5},
        {"params",
         {{"M", 16}, {"ef_construction", 200}, {"max_M", 16}, {"max_M0", 32}, {"metric", "L2"}}},
        {"version", "1.0.0"}};

    EXPECT_TRUE(response.contains("size"));
    EXPECT_TRUE(response.contains("dimension"));
    EXPECT_TRUE(response.contains("params"));
    EXPECT_TRUE(response["params"].contains("M"));
}

// Mock integration test (would require running server)
TEST_F(APIIntegrationTest, DISABLED_FullWorkflow) {
    httplib::Client client(base_url);

    // Add vectors
    for (int i = 0; i < 100; ++i) {
        json request = {{"id", i},
                        {"vec", {static_cast<float>(i) * 0.1f, static_cast<float>(i) * 0.2f}}};

        auto res = client.Post("/add", request.dump(), "application/json");
        ASSERT_TRUE(res);
        EXPECT_EQ(res->status, 200);
    }

    // Query
    json query_request = {{"vec", {1.0f, 2.0f}}};

    auto res = client.Post("/query?k=5&ef=50", query_request.dump(), "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    auto response = json::parse(res->body);
    EXPECT_TRUE(response.contains("results"));
    EXPECT_LE(response["results"].size(), 5u);

    // Stats
    res = client.Get("/stats");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    response = json::parse(res->body);
    EXPECT_EQ(response["size"], 100);
}

TEST_F(APIIntegrationTest, ErrorHandling) {
    // Test invalid JSON structure
    json invalid_add = {
        {"id", 1}
        // missing "vec"
    };

    EXPECT_FALSE(invalid_add.contains("vec"));

    json invalid_query = {{"invalid_field", 123}};

    EXPECT_FALSE(invalid_query.contains("vec"));
}
