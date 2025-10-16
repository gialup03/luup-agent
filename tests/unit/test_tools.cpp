/**
 * @file test_tools.cpp
 * @brief Unit tests for tool calling system
 */

#include <catch2/catch_test_macros.hpp>
#include <luup_agent.h>
#include <string>
#include <cstring>

// Helper to create a minimal model config for testing
// Returns nullptr if model file doesn't exist (tests will skip)
static luup_model* create_test_model() {
    const char* test_paths[] = {
        "models/qwen2-0.5b-instruct-q4_k_m.gguf",
        "../models/qwen2-0.5b-instruct-q4_k_m.gguf",
        "../../models/qwen2-0.5b-instruct-q4_k_m.gguf",
        nullptr
    };
    
    // Try each path
    for (int i = 0; test_paths[i] != nullptr; i++) {
        luup_model_config config = {
            .path = test_paths[i],
            .gpu_layers = 0,  // CPU only for tests
            .context_size = 512,
            .threads = 1,
            .api_key = nullptr,
            .api_base_url = nullptr
        };
        
        luup_model* model = luup_model_create_local(&config);
        if (model) {
            return model;
        }
    }
    
    return nullptr;
}

TEST_CASE("Built-in tools opt-out design", "[tools][builtin]") {
    auto model = create_test_model();
    if (!model) {
        SKIP("Model file not found - skipping test");
    }
    
    SECTION("Tools enabled by default") {
        luup_agent_config config = {
            .model = model,
            .system_prompt = "Test agent",
            .temperature = 0.7f,
            .max_tokens = 100,
            .enable_tool_calling = true,
            .enable_history_management = true,
            .enable_builtin_tools = true
        };
        
        auto agent = luup_agent_create(&config);
        REQUIRE(agent != nullptr);
        
        // Built-in tools should be registered automatically
        // We can't directly check registration, but agent creation should succeed
        
        luup_agent_destroy(agent);
    }
    
    SECTION("Tools can be disabled (opt-out)") {
        luup_agent_config config = {
            .model = model,
            .system_prompt = "Lightweight agent",
            .temperature = 0.7f,
            .max_tokens = 100,
            .enable_tool_calling = true,
            .enable_history_management = true,
            .enable_builtin_tools = false
        };
        
        auto agent = luup_agent_create(&config);
        REQUIRE(agent != nullptr);
        
        // Agent should be created without built-in tools
        
        luup_agent_destroy(agent);
    }
    
    luup_model_destroy(model);
}

TEST_CASE("Built-in todo tool", "[tools][builtin][todo]") {
    auto model = create_test_model();
    if (!model) {
        SKIP("Model file not found - skipping test");
    }
    
    luup_agent_config config = {
        .model = model,
        .system_prompt = "Test agent",
        .temperature = 0.7f,
        .max_tokens = 100,
        .enable_tool_calling = true,
        .enable_history_management = true,
        .enable_builtin_tools = false  // Manual registration
    };
    
    auto agent = luup_agent_create(&config);
    REQUIRE(agent != nullptr);
    
    // Manually enable todo tool
    REQUIRE(luup_agent_enable_builtin_todo(agent, nullptr) == LUUP_SUCCESS);
    
    // Tool is now registered and can be used via agent generation
    // Detailed testing would require actual model inference
    
    luup_agent_destroy(agent);
    luup_model_destroy(model);
}

TEST_CASE("Built-in notes tool", "[tools][builtin][notes]") {
    auto model = create_test_model();
    if (!model) {
        SKIP("Model file not found - skipping test");
    }
    
    luup_agent_config config = {
        .model = model,
        .system_prompt = "Test agent",
        .temperature = 0.7f,
        .max_tokens = 100,
        .enable_tool_calling = true,
        .enable_history_management = true,
        .enable_builtin_tools = false  // Manual registration
    };
    
    auto agent = luup_agent_create(&config);
    REQUIRE(agent != nullptr);
    
    // Manually enable notes tool
    REQUIRE(luup_agent_enable_builtin_notes(agent, nullptr) == LUUP_SUCCESS);
    
    // Tool is now registered and can be used via agent generation
    
    luup_agent_destroy(agent);
    luup_model_destroy(model);
}

TEST_CASE("Built-in summarization tool", "[tools][builtin][summarization]") {
    auto model = create_test_model();
    if (!model) {
        SKIP("Model file not found - skipping test");
    }
    
    luup_agent_config config = {
        .model = model,
        .system_prompt = "Test agent",
        .temperature = 0.7f,
        .max_tokens = 100,
        .enable_tool_calling = true,
        .enable_history_management = true,
        .enable_builtin_tools = false  // Manual registration
    };
    
    auto agent = luup_agent_create(&config);
    REQUIRE(agent != nullptr);
    
    // Manually enable summarization
    REQUIRE(luup_agent_enable_builtin_summarization(agent) == LUUP_SUCCESS);
    
    // Summarization is now active
    
    luup_agent_destroy(agent);
    luup_model_destroy(model);
}

TEST_CASE("Built-in tools with persistent storage", "[tools][builtin][storage]") {
    auto model = create_test_model();
    if (!model) {
        SKIP("Model file not found - skipping test");
    }
    
    luup_agent_config config = {
        .model = model,
        .system_prompt = "Test agent",
        .temperature = 0.7f,
        .max_tokens = 100,
        .enable_tool_calling = true,
        .enable_history_management = true,
        .enable_builtin_tools = false
    };
    
    auto agent = luup_agent_create(&config);
    REQUIRE(agent != nullptr);
    
    // Enable with file storage
    REQUIRE(luup_agent_enable_builtin_todo(agent, "/tmp/test_todos.json") == LUUP_SUCCESS);
    REQUIRE(luup_agent_enable_builtin_notes(agent, "/tmp/test_notes.json") == LUUP_SUCCESS);
    
    // Storage files would be created on first write
    
    luup_agent_destroy(agent);
    luup_model_destroy(model);
}

