/**
 * @file agent.cpp
 * @brief Agent layer implementation
 */

#include "../../include/luup_agent.h"
#include "internal.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

extern void luup_set_error(luup_error_t code, const char* message);

// Tool registration info
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

// Message structure
struct Message {
    std::string role;
    std::string content;
};

// Internal agent structure
struct luup_agent {
    luup_model* model;
    std::string system_prompt;
    float temperature;
    int max_tokens;
    bool enable_tool_calling;
    bool enable_history_management;
    
    std::vector<Message> history;
    std::map<std::string, ToolInfo> tools;
    
    luup_agent() : model(nullptr), temperature(0.7f), max_tokens(0),
                   enable_tool_calling(true), enable_history_management(true) {}
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
        
        // Add system message to history if provided
        if (!agent->system_prompt.empty()) {
            Message msg;
            msg.role = "system";
            msg.content = agent->system_prompt;
            agent->history.push_back(msg);
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

luup_error_t luup_agent_generate_stream(
    luup_agent* agent,
    const char* user_message,
    luup_stream_callback_t callback,
    void* user_data)
{
    if (!agent || !user_message || !callback) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "Invalid parameters for generation");
        return LUUP_ERROR_INVALID_PARAM;
    }
    
    // TODO: Implement streaming generation
    // This will be implemented in Phase 2
    
    luup_set_error(LUUP_ERROR_INFERENCE_FAILED, "Not yet implemented");
    return LUUP_ERROR_INFERENCE_FAILED;
}

char* luup_agent_generate(luup_agent* agent, const char* user_message) {
    if (!agent || !user_message) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "Invalid parameters for generation");
        return nullptr;
    }
    
    // TODO: Implement blocking generation
    // This will be implemented in Phase 2
    
    luup_set_error(LUUP_ERROR_INFERENCE_FAILED, "Not yet implemented");
    return nullptr;
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
    
    // TODO: Implement JSON serialization
    // This will be implemented in Phase 2
    
    luup_set_error(LUUP_ERROR_INFERENCE_FAILED, "Not yet implemented");
    return nullptr;
}

luup_error_t luup_agent_enable_builtin_todo(
    luup_agent* agent,
    const char* storage_path)
{
    if (!agent) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "Invalid agent handle");
        return LUUP_ERROR_INVALID_PARAM;
    }
    
    // TODO: Implement built-in todo tool
    // This will be implemented in Phase 3
    
    luup_set_error(LUUP_ERROR_INFERENCE_FAILED, "Not yet implemented");
    return LUUP_ERROR_INFERENCE_FAILED;
}

luup_error_t luup_agent_enable_builtin_notes(
    luup_agent* agent,
    const char* storage_path)
{
    if (!agent) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "Invalid agent handle");
        return LUUP_ERROR_INVALID_PARAM;
    }
    
    // TODO: Implement built-in notes tool
    // This will be implemented in Phase 3
    
    luup_set_error(LUUP_ERROR_INFERENCE_FAILED, "Not yet implemented");
    return LUUP_ERROR_INFERENCE_FAILED;
}

luup_error_t luup_agent_enable_builtin_summarization(luup_agent* agent) {
    if (!agent) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "Invalid agent handle");
        return LUUP_ERROR_INVALID_PARAM;
    }
    
    // TODO: Implement built-in summarization
    // This will be implemented in Phase 3
    
    luup_set_error(LUUP_ERROR_INFERENCE_FAILED, "Not yet implemented");
    return LUUP_ERROR_INFERENCE_FAILED;
}

void luup_agent_destroy(luup_agent* agent) {
    if (agent) {
        delete agent;
    }
}

} // extern "C"

