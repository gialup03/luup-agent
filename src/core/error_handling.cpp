/**
 * @file error_handling.cpp
 * @brief Error handling implementation with thread-local storage
 */

#include "../../include/luup_agent.h"
#include "internal.h"
#include <cstring>
#include <string>
#include <mutex>
#include <sstream>

namespace {
    // Thread-local error message storage
    thread_local std::string last_error_message;
    thread_local luup_error_t last_error_code = LUUP_SUCCESS;
    
    // Global error callback
    luup_error_callback_t global_error_callback = nullptr;
    void* global_error_callback_user_data = nullptr;
    std::mutex callback_mutex;
    
    // Error code to string mapping
    const char* error_code_to_string(luup_error_t code) {
        switch (code) {
            case LUUP_SUCCESS: return "Success";
            case LUUP_ERROR_INVALID_PARAM: return "Invalid parameter";
            case LUUP_ERROR_OUT_OF_MEMORY: return "Out of memory";
            case LUUP_ERROR_MODEL_NOT_FOUND: return "Model file not found";
            case LUUP_ERROR_INFERENCE_FAILED: return "Inference failed";
            case LUUP_ERROR_TOOL_NOT_FOUND: return "Tool not found";
            case LUUP_ERROR_JSON_PARSE_FAILED: return "JSON parse failed";
            case LUUP_ERROR_HTTP_FAILED: return "HTTP request failed";
            case LUUP_ERROR_BACKEND_INIT_FAILED: return "Backend initialization failed";
            default: return "Unknown error";
        }
    }
}

extern "C" {

const char* luup_get_last_error(void) {
    return last_error_message.c_str();
}

void luup_set_error_callback(luup_error_callback_t callback, void* user_data) {
    std::lock_guard<std::mutex> lock(callback_mutex);
    global_error_callback = callback;
    global_error_callback_user_data = user_data;
}

} // extern "C"

// Internal helper function to set error
void luup_set_error(luup_error_t code, const char* message) {
    last_error_code = code;
    
    // Build error message with code
    if (message && message[0] != '\0') {
        std::ostringstream oss;
        oss << "[" << error_code_to_string(code) << "] " << message;
        last_error_message = oss.str();
    } else {
        last_error_message = error_code_to_string(code);
    }
    
    // Call global error callback if set
    std::lock_guard<std::mutex> lock(callback_mutex);
    if (global_error_callback) {
        global_error_callback(code, last_error_message.c_str(), global_error_callback_user_data);
    }
}

// Internal helper to clear error
void luup_clear_error() {
    last_error_code = LUUP_SUCCESS;
    last_error_message.clear();
}

// Internal helper to get last error code
luup_error_t luup_get_last_error_code() {
    return last_error_code;
}

