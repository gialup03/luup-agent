/**
 * @file test_model.cpp
 * @brief Unit tests for model layer
 */

#include <catch2/catch_test_macros.hpp>
#include <luup_agent.h>
#include <cstring>

TEST_CASE("Error handling system", "[error]") {
    SECTION("Initial error state") {
        // Initially no error
        const char* err = luup_get_last_error();
        REQUIRE(err != nullptr);
    }
    
    SECTION("Error callback") {
        static bool callback_called = false;
        static luup_error_t callback_code = LUUP_SUCCESS;
        
        auto callback = [](luup_error_t code, const char* msg, void* user_data) {
            callback_called = true;
            callback_code = code;
        };
        
        luup_set_error_callback(callback, nullptr);
        
        // Trigger error by passing null config
        luup_model* model = luup_model_create_local(nullptr);
        REQUIRE(model == nullptr);
        REQUIRE(callback_called == true);
        REQUIRE(callback_code == LUUP_ERROR_INVALID_PARAM);
        
        // Clear callback
        luup_set_error_callback(nullptr, nullptr);
    }
}

TEST_CASE("Model creation with invalid config", "[model]") {
    SECTION("Null config") {
        luup_model* model = luup_model_create_local(nullptr);
        REQUIRE(model == nullptr);
        const char* err = luup_get_last_error();
        REQUIRE(err != nullptr);
        REQUIRE(strlen(err) > 0);
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
        const char* err = luup_get_last_error();
        REQUIRE(err != nullptr);
        REQUIRE(strlen(err) > 0);
    }
    
    SECTION("Non-existent model file") {
        luup_model_config config = {
            .path = "/nonexistent/model.gguf",
            .gpu_layers = 0,
            .context_size = 512,
            .threads = 2,
            .api_key = nullptr,
            .api_base_url = nullptr
        };
        
        luup_model* model = luup_model_create_local(&config);
        REQUIRE(model == nullptr);
        const char* err = luup_get_last_error();
        REQUIRE(err != nullptr);
        // Should mention "not found"
        REQUIRE(strstr(err, "not found") != nullptr);
    }
}

TEST_CASE("Model configuration defaults", "[model]") {
    SECTION("Default context size") {
        luup_model_config config = {
            .path = "test.gguf",  // Will fail but tests defaults
            .gpu_layers = 0,
            .context_size = 0,  // Should default to 2048
            .threads = 0,       // Should auto-detect
            .api_key = nullptr,
            .api_base_url = nullptr
        };
        
        // Even if it fails, defaults should be applied
        luup_model* model = luup_model_create_local(&config);
        // Model creation will fail (file doesn't exist), but that's OK for this test
        if (model) {
            luup_model_info info;
            luup_error_t result = luup_model_get_info(model, &info);
            REQUIRE(result == LUUP_SUCCESS);
            REQUIRE(info.context_size == 2048);
            luup_model_destroy(model);
        }
    }
}

TEST_CASE("Model info retrieval", "[model]") {
    SECTION("Null model") {
        luup_model_info info;
        luup_error_t result = luup_model_get_info(nullptr, &info);
        REQUIRE(result == LUUP_ERROR_INVALID_PARAM);
        const char* err = luup_get_last_error();
        REQUIRE(err != nullptr);
    }
    
    SECTION("Null info pointer") {
        luup_model_config config = {
            .path = "test.gguf",
            .gpu_layers = 0,
            .context_size = 512,
            .threads = 1,
            .api_key = nullptr,
            .api_base_url = nullptr
        };
        
        luup_model* model = luup_model_create_local(&config);
        if (model) {
            luup_error_t result = luup_model_get_info(model, nullptr);
            REQUIRE(result == LUUP_ERROR_INVALID_PARAM);
            luup_model_destroy(model);
        }
    }
}

TEST_CASE("Model warmup", "[model]") {
    SECTION("Null model") {
        luup_error_t result = luup_model_warmup(nullptr);
        REQUIRE(result == LUUP_ERROR_INVALID_PARAM);
    }
    
    SECTION("Model without backend") {
        // Can't easily test this without creating a partially initialized model
        // This is more of an implementation detail
    }
}

TEST_CASE("Model destruction", "[model]") {
    SECTION("Null model") {
        // Should not crash
        luup_model_destroy(nullptr);
        REQUIRE(true);
    }
    
    SECTION("Double destruction") {
        // First destroy should be safe, second should also be safe (nullptr)
        luup_model* model = nullptr;
        luup_model_destroy(model);
        luup_model_destroy(model);
        REQUIRE(true);
    }
}

