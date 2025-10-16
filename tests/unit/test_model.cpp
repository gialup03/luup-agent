/**
 * @file test_model.cpp
 * @brief Unit tests for model layer
 */

#include <catch2/catch_test_macros.hpp>
#include <luup_agent.h>

TEST_CASE("Model creation with invalid config", "[model]") {
    SECTION("Null config") {
        luup_model* model = luup_model_create_local(nullptr);
        REQUIRE(model == nullptr);
        REQUIRE(luup_get_last_error() != nullptr);
    }
    
    SECTION("Config with null path") {
        luup_model_config config = {
            .path = nullptr,
            .gpu_layers = 0,
            .context_size = 512,
            .threads = 1,
            .api_key = nullptr,
            .api_base_url = nullptr
        };
        
        luup_model* model = luup_model_create_local(&config);
        REQUIRE(model == nullptr);
        REQUIRE(luup_get_last_error() != nullptr);
    }
}

TEST_CASE("Model creation with valid config", "[model]") {
    luup_model_config config = {
        .path = "test-model.gguf",
        .gpu_layers = 0,  // CPU only for tests
        .context_size = 512,
        .threads = 2,
        .api_key = nullptr,
        .api_base_url = nullptr
    };
    
    // Note: This will fail until Phase 1 is implemented
    // luup_model* model = luup_model_create_local(&config);
    // REQUIRE(model != nullptr);
    
    // if (model) {
    //     luup_model_destroy(model);
    // }
}

TEST_CASE("Model info retrieval", "[model]") {
    SECTION("Null model") {
        luup_model_info info;
        luup_error_t result = luup_model_get_info(nullptr, &info);
        REQUIRE(result == LUUP_ERROR_INVALID_PARAM);
    }
    
    SECTION("Null info pointer") {
        // Can't test without a valid model yet
        // Will be implemented in Phase 1
    }
}

TEST_CASE("Model destruction", "[model]") {
    SECTION("Null model") {
        // Should not crash
        luup_model_destroy(nullptr);
        REQUIRE(true);
    }
}

TEST_CASE("Remote model creation", "[model]") {
    luup_model_config config = {
        .path = "https://api.openai.com/v1",
        .gpu_layers = 0,
        .context_size = 2048,
        .threads = 0,
        .api_key = "test-key",
        .api_base_url = nullptr
    };
    
    // Note: This will be implemented in Phase 2
    // luup_model* model = luup_model_create_remote(&config);
    // REQUIRE(model != nullptr);
}

TEST_CASE("Version information", "[version]") {
    const char* version = luup_version();
    REQUIRE(version != nullptr);
    REQUIRE(strlen(version) > 0);
    
    int major, minor, patch;
    luup_version_components(&major, &minor, &patch);
    REQUIRE(major == LUUP_VERSION_MAJOR);
    REQUIRE(minor == LUUP_VERSION_MINOR);
    REQUIRE(patch == LUUP_VERSION_PATCH);
}

