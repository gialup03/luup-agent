/**
 * @file remote_api.cpp
 * @brief Remote API (OpenAI-compatible) backend
 */

#include "../../include/luup_agent.h"
#include "../core/internal.h"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <string>
#include <sstream>
#include <memory>
#include <regex>
#include <cstring>

using json = nlohmann::json;

// Backend data structure for remote API
struct openai_backend_data {
    std::string api_endpoint;
    std::string api_key;
    std::string model_name;
    int context_size;
    
    openai_backend_data(const char* endpoint, const char* key, const char* model, int ctx_size)
        : api_endpoint(endpoint ? endpoint : "https://api.openai.com/v1"),
          api_key(key ? key : ""),
          model_name(model ? model : "gpt-4"),
          context_size(ctx_size > 0 ? ctx_size : 8192) {}
};

namespace {
    // Parse URL into components
    struct ParsedURL {
        std::string scheme;
        std::string host;
        int port;
        std::string path;
        
        ParsedURL() : port(443) {}
    };
    
    bool parse_url(const std::string& url, ParsedURL& out) {
        // Simple URL parser for https://host:port/path format
        std::regex url_regex(R"(^(https?)://([^:/]+)(?::(\d+))?(/.*)?$)");
        std::smatch match;
        
        if (std::regex_match(url, match, url_regex)) {
            out.scheme = match[1].str();
            out.host = match[2].str();
            
            if (match[3].matched) {
                out.port = std::stoi(match[3].str());
            } else {
                out.port = (out.scheme == "https") ? 443 : 80;
            }
            
            out.path = match[4].matched ? match[4].str() : "/";
            return true;
        }
        
        return false;
    }
    
    // Parse SSE (Server-Sent Events) data line
    std::string parse_sse_data(const std::string& line) {
        if (line.find("data: ") == 0) {
            return line.substr(6); // Skip "data: " prefix
        }
        return "";
    }
    
    // Extract content from streaming chunk
    std::string extract_streaming_content(const std::string& json_str) {
        try {
            if (json_str == "[DONE]") {
                return "";
            }
            
            auto j = json::parse(json_str);
            if (j.contains("choices") && !j["choices"].empty()) {
                auto& choice = j["choices"][0];
                if (choice.contains("delta") && choice["delta"].contains("content")) {
                    return choice["delta"]["content"].get<std::string>();
                }
            }
        } catch (const json::exception&) {
            // Ignore parsing errors for streaming chunks
        }
        return "";
    }
    
    // Extract tool calls from response
    std::string extract_tool_calls(const json& response) {
        try {
            if (response.contains("choices") && !response["choices"].empty()) {
                auto& choice = response["choices"][0];
                if (choice.contains("message") && choice["message"].contains("tool_calls")) {
                    // Convert OpenAI tool calls to our format
                    auto& tool_calls = choice["message"]["tool_calls"];
                    if (!tool_calls.empty()) {
                        std::string result;
                        for (const auto& tc : tool_calls) {
                            if (tc.contains("function")) {
                                std::string name = tc["function"]["name"];
                                std::string args = tc["function"]["arguments"];
                                result += "<tool_call>" + name + "(" + args + ")</tool_call>\n";
                            }
                        }
                        return result;
                    }
                }
            }
        } catch (const json::exception&) {
            // Return empty if tool calls can't be parsed
        }
        return "";
    }
}

// Initialize remote API backend
void* openai_backend_init(const char* api_endpoint, const char* api_key, 
                          const char* model_name, int context_size) {
    // Validate parameters
    if (!api_key || strlen(api_key) == 0) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "API key is required for remote models");
        return nullptr;
    }
    
    if (!model_name || strlen(model_name) == 0) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "Model name is required for remote models");
        return nullptr;
    }
    
    try {
        // Set default endpoint if not provided
        const char* endpoint = api_endpoint && strlen(api_endpoint) > 0 
            ? api_endpoint 
            : "https://api.openai.com/v1";
        
        // Validate endpoint URL
        ParsedURL parsed;
        if (!parse_url(endpoint, parsed)) {
            luup_set_error(LUUP_ERROR_INVALID_PARAM, 
                          ("Invalid API endpoint URL: " + std::string(endpoint)).c_str());
            return nullptr;
        }
        
        // Create backend data
        auto backend = new openai_backend_data(endpoint, api_key, model_name, context_size);
        
        // Test connection with a simple request (optional, but good for validation)
        // For now, we'll just validate the parameters and return
        
        luup_clear_error();
        return backend;
        
    } catch (const std::exception& e) {
        luup_set_error(LUUP_ERROR_BACKEND_INIT_FAILED, e.what());
        return nullptr;
    }
}