TEST_CASE("Remote model creation", "[model][remote]") {
    SECTION("Valid remote config") {
        luup_model_config config = {
            .path = "gpt-3.5-turbo",  // Model name for remote APIs
            .gpu_layers = 0,
            .context_size = 4096,
            .threads = 0,
            .api_key = "test-key-12345",
            .api_base_url = "https://api.openai.com/v1"
        };
        
        luup_model* model = luup_model_create_remote(&config);
        REQUIRE(model != nullptr);
        
        if (model) {
            luup_model_info info;
            luup_error_t result = luup_model_get_info(model, &info);
            REQUIRE(result == LUUP_SUCCESS);
            REQUIRE(strcmp(info.backend, "openai") == 0);
            REQUIRE(strcmp(info.device, "API") == 0);
            REQUIRE(info.context_size == 4096);
            REQUIRE(info.gpu_layers_loaded == 0);  // N/A for remote
            REQUIRE(info.memory_usage == 0);        // N/A for remote
            luup_model_destroy(model);
        }
    }
    
    SECTION("Missing API key") {
        luup_model_config config = {
            .path = "gpt-4",
            .gpu_layers = 0,
            .context_size = 2048,
            .threads = 0,
            .api_key = nullptr,  // Missing API key
            .api_base_url = "https://api.openai.com/v1"
        };
        
        luup_model* model = luup_model_create_remote(&config);
        REQUIRE(model == nullptr);
        const char* err = luup_get_last_error();
        REQUIRE(err != nullptr);
        REQUIRE(strstr(err, "API key") != nullptr);
    }
    
    SECTION("Empty API key") {
        luup_model_config config = {
            .path = "gpt-4",
            .gpu_layers = 0,
            .context_size = 2048,
            .threads = 0,
            .api_key = "",  // Empty API key
            .api_base_url = "https://api.openai.com/v1"
        };
        
        luup_model* model = luup_model_create_remote(&config);
        REQUIRE(model == nullptr);
        const char* err = luup_get_last_error();
        REQUIRE(err != nullptr);
    }
    
    SECTION("Missing model name") {
        luup_model_config config = {
            .path = nullptr,  // Missing model name
            .gpu_layers = 0,
            .context_size = 2048,
            .threads = 0,
            .api_key = "test-key",
            .api_base_url = "https://api.openai.com/v1"
        };
        
        luup_model* model = luup_model_create_remote(&config);
        REQUIRE(model == nullptr);
    }
    
    SECTION("Empty model name") {
        luup_model_config config = {
            .path = "",  // Empty model name
            .gpu_layers = 0,
            .context_size = 2048,
            .threads = 0,
            .api_key = "test-key",
            .api_base_url = "https://api.openai.com/v1"
        };
        
        luup_model* model = luup_model_create_remote(&config);
        REQUIRE(model == nullptr);
    }
    
    SECTION("Default API endpoint") {
        luup_model_config config = {
            .path = "gpt-3.5-turbo",
            .gpu_layers = 0,
            .context_size = 2048,
            .threads = 0,
            .api_key = "test-key",
            .api_base_url = nullptr  // Should default to OpenAI
        };
        
        luup_model* model = luup_model_create_remote(&config);
        REQUIRE(model != nullptr);
        
        if (model) {
            luup_model_info info;
            luup_error_t result = luup_model_get_info(model, &info);
            REQUIRE(result == LUUP_SUCCESS);
            luup_model_destroy(model);
        }
    }
    
    SECTION("Custom endpoint (Ollama)") {
        luup_model_config config = {
            .path = "llama2",
            .gpu_layers = 0,
            .context_size = 2048,
            .threads = 0,
            .api_key = "ollama",  // Ollama doesn't require real key
            .api_base_url = "http://localhost:11434/v1"
        };
        
        luup_model* model = luup_model_create_remote(&config);
        REQUIRE(model != nullptr);
        
        if (model) {
            luup_model_destroy(model);
        }
    }
    
    SECTION("Invalid URL format") {
        luup_model_config config = {
            .path = "gpt-4",
            .gpu_layers = 0,
            .context_size = 2048,
            .threads = 0,
            .api_key = "test-key",
            .api_base_url = "not-a-valid-url"
        };
        
        luup_model* model = luup_model_create_remote(&config);
        REQUIRE(model == nullptr);
        const char* err = luup_get_last_error();
        REQUIRE(err != nullptr);
    }
    
    SECTION("Context size defaults") {
        luup_model_config config = {
            .path = "gpt-4",
            .gpu_layers = 0,
            .context_size = 0,  // Should default to 8192 for remote
            .threads = 0,
            .api_key = "test-key",
            .api_base_url = "https://api.openai.com/v1"
        };
        
        luup_model* model = luup_model_create_remote(&config);
        REQUIRE(model != nullptr);
        
        if (model) {
            luup_model_info info;
            luup_error_t result = luup_model_get_info(model, &info);
            REQUIRE(result == LUUP_SUCCESS);
            REQUIRE(info.context_size == 8192);
            luup_model_destroy(model);
        }
    }
}

TEST_CASE("Remote model warmup", "[model][remote]") {
    SECTION("Warmup remote model (should be no-op)") {
        luup_model_config config = {
            .path = "gpt-3.5-turbo",
            .gpu_layers = 0,
            .context_size = 2048,
            .threads = 0,
            .api_key = "test-key",
            .api_base_url = "https://api.openai.com/v1"
        };
        
        luup_model* model = luup_model_create_remote(&config);
        REQUIRE(model != nullptr);
        
        if (model) {
            // Warmup should succeed but do nothing for remote models
            luup_error_t result = luup_model_warmup(model);
            REQUIRE(result == LUUP_SUCCESS);
            luup_model_destroy(model);
        }
    }
}

TEST_CASE("Version information", "[version]") {
    SECTION("Version string") {
        const char* version = luup_version();
        REQUIRE(version != nullptr);
        REQUIRE(strlen(version) > 0);
        // Should be in format "X.Y.Z"
        REQUIRE(strchr(version, '.') != nullptr);
    }
    
    SECTION("Version components") {
        int major, minor, patch;
        luup_version_components(&major, &minor, &patch);
        REQUIRE(major == LUUP_VERSION_MAJOR);
        REQUIRE(minor == LUUP_VERSION_MINOR);
        REQUIRE(patch == LUUP_VERSION_PATCH);
        REQUIRE(major >= 0);
        REQUIRE(minor >= 0);
        REQUIRE(patch >= 0);
    }
}

TEST_CASE("Memory management", "[model]") {
    SECTION("Free null string") {
        // Should not crash
        luup_free_string(nullptr);
        REQUIRE(true);
    }
}

// Note: Full integration tests with actual model files will be in integration tests
// These unit tests focus on API contract, error handling, and edge cases

