/**
 * @file luup_agent.h
 * @brief luup-agent: Multi-Agent LLM Library C API
 * 
 * Cross-platform C library for LLM inference with multi-agent support and tool calling.
 * 
 * @version 0.1.0
 * @author luup-agent contributors
 * @copyright MIT License
 */

#ifndef LUUP_AGENT_H
#define LUUP_AGENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

// Version information
#define LUUP_VERSION_MAJOR 0
#define LUUP_VERSION_MINOR 1
#define LUUP_VERSION_PATCH 0

// Export/Import macros for Windows DLL
#if defined(_WIN32)
    #ifdef LUUP_EXPORT
        #define LUUP_API __declspec(dllexport)
    #elif defined(LUUP_IMPORT)
        #define LUUP_API __declspec(dllimport)
    #else
        #define LUUP_API
    #endif
#else
    #define LUUP_API __attribute__((visibility("default")))
#endif

// ============================================================================
// Error Handling
// ============================================================================

/**
 * @brief Error codes returned by luup-agent functions
 */
typedef enum {
    LUUP_SUCCESS = 0,                    /**< Operation completed successfully */
    LUUP_ERROR_INVALID_PARAM = -1,       /**< Invalid parameter provided */
    LUUP_ERROR_OUT_OF_MEMORY = -2,       /**< Memory allocation failed */
    LUUP_ERROR_MODEL_NOT_FOUND = -3,     /**< Model file not found */
    LUUP_ERROR_INFERENCE_FAILED = -4,    /**< Inference operation failed */
    LUUP_ERROR_TOOL_NOT_FOUND = -5,      /**< Requested tool not registered */
    LUUP_ERROR_JSON_PARSE_FAILED = -6,   /**< JSON parsing failed */
    LUUP_ERROR_HTTP_FAILED = -7,         /**< HTTP request failed */
    LUUP_ERROR_BACKEND_INIT_FAILED = -8  /**< Backend initialization failed */
} luup_error_t;

/**
 * @brief Get last error message (thread-local)
 * @return Error message string, valid until next luup API call in same thread
 */
LUUP_API const char* luup_get_last_error(void);

/**
 * @brief Error callback function type
 * @param code Error code
 * @param msg Error message
 * @param user_data User-provided data pointer
 */
typedef void (*luup_error_callback_t)(luup_error_t code, const char* msg, void* user_data);

/**
 * @brief Set global error callback for diagnostics
 * @param callback Callback function to invoke on errors
 * @param user_data User data to pass to callback
 */
LUUP_API void luup_set_error_callback(luup_error_callback_t callback, void* user_data);

// ============================================================================
// Model Layer API
// ============================================================================

/**
 * @brief Opaque model handle
 */
typedef struct luup_model luup_model;

/**
 * @brief Model configuration structure
 */
typedef struct {
    const char* path;              /**< Path to GGUF file or API endpoint URL */
    int gpu_layers;                /**< GPU layers: -1 for auto, 0 for CPU only, N for specific count */
    int context_size;              /**< Context window size (default: 2048) */
    int threads;                   /**< CPU threads (0 for auto-detect) */
    const char* api_key;           /**< API key for remote models (optional) */
    const char* api_base_url;      /**< Custom API endpoint (optional) */
} luup_model_config;

/**
 * @brief Model information structure
 */
typedef struct {
    const char* backend;           /**< Backend type: "llama.cpp", "openai", etc. */
    const char* device;            /**< Device: "Metal", "CUDA", "ROCm", "Vulkan", "CPU" */
    int gpu_layers_loaded;         /**< Actual number of layers loaded on GPU */
    size_t memory_usage;           /**< Estimated memory usage in bytes */
    int context_size;              /**< Configured context window size */
} luup_model_info;

/**
 * @brief Create a local model using llama.cpp backend
 * @param config Model configuration
 * @return Model handle or NULL on error
 */
LUUP_API luup_model* luup_model_create_local(const luup_model_config* config);

/**
 * @brief Create a remote model using OpenAI-compatible API
 * @param config Model configuration with API endpoint and key
 * @return Model handle or NULL on error
 */
LUUP_API luup_model* luup_model_create_remote(const luup_model_config* config);

/**
 * @brief Pre-warm model by running a dummy inference
 * 
 * This reduces first-token latency for subsequent generations.
 * Optional but recommended for better user experience.
 * 
 * @param model Model handle
 * @return LUUP_SUCCESS or error code
 */
LUUP_API luup_error_t luup_model_warmup(luup_model* model);

/**
 * @brief Get model information
 * @param model Model handle
 * @param out_info Pointer to info structure to fill
 * @return LUUP_SUCCESS or error code
 */
LUUP_API luup_error_t luup_model_get_info(luup_model* model, luup_model_info* out_info);

/**
 * @brief Destroy model and free resources
 * @param model Model handle
 */
LUUP_API void luup_model_destroy(luup_model* model);

// ============================================================================
// Agent Layer API
// ============================================================================

/**
 * @brief Opaque agent handle
 */
typedef struct luup_agent luup_agent;

/**
 * @brief Agent configuration structure
 */
typedef struct {
    luup_model* model;                  /**< Model to use (can be shared across agents) */
    const char* system_prompt;          /**< System prompt defining agent's role */
    float temperature;                  /**< Sampling temperature: 0.0 to 2.0 (default: 0.7) */
    int max_tokens;                     /**< Maximum tokens to generate (0 for no limit) */
    bool enable_tool_calling;           /**< Enable function calling (default: true) */
    bool enable_history_management;     /**< Auto-manage conversation history (default: true) */
} luup_agent_config;

