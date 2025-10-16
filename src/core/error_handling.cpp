/**
 * @file error_handling.cpp
 * @brief Error handling implementation with thread-local storage
 */

#include "../../include/luup_agent.h"
#include <cstring>
#include <string>
#include <mutex>

namespace {
    // Thread-local error message storage
    thread_local std::string last_error_message;
    
    // Global error callback
    luup_error_callback_t global_error_callback = nullptr;
    void* global_error_callback_user_data = nullptr;
    std::mutex callback_mutex;
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
    last_error_message = message ? message : "";
    
    // Call global error callback if set
    std::lock_guard<std::mutex> lock(callback_mutex);
    if (global_error_callback) {
        global_error_callback(code, message, global_error_callback_user_data);
    }
}

