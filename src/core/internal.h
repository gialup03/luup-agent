/**
 * @file internal.h
 * @brief Internal headers and utilities
 */

#ifndef LUUP_INTERNAL_H
#define LUUP_INTERNAL_H

#include "../../include/luup_agent.h"
#include <stddef.h>

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

#endif // LUUP_INTERNAL_H

