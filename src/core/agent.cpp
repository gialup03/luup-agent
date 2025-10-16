/**
 * @file agent.cpp
 * @brief Agent layer implementation
 */

#include "../../include/luup_agent.h"
#include "internal.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstring>
#include <cstdlib>

using json = nlohmann::json;

extern void luup_set_error(luup_error_t code, const char* message);

// Tool registration info (matches tool_calling.cpp)
struct ToolInfo {
    luup_tool tool;
    luup_tool_callback_t callback;
    void* user_data;
    
    ToolInfo() : callback(nullptr), user_data(nullptr) {
        tool.name = nullptr;
        tool.description = nullptr;
        tool.parameters_json = nullptr;
    }
};

// Message structure (matches context_manager.cpp)
struct Message {
    std::string role;
    std::string content;
};

// Tool call structure (matches tool_calling.cpp)
struct ToolCall {
    std::string tool_name;
    std::string parameters_json;
};

// Internal agent structure
struct luup_agent {
    luup_model* model;
    std::string system_prompt;
    float temperature;
    int max_tokens;
    bool enable_tool_calling;
    bool enable_history_management;
    bool enable_builtin_tools;
    
    std::vector<Message> history;
    std::map<std::string, ToolInfo> tools;
    
    luup_agent() : model(nullptr), temperature(0.7f), max_tokens(0),
                   enable_tool_calling(true), enable_history_management(true),
                   enable_builtin_tools(true) {}
};

extern "C" {

luup_agent* luup_agent_create(const luup_agent_config* config) {
    if (!config || !config->model) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "Invalid agent configuration");
        return nullptr;
    }
    
    try {
        auto agent = new luup_agent();
        agent->model = config->model;
        agent->system_prompt = config->system_prompt ? config->system_prompt : "";
        agent->temperature = config->temperature;
        agent->max_tokens = config->max_tokens;
        agent->enable_tool_calling = config->enable_tool_calling;
        agent->enable_history_management = config->enable_history_management;
        agent->enable_builtin_tools = config->enable_builtin_tools;
        
        // Add system message to history if provided
        if (!agent->system_prompt.empty()) {
            Message msg;
            msg.role = "system";
            msg.content = agent->system_prompt;
            agent->history.push_back(msg);
        }
        
        // Auto-register built-in tools if enabled (opt-out design)
        if (agent->enable_builtin_tools) {
            // Register todo list tool (in-memory only)
            luup_agent_enable_builtin_todo(agent, nullptr);
            
            // Register notes tool (in-memory only)
            luup_agent_enable_builtin_notes(agent, nullptr);
            
            // Register auto-summarization
            luup_agent_enable_builtin_summarization(agent);
        }
        
        return agent;
    } catch (const std::exception& e) {
        luup_set_error(LUUP_ERROR_OUT_OF_MEMORY, e.what());
        return nullptr;
    }
}

luup_error_t luup_agent_register_tool(
    luup_agent* agent,
    const luup_tool* tool,
    luup_tool_callback_t callback,
    void* user_data)
{
    if (!agent || !tool || !tool->name || !callback) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "Invalid parameters for tool registration");
        return LUUP_ERROR_INVALID_PARAM;
    }
    
    try {
        ToolInfo info;
        info.tool = *tool;
        info.callback = callback;
        info.user_data = user_data;
        
        agent->tools[tool->name] = info;
        
        return LUUP_SUCCESS;
    } catch (const std::exception& e) {
        luup_set_error(LUUP_ERROR_OUT_OF_MEMORY, e.what());
        return LUUP_ERROR_OUT_OF_MEMORY;
    }
}

