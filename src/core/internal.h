/**
 * @file internal.h
 * @brief Internal headers and utilities
 */

#ifndef LUUP_INTERNAL_H
#define LUUP_INTERNAL_H

#include "../../include/luup_agent.h"
#include <stddef.h>
#include <string>
#include <vector>
#include <map>

// Forward declarations for internal types
struct Message;
struct ToolInfo;
struct ToolCall;

// Error handling functions
extern void luup_set_error(luup_error_t code, const char* message);
extern void luup_clear_error();
extern luup_error_t luup_get_last_error_code();

// llama.cpp backend functions
extern void* llama_backend_init(const char* model_path, int gpu_layers, 
                                int context_size, int threads);
extern void llama_backend_free(void* backend_data);
extern bool llama_backend_get_info(void* backend_data, const char** device,
                                   int* gpu_layers, size_t* memory_usage);
extern bool llama_backend_warmup(void* backend_data);
extern char* llama_backend_generate(void* backend_data, const char* prompt,
                                    float temperature, int max_tokens);

// Model helper functions
extern void* luup_model_get_backend_data(luup_model* model);

// Context manager functions (from context_manager.cpp)
extern std::string format_chat_history(const std::vector<Message>& history);
extern size_t estimate_token_count(const std::string& text);
extern bool is_context_full(const std::vector<Message>& history, size_t context_size, float threshold);

// Tool calling functions (from tool_calling.cpp)
extern std::vector<ToolCall> parse_tool_calls(const std::string& text);
extern std::string execute_tool(const std::string& tool_name, 
                                const std::string& parameters_json,
                                const std::map<std::string, ToolInfo>& tools);
extern std::string format_tool_result(const std::string& tool_name, const std::string& result_json);
extern std::string generate_tool_schema(const std::map<std::string, ToolInfo>& tools);

#endif // LUUP_INTERNAL_H

