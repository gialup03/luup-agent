/**
 * @file local_llama.cpp
 * @brief llama.cpp backend integration
 */

#include "../../include/luup_agent.h"
#include "../core/internal.h"
#include <llama.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <thread>

// Backend data structure for llama.cpp
struct llama_backend_data {
    llama_model* model;
    llama_context* ctx;
    llama_sampler* sampler;
    std::string device_type;
    int gpu_layers_loaded;
    size_t memory_usage;
    
    llama_backend_data() 
        : model(nullptr), ctx(nullptr), sampler(nullptr),
          device_type("CPU"), gpu_layers_loaded(0), memory_usage(0) {}
    
    ~llama_backend_data() {
        if (sampler) {
            llama_sampler_free(sampler);
        }
        if (ctx) {
            llama_free(ctx);
        }
        if (model) {
            llama_model_free(model);
        }
    }
};

namespace {
    // Global initialization flag
    bool llama_initialized = false;
    
    // Initialize llama.cpp backend once
    void ensure_llama_initialized() {
        if (!llama_initialized) {
            llama_backend_init();
            llama_initialized = true;
        }
    }
    
    // Detect available GPU backend
    std::string detect_gpu_backend() {
#if defined(__APPLE__) && defined(GGML_USE_METAL)
        return "Metal";
#elif defined(GGML_USE_CUDA)
        return "CUDA";
#elif defined(GGML_USE_HIPBLAS)
        return "ROCm";
#elif defined(GGML_USE_VULKAN)
        return "Vulkan";
#else
        return "CPU";
#endif
    }
    
    // Auto-detect optimal number of GPU layers
    int auto_detect_gpu_layers(llama_model* model) {
        // Get total layers in model
        int n_layers = llama_model_n_layer(model);
        
        // Try to offload all layers to GPU if available
        std::string backend = detect_gpu_backend();
        if (backend != "CPU") {
            return n_layers;
        }
        
        return 0;
    }
    
    // Check if file exists
    bool file_exists(const char* path) {
        FILE* f = fopen(path, "rb");
        if (f) {
            fclose(f);
            return true;
        }
        return false;
    }
}

// Initialize llama.cpp backend with given model
void* llama_backend_init(const char* model_path, int gpu_layers, 
                         int context_size, int threads) {
    ensure_llama_initialized();
    
    // Check if model file exists
    if (!file_exists(model_path)) {
        luup_set_error(LUUP_ERROR_MODEL_NOT_FOUND, 
                      ("Model file not found: " + std::string(model_path)).c_str());
        return nullptr;
    }
    
    try {
        auto backend = new llama_backend_data();
        
        // Set up model parameters
        llama_model_params model_params = llama_model_default_params();
        
        // Configure GPU layers
        if (gpu_layers == -1) {
            // Auto-detect: load model first to count layers
            llama_model* temp_model = llama_model_load_from_file(model_path, model_params);
            if (!temp_model) {
                luup_set_error(LUUP_ERROR_BACKEND_INIT_FAILED, 
                              "Failed to load model for GPU layer detection");
                delete backend;
                return nullptr;
            }
            model_params.n_gpu_layers = auto_detect_gpu_layers(temp_model);
            llama_model_free(temp_model);
        } else {
            model_params.n_gpu_layers = gpu_layers;
        }
        
        // Load model
        backend->model = llama_model_load_from_file(model_path, model_params);
        if (!backend->model) {
            luup_set_error(LUUP_ERROR_BACKEND_INIT_FAILED, 
                          "Failed to load model from file");
            delete backend;
            return nullptr;
        }
        
        // Set up context parameters
        llama_context_params ctx_params = llama_context_default_params();
        ctx_params.n_ctx = context_size > 0 ? context_size : 2048;
        ctx_params.n_threads = threads > 0 ? threads : std::thread::hardware_concurrency();
        ctx_params.n_threads_batch = ctx_params.n_threads;
        
        // Create context
        backend->ctx = llama_init_from_model(backend->model, ctx_params);
        if (!backend->ctx) {
            luup_set_error(LUUP_ERROR_BACKEND_INIT_FAILED, 
                          "Failed to create llama context");
            delete backend;
            return nullptr;
        }
        
        // Create sampler with default parameters
        backend->sampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
        llama_sampler_chain_add(backend->sampler, 
                               llama_sampler_init_temp(0.7f)); // Default temperature
        llama_sampler_chain_add(backend->sampler,
                               llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
        
        // Store backend info
        backend->device_type = detect_gpu_backend();
        backend->gpu_layers_loaded = model_params.n_gpu_layers;
        
        // Estimate memory usage
        size_t model_size = llama_model_size(backend->model);
        size_t ctx_size = llama_state_get_size(backend->ctx);
        backend->memory_usage = model_size + ctx_size;
        
        luup_clear_error();
        return backend;
        
    } catch (const std::exception& e) {
        luup_set_error(LUUP_ERROR_OUT_OF_MEMORY, e.what());
        return nullptr;
    }
}

// Free backend resources
void llama_backend_free(void* backend_data) {
    if (backend_data) {
        delete static_cast<llama_backend_data*>(backend_data);
    }
}

// Get backend information
bool llama_backend_get_info(void* backend_data, const char** device, 
                            int* gpu_layers, size_t* memory_usage) {
    if (!backend_data) {
        return false;
    }
    
    auto backend = static_cast<llama_backend_data*>(backend_data);
    if (device) {
        *device = backend->device_type.c_str();
    }
    if (gpu_layers) {
        *gpu_layers = backend->gpu_layers_loaded;
    }
    if (memory_usage) {
        *memory_usage = backend->memory_usage;
    }
    
    return true;
}

// Perform warmup inference
bool llama_backend_warmup(void* backend_data) {
    if (!backend_data) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "Invalid backend data");
        return false;
    }
    
    auto backend = static_cast<llama_backend_data*>(backend_data);
    
    try {
        // Tokenize a simple warmup prompt
        const char* warmup_prompt = "Hello";
        std::vector<llama_token> tokens;
        
        // Get vocab from model
        const llama_vocab* vocab = llama_model_get_vocab(backend->model);
        
        // Tokenize
        int n_tokens = llama_tokenize(
            vocab,
            warmup_prompt,
            strlen(warmup_prompt),
            nullptr,
            0,
            true,  // add_special
            true   // parse_special
        );
        
        if (n_tokens <= 0) {
            luup_set_error(LUUP_ERROR_INFERENCE_FAILED, "Failed to tokenize warmup prompt");
            return false;
        }
        
        tokens.resize(n_tokens);
        llama_tokenize(
            vocab,
            warmup_prompt,
            strlen(warmup_prompt),
            tokens.data(),
            tokens.size(),
            true,
            true
        );
        
        // Create batch
        llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size());
        
        // Decode (process prompt)
        if (llama_decode(backend->ctx, batch) != 0) {
            luup_set_error(LUUP_ERROR_INFERENCE_FAILED, "Failed to decode warmup prompt");
            return false;
        }
        
        // Sample one token
        llama_token new_token = llama_sampler_sample(backend->sampler, backend->ctx, -1);
        
        // Note: KV cache will be cleared at start of next generation
        
        luup_clear_error();
        return true;
        
    } catch (const std::exception& e) {
        luup_set_error(LUUP_ERROR_INFERENCE_FAILED, e.what());
        return false;
    }
}