// Free backend resources
void openai_backend_free(void* backend_data) {
    if (backend_data) {
        delete static_cast<openai_backend_data*>(backend_data);
    }
}

// Generate text using OpenAI API
char* openai_backend_generate(void* backend_data, const char* prompt,
                               float temperature, int max_tokens) {
    if (!backend_data || !prompt) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "Invalid parameters");
        return nullptr;
    }
    
    auto backend = static_cast<openai_backend_data*>(backend_data);
    
    try {
        // Parse endpoint URL
        ParsedURL parsed;
        if (!parse_url(backend->api_endpoint, parsed)) {
            luup_set_error(LUUP_ERROR_HTTP_FAILED, "Invalid API endpoint URL");
            return nullptr;
        }
        
        // Build request body
        json request_body = {
            {"model", backend->model_name},
            {"messages", json::array({
                {{"role", "user"}, {"content", prompt}}
            })},
            {"temperature", temperature},
            {"stream", false}
        };
        
        if (max_tokens > 0) {
            request_body["max_tokens"] = max_tokens;
        }
        
        std::string body_str = request_body.dump();
        
        // Build headers
        httplib::Headers headers = {
            {"Content-Type", "application/json"},
            {"Authorization", "Bearer " + backend->api_key}
        };
        
        // Make request to /chat/completions endpoint
        std::string endpoint_path = parsed.path;
        if (endpoint_path.back() != '/') {
            endpoint_path += "/";
        }
        endpoint_path += "chat/completions";
        
        // Create HTTP client and make request
        httplib::Result response;
        if (parsed.scheme == "https") {
            httplib::SSLClient client(parsed.host, parsed.port);
            client.set_connection_timeout(30, 0);  // 30 seconds
            client.set_read_timeout(120, 0);       // 120 seconds for generation
            response = client.Post(endpoint_path, headers, body_str, "application/json");
        } else {
            httplib::Client client(parsed.host, parsed.port);
            client.set_connection_timeout(30, 0);
            client.set_read_timeout(120, 0);
            response = client.Post(endpoint_path, headers, body_str, "application/json");
        }
        
        if (!response) {
            luup_set_error(LUUP_ERROR_HTTP_FAILED, "Failed to connect to API endpoint");
            return nullptr;
        }
        
        if (response->status != 200) {
            std::string error_msg = "API request failed with status " + 
                                   std::to_string(response->status);
            
            // Try to extract error message from response
            try {
                auto error_json = json::parse(response->body);
                if (error_json.contains("error") && error_json["error"].contains("message")) {
                    error_msg += ": " + error_json["error"]["message"].get<std::string>();
                }
            } catch (...) {
                error_msg += ": " + response->body;
            }
            
            luup_set_error(LUUP_ERROR_HTTP_FAILED, error_msg.c_str());
            return nullptr;
        }
        
        // Parse response
        auto response_json = json::parse(response->body);
        
        // Check for tool calls first
        std::string tool_calls = extract_tool_calls(response_json);
        if (!tool_calls.empty()) {
            char* result = static_cast<char*>(malloc(tool_calls.size() + 1));
            if (result) {
                memcpy(result, tool_calls.c_str(), tool_calls.size());
                result[tool_calls.size()] = '\0';
            }
            luup_clear_error();
            return result;
        }
        
        // Extract content
        if (response_json.contains("choices") && !response_json["choices"].empty()) {
            auto& choice = response_json["choices"][0];
            if (choice.contains("message") && choice["message"].contains("content")) {
                std::string content = choice["message"]["content"].get<std::string>();
                
                // Allocate and return result
                char* result = static_cast<char*>(malloc(content.size() + 1));
                if (result) {
                    memcpy(result, content.c_str(), content.size());
                    result[content.size()] = '\0';
                }
                
                luup_clear_error();
                return result;
            }
        }
        
        luup_set_error(LUUP_ERROR_INFERENCE_FAILED, "No content in API response");
        return nullptr;
        
    } catch (const json::exception& e) {
        luup_set_error(LUUP_ERROR_JSON_PARSE_FAILED, e.what());
        return nullptr;
    } catch (const std::exception& e) {
        luup_set_error(LUUP_ERROR_HTTP_FAILED, e.what());
        return nullptr;
    }
}

