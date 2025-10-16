/**
 * @file tool_calling.cpp
 * @brief Tool calling and execution system
 */

#include "../../include/luup_agent.h"
#include "internal.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <sstream>

using json = nlohmann::json;

// Tool registration info structure
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

// Parsed tool call structure
struct ToolCall {
    std::string tool_name;
    std::string parameters_json;
};

/**
 * @brief Parse tool calls from LLM output
 * 
 * Expected format in LLM output:
 * ```json
 * {
 *   "tool_calls": [
 *     {
 *       "name": "tool_name",
 *       "parameters": { ... }
 *     }
 *   ]
 * }
 * ```
 * 
 * @param text LLM output text
 * @return Vector of parsed tool calls
 */
std::vector<ToolCall> parse_tool_calls(const std::string& text) {
    std::vector<ToolCall> tool_calls;
    
    try {
        // Helper function to extract JSON with proper brace matching
        auto extract_json = [](const std::string& str, size_t start_pos) -> std::string {
            if (start_pos >= str.length() || str[start_pos] != '{') {
                return "";
            }
            
            int brace_count = 0;
            size_t i = start_pos;
            bool in_string = false;
            bool escape_next = false;
            
            while (i < str.length()) {
                char c = str[i];
                
                if (escape_next) {
                    escape_next = false;
                } else if (c == '\\' && in_string) {
                    escape_next = true;
                } else if (c == '"') {
                    in_string = !in_string;
                } else if (!in_string) {
                    if (c == '{') {
                        brace_count++;
                    } else if (c == '}') {
                        brace_count--;
                        if (brace_count == 0) {
                            return str.substr(start_pos, i - start_pos + 1);
                        }
                    }
                }
                i++;
            }
            return "";
        };
        
        // Look for JSON objects in the text
        size_t pos = 0;
        while ((pos = text.find('{', pos)) != std::string::npos) {
            std::string json_str = extract_json(text, pos);
            
            if (!json_str.empty()) {
                try {
                    json j = json::parse(json_str);
                    
                    // Check if this is a tool call structure
                    if (j.contains("tool_calls") && j["tool_calls"].is_array()) {
                        for (const auto& call : j["tool_calls"]) {
                            if (call.contains("name") && call.contains("parameters")) {
                                ToolCall tc;
                                tc.tool_name = call["name"].get<std::string>();
                                tc.parameters_json = call["parameters"].dump();
                                tool_calls.push_back(tc);
                            }
                        }
                        // Found valid tool calls, we're done
                        break;
                    }
                    // Also support direct tool call format
                    else if (j.contains("name") && j.contains("parameters")) {
                        ToolCall tc;
                        tc.tool_name = j["name"].get<std::string>();
                        tc.parameters_json = j["parameters"].dump();
                        tool_calls.push_back(tc);
                        // Found valid tool call, we're done
                        break;
                    }
                } catch (const json::exception&) {
                    // Not valid JSON, continue searching
                }
            }
            
            pos++;
        }
    } catch (const std::exception&) {
        // Error parsing, return empty list
    }
    
    return tool_calls;
}

/**
 * @brief Execute a tool call
 * 
 * @param tool_name Name of the tool to execute
 * @param parameters_json JSON string with parameters
 * @param tools Map of registered tools
 * @return Tool execution result as JSON string, or empty string on error
 */
std::string execute_tool(
    const std::string& tool_name,
    const std::string& parameters_json,
    const std::map<std::string, ToolInfo>& tools)
{
    auto it = tools.find(tool_name);
    if (it == tools.end()) {
        // Tool not found
        json error_result = {
            {"error", "Tool not found"},
            {"tool_name", tool_name}
        };
        return error_result.dump();
    }
    
    const ToolInfo& tool_info = it->second;
    
    try {
        // Execute the tool callback
        char* result = tool_info.callback(parameters_json.c_str(), tool_info.user_data);
        
        if (result) {
            std::string result_str(result);
            free(result);  // Free the result returned by callback
            return result_str;
        } else {
            // Callback returned null - execution failed
            json error_result = {
                {"error", "Tool execution failed"},
                {"tool_name", tool_name}
            };
            return error_result.dump();
        }
    } catch (const std::exception& e) {
        json error_result = {
            {"error", e.what()},
            {"tool_name", tool_name}
        };
        return error_result.dump();
    }
}

/**
 * @brief Format tool results for the LLM
 * 
 * @param tool_name Name of the tool
 * @param result_json JSON result from tool execution
 * @return Formatted string for LLM
 */
std::string format_tool_result(const std::string& tool_name, const std::string& result_json) {
    std::ostringstream oss;
    oss << "Tool '" << tool_name << "' returned:\n";
    oss << result_json;
    return oss.str();
}

/**
 * @brief Generate tool schema for system prompt
 * 
 * Creates a description of available tools for the LLM to understand
 * 
 * @param tools Map of registered tools
 * @return Tool schema as string
 */
std::string generate_tool_schema(const std::map<std::string, ToolInfo>& tools) {
    if (tools.empty()) {
        return "";
    }
    
    std::ostringstream oss;
    oss << "\n\nYou have access to the following tools:\n\n";
    
    for (const auto& [name, info] : tools) {
        oss << "Tool: " << name << "\n";
        oss << "Description: " << (info.tool.description ? info.tool.description : "No description") << "\n";
        oss << "Parameters: " << (info.tool.parameters_json ? info.tool.parameters_json : "{}") << "\n\n";
    }
    
    oss << "To call a tool, respond with JSON in the following format:\n";
    oss << "```json\n";
    oss << "{\n";
    oss << "  \"tool_calls\": [\n";
    oss << "    {\n";
    oss << "      \"name\": \"tool_name\",\n";
    oss << "      \"parameters\": { ... }\n";
    oss << "    }\n";
    oss << "  ]\n";
    oss << "}\n";
    oss << "```\n\n";
    
    return oss.str();
}

// TODO: Future enhancements:
// - JSON schema validation for tool parameters
// - Retry logic for failed tool calls
// - Parallel tool execution
// - Tool call timeouts
// - Tool call permissions/security

