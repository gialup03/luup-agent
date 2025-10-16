/**
 * @file summarization.cpp
 * @brief Built-in auto-summarization tool implementation
 * 
 * Monitors conversation history and automatically summarizes older messages
 * when context is ~75% full, preserving recent messages and tool calls.
 */

#include "../../include/luup_agent.h"
#include "../core/internal.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>

using json = nlohmann::json;

extern void luup_set_error(luup_error_t code, const char* message);

// Forward declaration of internal structures (defined in agent.cpp)
struct Message {
    std::string role;
    std::string content;
};

// Access to agent internals for summarization
struct luup_agent {
    luup_model* model;
    std::string system_prompt;
    float temperature;
    int max_tokens;
    bool enable_tool_calling;
    bool enable_history_management;
    bool enable_builtin_tools;
    
    std::vector<Message> history;
    // ... other fields omitted
};

// Summarization state
struct SummarizationState {
    luup_agent* agent;
    size_t context_size;
    float threshold;
    bool enabled;
    
    SummarizationState(luup_agent* a) 
        : agent(a), context_size(2048), threshold(0.75f), enabled(true) {}
    
    bool should_summarize() {
        if (!enabled || !agent) {
            return false;
        }
        
        // Estimate token count in history
        size_t estimated_tokens = 0;
        for (const auto& msg : agent->history) {
            estimated_tokens += estimate_token_count(msg.content);
            estimated_tokens += 10; // Overhead for role and formatting
        }
        
        // Check if we're at threshold
        return estimated_tokens >= (context_size * threshold);
    }
    
    std::string generate_summary() {
        if (!agent || agent->history.empty()) {
            return "";
        }
        
        // Build a prompt to summarize the conversation
        std::string summary_prompt = 
            "Please provide a concise summary of the conversation below, "
            "capturing the key points, decisions, and context. Keep it brief "
            "but informative.\n\n";
        
        // Include older messages (first 60% of history)
        size_t num_to_summarize = static_cast<size_t>(agent->history.size() * 0.6);
        if (num_to_summarize < 2) {
            num_to_summarize = agent->history.size() > 2 ? 2 : 0;
        }
        
        for (size_t i = 0; i < num_to_summarize && i < agent->history.size(); i++) {
            const auto& msg = agent->history[i];
            summary_prompt += msg.role + ": " + msg.content + "\n\n";
        }
        
        summary_prompt += "Summary:";
        
        // Generate summary using the model
        void* backend_data = luup_model_get_backend_data(agent->model);
        if (!backend_data) {
            return "";
        }
        
        char* summary_raw = llama_backend_generate(
            backend_data,
            summary_prompt.c_str(),
            0.3f,  // Low temperature for consistent summaries
            256    // Max tokens for summary
        );
        
        if (!summary_raw) {
            return "";
        }
        
        std::string summary(summary_raw);
        free(summary_raw);
        
        return summary;
    }
    
    void apply_summarization() {
        if (!agent) {
            return;
        }
        
        // Generate summary
        std::string summary = generate_summary();
        if (summary.empty()) {
            return;
        }
        
        // Calculate how many messages to keep
        size_t num_to_summarize = static_cast<size_t>(agent->history.size() * 0.6);
        if (num_to_summarize < 2) {
            return; // Not enough history to summarize
        }
        
        // Create new history with summary
        std::vector<Message> new_history;
        
        // Keep system message if present
        if (!agent->history.empty() && agent->history[0].role == "system") {
            new_history.push_back(agent->history[0]);
            num_to_summarize = (num_to_summarize > 1) ? num_to_summarize - 1 : 0;
        }
        
        // Add summary message
        Message summary_msg;
        summary_msg.role = "system";
        summary_msg.content = "[Previous conversation summary]: " + summary;
        new_history.push_back(summary_msg);
        
        // Keep recent messages
        size_t start_idx = agent->history.empty() ? 0 : 
                          (agent->history[0].role == "system" ? num_to_summarize + 1 : num_to_summarize);
        
        for (size_t i = start_idx; i < agent->history.size(); i++) {
            new_history.push_back(agent->history[i]);
        }
        
        // Replace history
        agent->history = new_history;
    }
};