// Internal helper to avoid duplicate history addition
static luup_error_t luup_agent_generate_stream_internal(
    luup_agent* agent,
    const char* user_message,
    luup_stream_callback_t callback,
    void* user_data,
    bool add_to_history)
{
    if (!agent || !user_message || !callback) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "Invalid parameters for generation");
        return LUUP_ERROR_INVALID_PARAM;
    }
    
    try {
        // Add user message to history if requested
        if (add_to_history && agent->enable_history_management) {
            Message msg;
            msg.role = "user";
            msg.content = user_message;
            agent->history.push_back(msg);
        }
        
        // Build the prompt from conversation history
        std::string prompt;
        if (agent->enable_history_management) {
            prompt = format_chat_history(agent->history);
        } else {
            // No history - use ChatML format
            if (!agent->system_prompt.empty()) {
                prompt = "<|im_start|>system\n" + agent->system_prompt + "<|im_end|>\n" +
                         "<|im_start|>user\n" + std::string(user_message) + "<|im_end|>\n" +
                         "<|im_start|>assistant\n";
            } else {
                prompt = "<|im_start|>user\n" + std::string(user_message) + "<|im_end|>\n" +
                         "<|im_start|>assistant\n";
            }
        }
        
        // Add tool schema if tools are registered and enabled
        if (agent->enable_tool_calling && !agent->tools.empty()) {
            std::string tool_schema = generate_tool_schema(agent->tools);
            // Insert tool schema right after system message (before conversation)
            size_t insert_pos = prompt.find("<|im_end|>");
            if (insert_pos != std::string::npos) {
                // Insert after the first <|im_end|> (end of system message)
                prompt.insert(insert_pos + 11, tool_schema);  // 11 = length of "<|im_end|>\n"
            }
        }
        
        // For now, use blocking generation and simulate streaming
        // TODO: Implement true token-by-token streaming in future
        void* backend_data = luup_model_get_backend_data(agent->model);
        if (!backend_data) {
            luup_set_error(LUUP_ERROR_INVALID_PARAM, "Model backend not initialized");
            return LUUP_ERROR_INVALID_PARAM;
        }
        
        char* response_raw = llama_backend_generate(
            backend_data,
            prompt.c_str(),
            agent->temperature,
            agent->max_tokens
        );
        
        if (!response_raw) {
            return LUUP_ERROR_INFERENCE_FAILED;
        }
        
        std::string response(response_raw);
        free(response_raw);
        
        // Check for tool calls if enabled
        if (agent->enable_tool_calling && !agent->tools.empty()) {
            std::vector<ToolCall> tool_calls = parse_tool_calls(response);
            
            if (!tool_calls.empty()) {
                // Execute tool calls
                std::string tool_results;
                for (const auto& tc : tool_calls) {
                    std::string result = execute_tool(tc.tool_name, tc.parameters_json, agent->tools);
                    tool_results += format_tool_result(tc.tool_name, result) + "\n";
                }
                
                // Add tool results to history and regenerate
                if (agent->enable_history_management) {
                    Message assistant_msg;
                    assistant_msg.role = "assistant";
                    assistant_msg.content = response;
                    agent->history.push_back(assistant_msg);
                    
                    Message tool_msg;
                    tool_msg.role = "user";
                    tool_msg.content = tool_results;
                    agent->history.push_back(tool_msg);
                }
                
                // Recursively generate final response (don't re-add to history)
                // (In real implementation, you'd want a max recursion depth)
                return luup_agent_generate_stream_internal(agent, tool_results.c_str(), callback, user_data, false);
            }
        }
        
        // Simulate streaming by calling callback with full response
        callback(response.c_str(), user_data);
        
        // Add assistant response to history
        if (agent->enable_history_management) {
            Message msg;
            msg.role = "assistant";
            msg.content = response;
            agent->history.push_back(msg);
        }
        
        return LUUP_SUCCESS;
        
    } catch (const std::exception& e) {
        luup_set_error(LUUP_ERROR_INFERENCE_FAILED, e.what());
        return LUUP_ERROR_INFERENCE_FAILED;
    }
}

// Public API wrapper
luup_error_t luup_agent_generate_stream(
    luup_agent* agent,
    const char* user_message,
    luup_stream_callback_t callback,
    void* user_data)
{
    return luup_agent_generate_stream_internal(agent, user_message, callback, user_data, true);
}