// Generate text (basic implementation)
char* llama_backend_generate(void* backend_data, const char* prompt, 
                             float temperature, int max_tokens) {
    if (!backend_data || !prompt) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "Invalid parameters");
        return nullptr;
    }
    
    auto backend = static_cast<llama_backend_data*>(backend_data);
    
    try {
        // Note: For simplicity, we create fresh context for each generation
        // In production, you'd want to manage KV cache more efficiently
        
        // Get vocab from model
        const llama_vocab* vocab = llama_model_get_vocab(backend->model);
        
        // Tokenize prompt
        std::vector<llama_token> tokens;
        int n_tokens = llama_tokenize(
            vocab,
            prompt,
            strlen(prompt),
            nullptr,
            0,
            true,
            true
        );
        
        if (n_tokens <= 0) {
            luup_set_error(LUUP_ERROR_INFERENCE_FAILED, "Failed to tokenize prompt");
            return nullptr;
        }
        
        tokens.resize(n_tokens);
        llama_tokenize(
            vocab,
            prompt,
            strlen(prompt),
            tokens.data(),
            tokens.size(),
            true,
            true
        );
        
        // Process prompt
        llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size());
        if (llama_decode(backend->ctx, batch) != 0) {
            luup_set_error(LUUP_ERROR_INFERENCE_FAILED, "Failed to decode prompt");
            return nullptr;
        }
        
        // Generate tokens
        std::string response;
        int n_generated = 0;
        int max_gen = max_tokens > 0 ? max_tokens : 512;
        
        while (n_generated < max_gen) {
            // Sample next token
            llama_token new_token = llama_sampler_sample(backend->sampler, backend->ctx, -1);
            
            // Check for EOS
            if (llama_vocab_is_eog(vocab, new_token)) {
                break;
            }
            
            // Decode token to text
            char buf[256];
            int n = llama_token_to_piece(vocab, new_token, buf, sizeof(buf), 0, true);
            if (n > 0) {
                response.append(buf, n);
            }
            
            // Prepare next batch with single token
            batch = llama_batch_get_one(&new_token, 1);
            if (llama_decode(backend->ctx, batch) != 0) {
                break;
            }
            
            n_generated++;
        }
        
        // Allocate and return result
        char* result = static_cast<char*>(malloc(response.size() + 1));
        if (result) {
            memcpy(result, response.c_str(), response.size());
            result[response.size()] = '\0';
        }
        
        luup_clear_error();
        return result;
        
    } catch (const std::exception& e) {
        luup_set_error(LUUP_ERROR_INFERENCE_FAILED, e.what());
        return nullptr;
    }
}

