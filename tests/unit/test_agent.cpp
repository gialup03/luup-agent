/**
 * @file test_agent.cpp
 * @brief Unit tests for agent layer
 */

#include <catch2/catch_test_macros.hpp>
#include <luup_agent.h>
#include <string>
#include <cstring>

// Helper function to create a mock model for testing
static luup_model* create_mock_model() {
    luup_model_config config = {
        .path = "test_model.gguf",  // Will fail but that's okay for testing
        .gpu_layers = 0,
        .context_size = 512,
        .threads = 1,
        .api_key = nullptr,
        .api_base_url = nullptr
    };
    
    // Note: This will return nullptr since the model doesn't exist
    // For real tests with a model, you'd need an actual GGUF file
    return luup_model_create_local(&config);
}

TEST_CASE("Agent creation with invalid config", "[agent]") {
    SECTION("Null config") {
        luup_agent* agent = luup_agent_create(nullptr);
        REQUIRE(agent == nullptr);
        REQUIRE(luup_get_last_error() != nullptr);
    }
    
    SECTION("Config with null model") {
        luup_agent_config config = {
            .model = nullptr,
            .system_prompt = "Test",
            .temperature = 0.7f,
            .max_tokens = 100,
            .enable_tool_calling = true,
            .enable_history_management = true
        };
        
        luup_agent* agent = luup_agent_create(&config);
        REQUIRE(agent == nullptr);
        REQUIRE(luup_get_last_error() != nullptr);
    }
}

TEST_CASE("Agent creation with valid config", "[agent]") {
    // Create a dummy model pointer (opaque type, can't allocate on stack)
    // For real tests, you'd use luup_model_create_local with actual model file
    luup_model* dummy_model = reinterpret_cast<luup_model*>(0x1);  // Non-null dummy pointer
    
    luup_agent_config config = {
        .model = dummy_model,
        .system_prompt = "You are a helpful assistant.",
        .temperature = 0.8f,
        .max_tokens = 256,
        .enable_tool_calling = true,
        .enable_history_management = true
    };
    
    luup_agent* agent = luup_agent_create(&config);
    REQUIRE(agent != nullptr);
    
    // Cleanup
    luup_agent_destroy(agent);
}

TEST_CASE("Agent history management", "[agent]") {
    SECTION("Add message with null agent") {
        luup_error_t result = luup_agent_add_message(nullptr, "user", "test");
        REQUIRE(result == LUUP_ERROR_INVALID_PARAM);
    }
    
    SECTION("Clear history with null agent") {
        luup_error_t result = luup_agent_clear_history(nullptr);
        REQUIRE(result == LUUP_ERROR_INVALID_PARAM);
    }
    
    SECTION("Get history with null agent") {
        char* history = luup_agent_get_history_json(nullptr);
        REQUIRE(history == nullptr);
    }
    
    SECTION("Add and retrieve messages") {
        luup_model* dummy_model = reinterpret_cast<luup_model*>(0x1);
        luup_agent_config config = {
            .model = dummy_model,
            .system_prompt = "Test system",
            .temperature = 0.7f,
            .max_tokens = 100,
            .enable_tool_calling = false,
            .enable_history_management = true
        };
        
        luup_agent* agent = luup_agent_create(&config);
        REQUIRE(agent != nullptr);
        
        // Add user message
        luup_error_t result = luup_agent_add_message(agent, "user", "Hello");
        REQUIRE(result == LUUP_SUCCESS);
        
        // Add assistant message
        result = luup_agent_add_message(agent, "assistant", "Hi there!");
        REQUIRE(result == LUUP_SUCCESS);
        
        // Get history as JSON
        char* history_json = luup_agent_get_history_json(agent);
        REQUIRE(history_json != nullptr);
        
        // Check that JSON contains our messages
        std::string json_str(history_json);
        REQUIRE(json_str.find("Hello") != std::string::npos);
        REQUIRE(json_str.find("Hi there!") != std::string::npos);
        REQUIRE(json_str.find("Test system") != std::string::npos);
        
        luup_free_string(history_json);
        
        // Clear history
        result = luup_agent_clear_history(agent);
        REQUIRE(result == LUUP_SUCCESS);
        
        // Get history again - should only have system prompt
        history_json = luup_agent_get_history_json(agent);
        REQUIRE(history_json != nullptr);
        json_str = std::string(history_json);
        REQUIRE(json_str.find("Hello") == std::string::npos);
        REQUIRE(json_str.find("Test system") != std::string::npos);
        
        luup_free_string(history_json);
        luup_agent_destroy(agent);
    }
}