/**
 * @brief Tool definition structure
 */
typedef struct {
    const char* name;                   /**< Tool name (must be unique per agent) */
    const char* description;            /**< Human-readable description */
    const char* parameters_json;        /**< JSON schema for parameters */
} luup_tool;

/**
 * @brief Tool callback function type
 * 
 * @param params_json JSON string with tool parameters
 * @param user_data User-provided data pointer
 * @return JSON string with result (caller must free with luup_free_string)
 */
typedef char* (*luup_tool_callback_t)(const char* params_json, void* user_data);

/**
 * @brief Streaming callback function type
 * 
 * Called for each generated token during streaming generation.
 * 
 * @param token Generated token string
 * @param user_data User-provided data pointer
 */
typedef void (*luup_stream_callback_t)(const char* token, void* user_data);

/**
 * @brief Create a new agent
 * @param config Agent configuration
 * @return Agent handle or NULL on error
 */
LUUP_API luup_agent* luup_agent_create(const luup_agent_config* config);

/**
 * @brief Register a tool with an agent
 * 
 * Tools can be called by the agent during generation when appropriate.
 * 
 * @param agent Agent handle
 * @param tool Tool definition
 * @param callback Function to call when tool is invoked
 * @param user_data User data to pass to callback
 * @return LUUP_SUCCESS or error code
 */
LUUP_API luup_error_t luup_agent_register_tool(
    luup_agent* agent,
    const luup_tool* tool,
    luup_tool_callback_t callback,
    void* user_data
);

/**
 * @brief Generate response with streaming
 * 
 * Generates tokens one at a time, calling the callback for each token.
 * Handles tool calling automatically if enabled.
 * 
 * @param agent Agent handle
 * @param user_message User's input message
 * @param callback Streaming callback for each token
 * @param user_data User data to pass to callback
 * @return LUUP_SUCCESS or error code
 */
LUUP_API luup_error_t luup_agent_generate_stream(
    luup_agent* agent,
    const char* user_message,
    luup_stream_callback_t callback,
    void* user_data
);

/**
 * @brief Generate complete response (blocking)
 * 
 * Generates full response and returns it as a string.
 * Handles tool calling automatically if enabled.
 * 
 * @param agent Agent handle
 * @param user_message User's input message
 * @return Generated response (caller must free with luup_free_string) or NULL on error
 */
LUUP_API char* luup_agent_generate(luup_agent* agent, const char* user_message);

/**
 * @brief Manually add a message to conversation history
 * @param agent Agent handle
 * @param role Message role: "user", "assistant", or "system"
 * @param content Message content
 * @return LUUP_SUCCESS or error code
 */
LUUP_API luup_error_t luup_agent_add_message(
    luup_agent* agent,
    const char* role,
    const char* content
);

/**
 * @brief Clear conversation history
 * @param agent Agent handle
 * @return LUUP_SUCCESS or error code
 */
LUUP_API luup_error_t luup_agent_clear_history(luup_agent* agent);

/**
 * @brief Get conversation history as JSON
 * @param agent Agent handle
 * @return JSON string (caller must free with luup_free_string) or NULL on error
 */
LUUP_API char* luup_agent_get_history_json(luup_agent* agent);

/**
 * @brief Enable built-in todo list tool
 * @param agent Agent handle
 * @param storage_path Path to store todo list JSON (NULL for memory only)
 * @return LUUP_SUCCESS or error code
 */
LUUP_API luup_error_t luup_agent_enable_builtin_todo(
    luup_agent* agent,
    const char* storage_path
);

/**
 * @brief Enable built-in notes tool
 * @param agent Agent handle
 * @param storage_path Path to store notes JSON (NULL for memory only)
 * @return LUUP_SUCCESS or error code
 */
LUUP_API luup_error_t luup_agent_enable_builtin_notes(
    luup_agent* agent,
    const char* storage_path
);

/**
 * @brief Enable built-in auto-summarization
 * 
 * Automatically summarizes conversation history when context fills.
 * 
 * @param agent Agent handle
 * @return LUUP_SUCCESS or error code
 */
LUUP_API luup_error_t luup_agent_enable_builtin_summarization(luup_agent* agent);

/**
 * @brief Destroy agent and free resources
 * @param agent Agent handle
 */
LUUP_API void luup_agent_destroy(luup_agent* agent);

// ============================================================================
// Memory Management
// ============================================================================

/**
 * @brief Free string allocated by library
 * 
 * Use this to free strings returned by:
 * - luup_agent_generate()
 * - luup_agent_get_history_json()
 * - Tool callbacks return values
 * 
 * @param str String to free
 */
LUUP_API void luup_free_string(char* str);

// ============================================================================
// Version Information
// ============================================================================

/**
 * @brief Get library version string
 * @return Version string (e.g., "0.1.0")
 */
LUUP_API const char* luup_version(void);

/**
 * @brief Get library version components
 * @param major Output for major version
 * @param minor Output for minor version
 * @param patch Output for patch version
 */
LUUP_API void luup_version_components(int* major, int* minor, int* patch);

#ifdef __cplusplus
}
#endif

#endif // LUUP_AGENT_H