// Generate text with streaming using OpenAI API
bool openai_backend_generate_stream(void* backend_data, const char* prompt,
                                    float temperature, int max_tokens,
                                    void (*callback)(const char* token, void* user_data),
                                    void* user_data) {
    if (!backend_data || !prompt || !callback) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "Invalid parameters");
        return false;
    }
    
    auto backend = static_cast<openai_backend_data*>(backend_data);
    
    try {
        // Parse endpoint URL
        ParsedURL parsed;
        if (!parse_url(backend->api_endpoint, parsed)) {
            luup_set_error(LUUP_ERROR_HTTP_FAILED, "Invalid API endpoint URL");
            return false;
        }
        
        // Build request body
        json request_body = {
            {"model", backend->model_name},
            {"messages", json::array({
                {{"role", "user"}, {"content", prompt}}
            })},
            {"temperature", temperature},
            {"stream", true}
        };
        
        if (max_tokens > 0) {
            request_body["max_tokens"] = max_tokens;
        }
        
        std::string body_str = request_body.dump();
        
        // Build headers
        httplib::Headers headers = {
            {"Content-Type", "application/json"},
            {"Authorization", "Bearer " + backend->api_key}
        };
        
        // Build endpoint path
        std::string endpoint_path = parsed.path;
        if (endpoint_path.back() != '/') {
            endpoint_path += "/";
        }
        endpoint_path += "chat/completions";
        
        // Create HTTP client and make streaming request
        httplib::Result response;
        if (parsed.scheme == "https") {
            httplib::SSLClient client(parsed.host, parsed.port);
            client.set_connection_timeout(30, 0);
            client.set_read_timeout(300, 0);  // Longer timeout for streaming
            response = client.Post(endpoint_path, headers, body_str, "application/json");
        } else {
            httplib::Client client(parsed.host, parsed.port);
            client.set_connection_timeout(30, 0);
            client.set_read_timeout(300, 0);
            response = client.Post(endpoint_path, headers, body_str, "application/json");
        }
        
        if (!response) {
            luup_set_error(LUUP_ERROR_HTTP_FAILED, "Failed to connect to API endpoint");
            return false;
        }
        
        if (response->status != 200) {
            std::string error_msg = "API streaming request failed with status " + 
                                   std::to_string(response->status);
            
            // Try to extract error message from response
            try {
                auto error_json = json::parse(response->body);
                if (error_json.contains("error") && error_json["error"].contains("message")) {
                    error_msg += ": " + error_json["error"]["message"].get<std::string>();
                }
            } catch (...) {
                if (!response->body.empty()) {
                    error_msg += ": " + response->body;
                }
            }
            
            luup_set_error(LUUP_ERROR_HTTP_FAILED, error_msg.c_str());
            return false;
        }
        
        // Parse SSE response and call callback for each token
        std::string buffer = response->body;
        size_t pos = 0;
        
        while (pos < buffer.size()) {
            // Find next newline
            size_t newline_pos = buffer.find('\n', pos);
            if (newline_pos == std::string::npos) {
                // Process remaining data
                newline_pos = buffer.size();
            }
            
            std::string line = buffer.substr(pos, newline_pos - pos);
            pos = newline_pos + 1;
            
            // Skip empty lines
            if (line.empty() || line == "\r") {
                continue;
            }
            
            // Parse SSE data
            std::string data_str = parse_sse_data(line);
            if (data_str.empty()) {
                continue;
            }
            
            // Extract content from chunk
            std::string content = extract_streaming_content(data_str);
            if (!content.empty()) {
                callback(content.c_str(), user_data);
            }
        }
        
        luup_clear_error();
        return true;
        
    } catch (const json::exception& e) {
        luup_set_error(LUUP_ERROR_JSON_PARSE_FAILED, e.what());
        return false;
    } catch (const std::exception& e) {
        luup_set_error(LUUP_ERROR_HTTP_FAILED, e.what());
        return false;
    }
}

// Get backend information
bool openai_backend_get_info(void* backend_data, const char** model_name, 
                             int* context_size) {
    if (!backend_data) {
        return false;
    }
    
    auto backend = static_cast<openai_backend_data*>(backend_data);
    if (model_name) {
        *model_name = backend->model_name.c_str();
    }
    if (context_size) {
        *context_size = backend->context_size;
    }
    
    return true;
}

