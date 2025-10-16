/**
 * @file model.cpp
 * @brief Model abstraction layer implementation
 */

#include "../../include/luup_agent.h"
#include "internal.h"
#include <string>
#include <memory>
#include <cstring>
#include <cstdlib>

// Internal model structure
struct luup_model {
    std::string path;
    int gpu_layers;
    int context_size;
    int threads;
    std::string api_key;
    std::string api_base_url;
    bool is_local;
    
    // Backend-specific data
    void* backend_data;
    
    // Cached info
    std::string device_type;
    int gpu_layers_loaded;
    size_t memory_usage;
    
    luup_model() : gpu_layers(-1), context_size(2048), threads(0), 
                   is_local(true), backend_data(nullptr),
                   gpu_layers_loaded(0), memory_usage(0) {}
    
    ~luup_model() {
        if (backend_data) {
            if (is_local) {
                llama_backend_free(backend_data);
            }
            // Remote backend cleanup will be added in Phase 2
        }
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
        
        // Initialize llama.cpp backend
        model->backend_data = llama_backend_init(
            config->path,
            config->gpu_layers,
            model->context_size,
            model->threads
        );
        
        if (!model->backend_data) {
            // Error already set by llama_backend_init
            delete model;
            return nullptr;
        }
        
        // Get backend info
        const char* device = nullptr;
        llama_backend_get_info(model->backend_data, &device, 
                              &model->gpu_layers_loaded, 
                              &model->memory_usage);
        if (device) {
            model->device_type = device;
        }
        
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
    
    if (!model->backend_data) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "Model backend not initialized");
        return LUUP_ERROR_INVALID_PARAM;
    }
    
    if (model->is_local) {
        // Run warmup inference using llama.cpp backend
        if (!llama_backend_warmup(model->backend_data)) {
            // Error already set by llama_backend_warmup
            return LUUP_ERROR_INFERENCE_FAILED;
        }
    } else {
        // Remote models don't need warmup
        luup_clear_error();
    }
    
    return LUUP_SUCCESS;
}

luup_error_t luup_model_get_info(luup_model* model, luup_model_info* out_info) {
    if (!model || !out_info) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "Invalid parameters");
        return LUUP_ERROR_INVALID_PARAM;
    }
    
    // Fill in model info
    out_info->backend = model->is_local ? "llama.cpp" : "openai";
    out_info->device = model->device_type.c_str();
    out_info->gpu_layers_loaded = model->gpu_layers_loaded;
    out_info->memory_usage = model->memory_usage;
    out_info->context_size = model->context_size;
    
    luup_clear_error();
    return LUUP_SUCCESS;
}

void luup_model_destroy(luup_model* model) {
    if (model) {
        delete model;
    }
}

void luup_free_string(char* str) {
    if (str) {
        free(str);
    }
}

} // extern "C"

// Helper function for internal use
void* luup_model_get_backend_data(luup_model* model) {
    return model ? model->backend_data : nullptr;
}