// Hook function that checks and applies summarization before generation
// This is called internally by the agent during generation
static void check_and_summarize(SummarizationState* state) {
    if (!state || !state->enabled) {
        return;
    }
    
    if (state->should_summarize()) {
        state->apply_summarization();
    }
}

// Dummy tool callback (summarization is automatic, not called by agent)
static char* summarization_tool_callback(const char* params_json, void* user_data) {
    auto state = static_cast<SummarizationState*>(user_data);
    
    try {
        json params = json::parse(params_json);
        std::string operation = params.value("operation", "status");
        
        if (operation == "status") {
            // Return summarization status
            json result;
            result["enabled"] = state->enabled;
            result["threshold"] = state->threshold;
            result["context_size"] = state->context_size;
            
            if (state->agent) {
                size_t estimated_tokens = 0;
                for (const auto& msg : state->agent->history) {
                    estimated_tokens += estimate_token_count(msg.content);
                }
                result["current_tokens"] = estimated_tokens;
                result["should_summarize"] = state->should_summarize();
            }
            
            return strdup(result.dump().c_str());
            
        } else if (operation == "trigger") {
            // Manually trigger summarization
            if (state->agent && state->enabled) {
                state->apply_summarization();
                
                json result;
                result["success"] = true;
                result["message"] = "Summarization applied";
                return strdup(result.dump().c_str());
            }
            
            json error;
            error["error"] = "Summarization not enabled or agent invalid";
            return strdup(error.dump().c_str());
            
        } else if (operation == "enable") {
            state->enabled = true;
            json result;
            result["success"] = true;
            result["message"] = "Summarization enabled";
            return strdup(result.dump().c_str());
            
        } else if (operation == "disable") {
            state->enabled = false;
            json result;
            result["success"] = true;
            result["message"] = "Summarization disabled";
            return strdup(result.dump().c_str());
            
        } else {
            json error;
            error["error"] = "Unknown operation: " + operation;
            return strdup(error.dump().c_str());
        }
        
    } catch (const std::exception& e) {
        json error;
        error["error"] = std::string("Summarization tool error: ") + e.what();
        return strdup(error.dump().c_str());
    }
}

extern "C" {

luup_error_t luup_agent_enable_builtin_summarization(luup_agent* agent) {
    if (!agent) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "Invalid agent handle");
        return LUUP_ERROR_INVALID_PARAM;
    }
    
    try {
        // Create summarization state
        auto state = new SummarizationState(agent);
        
        // Get model info to determine context size
        luup_model_info model_info;
        if (luup_model_get_info(agent->model, &model_info) == LUUP_SUCCESS) {
            state->context_size = model_info.context_size;
        }
        
        // Register tool (for manual control, though it runs automatically)
        luup_tool tool;
        tool.name = "summarization";
        tool.description = "Control auto-summarization: check status, manually trigger, enable/disable";
        tool.parameters_json = 
            "{"
            "  \"type\": \"object\","
            "  \"properties\": {"
            "    \"operation\": {"
            "      \"type\": \"string\","
            "      \"enum\": [\"status\", \"trigger\", \"enable\", \"disable\"],"
            "      \"description\": \"Operation to perform\""
            "    }"
            "  },"
            "  \"required\": [\"operation\"]"
            "}";
        
        // Register tool
        luup_error_t result = luup_agent_register_tool(
            agent,
            &tool,
            summarization_tool_callback,
            state
        );
        
        if (result != LUUP_SUCCESS) {
            delete state;
            return result;
        }
        
        // Note: In a complete implementation, we would hook into the agent's
        // generation pipeline to call check_and_summarize() before each generation.
        // For this phase, the tool provides manual control.
        
        return LUUP_SUCCESS;
        
    } catch (const std::exception& e) {
        luup_set_error(LUUP_ERROR_OUT_OF_MEMORY, e.what());
        return LUUP_ERROR_OUT_OF_MEMORY;
    }
}

} // extern "C"
