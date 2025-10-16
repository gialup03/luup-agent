/**
 * @file model.cpp
 * @brief Model abstraction layer implementation
 */

#include "../../include/luup_agent.h"
#include "internal.h"
#include <string>
#include <memory>

// Forward declare internal error setter
extern void luup_set_error(luup_error_t code, const char* message);

// Internal model structure
struct luup_model {
    std::string path;
    int gpu_layers;
    int context_size;
    int threads;
    std::string api_key;
    std::string api_base_url;
    bool is_local;
    
    // Backend-specific data (will be implemented)
    void* backend_data;
    
    luup_model() : gpu_layers(-1), context_size(2048), threads(0), 
                   is_local(true), backend_data(nullptr) {}
    
    ~luup_model() {
        // Backend cleanup will be implemented
    }
};

extern "C" {

luup_model* luup_model_create_local(const luup_model_config* config) {
    if (!config || !config->path) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "Invalid model configuration");
        return nullptr;
    }
    
    try {
        auto model = new luup_model();
        model->path = config->path;
        model->gpu_layers = config->gpu_layers;
        model->context_size = config->context_size > 0 ? config->context_size : 2048;
        model->threads = config->threads;
        model->is_local = true;
        
        // TODO: Initialize llama.cpp backend
        // This will be implemented in Phase 1
        
        return model;
    } catch (const std::exception& e) {
        luup_set_error(LUUP_ERROR_OUT_OF_MEMORY, e.what());
        return nullptr;
    }
}

luup_model* luup_model_create_remote(const luup_model_config* config) {
    if (!config || !config->path) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "Invalid model configuration");
        return nullptr;
    }
    
    try {
        auto model = new luup_model();
        model->path = config->path;
        model->api_key = config->api_key ? config->api_key : "";
        model->api_base_url = config->api_base_url ? config->api_base_url : "";
        model->context_size = config->context_size > 0 ? config->context_size : 2048;
        model->is_local = false;
        
        // TODO: Initialize remote API backend
        // This will be implemented in Phase 2
        
        return model;
    } catch (const std::exception& e) {
        luup_set_error(LUUP_ERROR_OUT_OF_MEMORY, e.what());
        return nullptr;
    }
}

luup_error_t luup_model_warmup(luup_model* model) {
    if (!model) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "Invalid model handle");
        return LUUP_ERROR_INVALID_PARAM;
    }
    
    // TODO: Run dummy inference for warmup
    // This will be implemented in Phase 1
    
    return LUUP_SUCCESS;
}

luup_error_t luup_model_get_info(luup_model* model, luup_model_info* out_info) {
    if (!model || !out_info) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "Invalid parameters");
        return LUUP_ERROR_INVALID_PARAM;
    }
    
    // TODO: Fill info from backend
    // This will be implemented in Phase 1
    
    out_info->backend = model->is_local ? "llama.cpp" : "openai";
    out_info->device = "CPU"; // Will be detected from backend
    out_info->gpu_layers_loaded = 0;
    out_info->memory_usage = 0;
    out_info->context_size = model->context_size;
    
    return LUUP_SUCCESS;
}

void luup_model_destroy(luup_model* model) {
    if (model) {
        // TODO: Clean up backend resources
        delete model;
    }
}

} // extern "C"