TEST_CASE("Agent tool registration", "[agent]") {
    SECTION("Register tool with null agent") {
        luup_tool tool = {
            .name = "test_tool",
            .description = "Test",
            .parameters_json = "{}"
        };
        
        auto callback = [](const char* params, void* data) -> char* {
            return nullptr;
        };
        
        luup_error_t result = luup_agent_register_tool(nullptr, &tool, callback, nullptr);
        REQUIRE(result == LUUP_ERROR_INVALID_PARAM);
    }
    
    SECTION("Register valid tool") {
        luup_model* dummy_model = reinterpret_cast<luup_model*>(0x1);
        luup_agent_config config = {
            .model = dummy_model,
            .system_prompt = "Test",
            .temperature = 0.7f,
            .max_tokens = 100,
            .enable_tool_calling = true,
            .enable_history_management = true
        };
        
        luup_agent* agent = luup_agent_create(&config);
        REQUIRE(agent != nullptr);
        
        luup_tool tool = {
            .name = "get_weather",
            .description = "Get current weather for a city",
            .parameters_json = R"({"type": "object", "properties": {"city": {"type": "string"}}})"
        };
        
        int callback_called = 0;
        auto callback = [](const char* params, void* data) -> char* {
            int* count = static_cast<int*>(data);
            (*count)++;
            char* result = static_cast<char*>(malloc(50));
            strcpy(result, R"({"temperature": 72, "condition": "sunny"})");
            return result;
        };
        
        luup_error_t result = luup_agent_register_tool(agent, &tool, callback, &callback_called);
        REQUIRE(result == LUUP_SUCCESS);
        
        luup_agent_destroy(agent);
    }
}

TEST_CASE("Agent generation", "[agent]") {
    SECTION("Generate with null agent") {
        char* response = luup_agent_generate(nullptr, "test");
        REQUIRE(response == nullptr);
        REQUIRE(luup_get_last_error() != nullptr);
    }
    
    SECTION("Generate with null message") {
        luup_model* dummy_model = reinterpret_cast<luup_model*>(0x1);
        luup_agent_config config = {
            .model = dummy_model,
            .system_prompt = "Test",
            .temperature = 0.7f,
            .max_tokens = 100,
            .enable_tool_calling = false,
            .enable_history_management = true
        };
        
        luup_agent* agent = luup_agent_create(&config);
        REQUIRE(agent != nullptr);
        
        char* response = luup_agent_generate(agent, nullptr);
        REQUIRE(response == nullptr);
        
        luup_agent_destroy(agent);
    }
    
    // Note: Full generation tests require a valid model
    // These would be integration tests with an actual GGUF file
}

TEST_CASE("Agent destruction", "[agent]") {
    SECTION("Null agent") {
        // Should not crash
        luup_agent_destroy(nullptr);
        REQUIRE(true);
    }
}

TEST_CASE("Built-in tools", "[agent][tools]") {
    SECTION("Enable todo with null agent") {
        luup_error_t result = luup_agent_enable_builtin_todo(nullptr, nullptr);
        REQUIRE(result == LUUP_ERROR_INVALID_PARAM);
    }
    
    SECTION("Enable notes with null agent") {
        luup_error_t result = luup_agent_enable_builtin_notes(nullptr, nullptr);
        REQUIRE(result == LUUP_ERROR_INVALID_PARAM);
    }
    
    SECTION("Enable summarization with null agent") {
        luup_error_t result = luup_agent_enable_builtin_summarization(nullptr);
        REQUIRE(result == LUUP_ERROR_INVALID_PARAM);
    }
}