// Internal helper to avoid duplicate history addition
static char* luup_agent_generate_internal(luup_agent* agent, const char* user_message, bool add_to_history) {
    if (!agent || !user_message) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "Invalid parameters for generation");
        return nullptr;
    }
    
    try {
        // Add user message to history if requested
        if (add_to_history && agent->enable_history_management) {
            Message msg;
            msg.role = "user";
            msg.content = user_message;
            agent->history.push_back(msg);
        }
        
        // Build the prompt from conversation history
        std::string prompt;
        if (agent->enable_history_management) {
            prompt = format_chat_history(agent->history);
        } else {
            // No history - use ChatML format
            if (!agent->system_prompt.empty()) {
                prompt = "<|im_start|>system\n" + agent->system_prompt + "<|im_end|>\n" +
                         "<|im_start|>user\n" + std::string(user_message) + "<|im_end|>\n" +
                         "<|im_start|>assistant\n";
            } else {
                prompt = "<|im_start|>user\n" + std::string(user_message) + "<|im_end|>\n" +
                         "<|im_start|>assistant\n";
            }
        }
        
        // Add tool schema if tools are registered and enabled
        if (agent->enable_tool_calling && !agent->tools.empty()) {
            std::string tool_schema = generate_tool_schema(agent->tools);
            // Insert tool schema right after system message (before conversation)
            size_t insert_pos = prompt.find("<|im_end|>");
            if (insert_pos != std::string::npos) {
                // Insert after the first <|im_end|> (end of system message)
                prompt.insert(insert_pos + 11, tool_schema);  // 11 = length of "<|im_end|>\n"
            }
        }
        
        // Generate response
        void* backend_data = luup_model_get_backend_data(agent->model);
        if (!backend_data) {
            luup_set_error(LUUP_ERROR_INVALID_PARAM, "Model backend not initialized");
            return nullptr;
        }
        
        char* response_raw = llama_backend_generate(
            backend_data,
            prompt.c_str(),
            agent->temperature,
            agent->max_tokens
        );
        
        if (!response_raw) {
            return nullptr;
        }
        
        std::string response(response_raw);
        free(response_raw);
        
        // Check for tool calls if enabled
        if (agent->enable_tool_calling && !agent->tools.empty()) {
            std::vector<ToolCall> tool_calls = parse_tool_calls(response);
            
            if (!tool_calls.empty()) {
                // Execute tool calls
                std::string tool_results;
                for (const auto& tc : tool_calls) {
                    std::string result = execute_tool(tc.tool_name, tc.parameters_json, agent->tools);
                    tool_results += format_tool_result(tc.tool_name, result) + "\n";
                }
                
                // Add tool results to history and regenerate
                if (agent->enable_history_management) {
                    Message assistant_msg;
                    assistant_msg.role = "assistant";
                    assistant_msg.content = response;
                    agent->history.push_back(assistant_msg);
                    
                    Message tool_msg;
                    tool_msg.role = "user";
                    tool_msg.content = tool_results;
                    agent->history.push_back(tool_msg);
                }
                
                // Recursively generate final response (don't re-add to history)
                // (In real implementation, you'd want a max recursion depth)
                return luup_agent_generate_internal(agent, tool_results.c_str(), false);
            }
        }
        
        // Add assistant response to history
        if (agent->enable_history_management) {
            Message msg;
            msg.role = "assistant";
            msg.content = response;
            agent->history.push_back(msg);
        }
        
        // Allocate and return result
        char* result = static_cast<char*>(malloc(response.size() + 1));
        if (result) {
            memcpy(result, response.c_str(), response.size());
            result[response.size()] = '\0';
        }
        
        return result;
        
    } catch (const std::exception& e) {
        luup_set_error(LUUP_ERROR_INFERENCE_FAILED, e.what());
        return nullptr;
    }
}

// Public API wrapper
char* luup_agent_generate(luup_agent* agent, const char* user_message) {
    return luup_agent_generate_internal(agent, user_message, true);
}

luup_error_t luup_agent_add_message(
    luup_agent* agent,
    const char* role,
    const char* content)
{
    if (!agent || !role || !content) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "Invalid parameters");
        return LUUP_ERROR_INVALID_PARAM;
    }
    
    try {
        Message msg;
        msg.role = role;
        msg.content = content;
        agent->history.push_back(msg);
        
        return LUUP_SUCCESS;
    } catch (const std::exception& e) {
        luup_set_error(LUUP_ERROR_OUT_OF_MEMORY, e.what());
        return LUUP_ERROR_OUT_OF_MEMORY;
    }
}

luup_error_t luup_agent_clear_history(luup_agent* agent) {
    if (!agent) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "Invalid agent handle");
        return LUUP_ERROR_INVALID_PARAM;
    }
    
    agent->history.clear();
    
    // Re-add system prompt if present
    if (!agent->system_prompt.empty()) {
        Message msg;
        msg.role = "system";
        msg.content = agent->system_prompt;
        agent->history.push_back(msg);
    }
    
    return LUUP_SUCCESS;
}

char* luup_agent_get_history_json(luup_agent* agent) {
    if (!agent) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "Invalid agent handle");
        return nullptr;
    }
    
    try {
        json history_json = json::array();
        
        for (const auto& msg : agent->history) {
            json msg_json;
            msg_json["role"] = msg.role;
            msg_json["content"] = msg.content;
            history_json.push_back(msg_json);
        }
        
        std::string json_str = history_json.dump(2);  // Pretty print with 2-space indent
        
        // Allocate and return result
        char* result = static_cast<char*>(malloc(json_str.size() + 1));
        if (result) {
            memcpy(result, json_str.c_str(), json_str.size());
            result[json_str.size()] = '\0';
        }
        
        return result;
        
    } catch (const std::exception& e) {
        luup_set_error(LUUP_ERROR_JSON_PARSE_FAILED, e.what());
        return nullptr;
    }
}

// Note: Built-in tool implementations are in src/builtin_tools/*.cpp
// These functions are declared extern "C" in those files

void luup_agent_destroy(luup_agent* agent) {
    if (agent) {
        delete agent;
    }
}

} // extern "C"

