/**
 * @file test_agent.cpp
 * @brief Unit tests for agent layer
 */

#include <catch2/catch_test_macros.hpp>
#include <luup_agent.h>

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

TEST_CASE("Agent history management", "[agent]") {
    // Note: These tests need a valid model first
    // Will be fully implemented in Phase 2
    
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
}

TEST_CASE("Agent generation", "[agent]") {
    SECTION("Generate with null agent") {
        char* response = luup_agent_generate(nullptr, "test");
        REQUIRE(response == nullptr);
        REQUIRE(luup_get_last_error() != nullptr);
    }
    
    SECTION("Generate with null message") {
        // Can't test without valid agent yet
        // Will be implemented in Phase 2
    }
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

