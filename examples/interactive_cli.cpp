/**
 * @file interactive_cli.cpp
 * @brief Simple interactive CLI for ad-hoc testing with small models
 * 
 * Quick and easy CLI for testing agent behavior with toy models.
 * Features:
 * - Interactive chat loop
 * - Simple commands (/help, /clear, /history, /quit)
 * - Optional tool calling support
 * - Streaming output
 */

#include <luup_agent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <string>

// Global agent for signal handling
luup_agent* g_agent = nullptr;

// Streaming callback
void stream_callback(const char* token, void* user_data) {
    printf("%s", token);
    fflush(stdout);
}

// Simple calculator tool for testing
char* calculator_tool(const char* params_json, void* user_data) {
    // Extract expression from JSON string (simple string parsing)
    const char* expr_start = strstr(params_json, "expression");
    if (!expr_start) {
        return strdup("{\"error\": \"No expression found\"}");
    }
    
    // Find the value after "expression": "
    const char* value_start = strchr(expr_start, '"');
    if (value_start) value_start = strchr(value_start + 1, '"');
    if (value_start) value_start++;
    
    const char* value_end = value_start ? strchr(value_start, '"') : nullptr;
    
    if (!value_start || !value_end) {
        return strdup("{\"error\": \"Failed to parse expression\"}");
    }
    
    char expression[256];
    size_t expr_len = value_end - value_start;
    if (expr_len >= sizeof(expression)) expr_len = sizeof(expression) - 1;
    strncpy(expression, value_start, expr_len);
    expression[expr_len] = '\0';
    
    // Parse and evaluate basic arithmetic
    double a = 0, b = 0;
    char op = 0;
    double result = 0;
    
    if (sscanf(expression, "%lf %c %lf", &a, &op, &b) == 3 ||
        sscanf(expression, "%lf%c%lf", &a, &op, &b) == 3) {
        switch (op) {
            case '+': result = a + b; break;
            case '-': result = a - b; break;
            case '*': case 'x': case 'X': result = a * b; break;
            case '/': result = (b != 0) ? a / b : 0; break;
            default: result = 0;
        }
    }
    
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "{\"result\": %.2f, \"expression\": \"%s\"}", result, expression);
    return strdup(buffer);
}

// Simple time tool for testing
char* time_tool(const char* params_json, void* user_data) {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    char time_str[100];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S %Z", timeinfo);
    
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "{\"time\": \"%s\", \"timestamp\": %ld}", 
             time_str, (long)now);
    return strdup(buffer);
}

void print_help() {
    printf("\nInteractive CLI Commands:\n");
    printf("  /help       - Show this help message\n");
    printf("  /clear      - Clear conversation history\n");
    printf("  /history    - Show conversation history (JSON)\n");
    printf("  /quit       - Exit the program\n");
    printf("  /exit       - Exit the program\n");
    printf("\nOr just type your message to chat with the agent.\n\n");
}

int main(int argc, char** argv) {
    // Suppress verbose llama.cpp logs (redirect stderr to /dev/null)
    freopen("/dev/null", "w", stderr);
    
    printf("====================================\n");
    printf("  luup-agent Interactive CLI\n");
    printf("  Version: %s\n", luup_version());
    printf("====================================\n\n");
    
    // Parse arguments
    if (argc < 2) {
        printf("Usage: %s <model-path> [options]\n", argv[0]);
        printf("\nOptions:\n");
        printf("  --no-tools       Disable tool calling\n");
        printf("  --temp <value>   Set temperature (default: 0.7)\n");
        printf("  --ctx <size>     Set context size (default: 2048)\n");
        printf("\nExamples:\n");
        printf("  %s models/qwen-0.5b.gguf\n", argv[0]);
        printf("  %s models/tiny-llama.gguf --temp 0.9\n", argv[0]);
        printf("  %s models/phi-2.gguf --no-tools --ctx 4096\n", argv[0]);
        return 1;
    }
    
    const char* model_path = argv[1];
    bool enable_tools = true;
    float temperature = 0.7f;
    int context_size = 2048;
    
    // Parse options
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--no-tools") == 0) {
            enable_tools = false;
        } else if (strcmp(argv[i], "--temp") == 0 && i + 1 < argc) {
            temperature = atof(argv[++i]);
        } else if (strcmp(argv[i], "--ctx") == 0 && i + 1 < argc) {
            context_size = atoi(argv[++i]);
        }
    }
    
    // Create model
    printf("Loading model: %s\n", model_path);
    printf("  Context size: %d\n", context_size);
    printf("  Temperature: %.2f\n", temperature);
    printf("  Tools: %s\n", enable_tools ? "enabled" : "disabled");
    printf("\n");
    
    luup_model_config model_config = {
        .path = model_path,
        .gpu_layers = -1,  // Use all available GPU
        .context_size = context_size,
        .threads = 0,  // Auto-detect
        .api_key = nullptr,
        .api_base_url = nullptr
    };
    
    luup_model* model = luup_model_create_local(&model_config);
    
    if (!model) {
        printf("❌ Error creating model: %s\n", luup_get_last_error());
        return 1;
    }
    
    // Show model info
    luup_model_info info;
    if (luup_model_get_info(model, &info) == LUUP_SUCCESS) {
        printf("Model loaded successfully!\n");
        printf("  Backend: %s\n", info.backend);
        printf("  Device: %s\n", info.device);
        printf("  GPU layers: %d\n", info.gpu_layers_loaded);
    }
    
    // Warmup
    printf("\nWarming up model...");
    fflush(stdout);
    if (luup_model_warmup(model) == LUUP_SUCCESS) {
        printf(" done!\n");
    } else {
        printf(" warning: %s\n", luup_get_last_error());
    }
    
    // Create agent
    luup_agent_config agent_config = {
        .model = model,
        .system_prompt = "You are a helpful AI assistant. Always respond in English. Be concise and friendly.",
        .temperature = temperature,
        .max_tokens = 512,
        .enable_tool_calling = enable_tools,
        .enable_history_management = true
    };
    
    g_agent = luup_agent_create(&agent_config);
    if (!g_agent) {
        printf("❌ Error creating agent: %s\n", luup_get_last_error());
        luup_model_destroy(model);
        return 1;
    }
    
    // Register tools if enabled
    if (enable_tools) {
        printf("\nRegistering tools...\n");
        
        // Calculator tool
        luup_tool calc_tool = {
            .name = "calculate",
            .description = "Perform basic mathematical calculations",
            .parameters_json =
                "{"
                "  \"type\": \"object\","
                "  \"properties\": {"
                "    \"expression\": {"
                "      \"type\": \"string\","
                "      \"description\": \"Math expression to evaluate\""
                "    }"
                "  },"
                "  \"required\": [\"expression\"]"
                "}"
        };
        
        if (luup_agent_register_tool(g_agent, &calc_tool, calculator_tool, nullptr) == LUUP_SUCCESS) {
            printf("  ✓ calculator\n");
        }
        
        // Time tool
        luup_tool time_tool_def = {
            .name = "get_time",
            .description = "Get the current date and time",
            .parameters_json = "{\"type\": \"object\", \"properties\": {}}"
        };
        
        if (luup_agent_register_tool(g_agent, &time_tool_def, time_tool, nullptr) == LUUP_SUCCESS) {
            printf("  ✓ get_time\n");
        }
    }
    
    printf("\n====================================\n");
    printf("Ready! Type your message or /help for commands.\n");
    printf("====================================\n\n");
    
    // Main chat loop
    std::string input;
    while (true) {
        printf("You: ");
        fflush(stdout);
        
        if (!std::getline(std::cin, input)) {
            break;
        }
        
        // Trim whitespace
        while (!input.empty() && isspace(input.back())) {
            input.pop_back();
        }
        while (!input.empty() && isspace(input.front())) {
            input.erase(0, 1);
        }
        
        // Skip empty input
        if (input.empty()) {
            continue;
        }
        
        // Handle commands
        if (input[0] == '/') {
            if (input == "/quit" || input == "/exit") {
                break;
            } else if (input == "/help") {
                print_help();
                continue;
            } else if (input == "/clear") {
                if (luup_agent_clear_history(g_agent) == LUUP_SUCCESS) {
                    printf("✓ History cleared\n\n");
                } else {
                    printf("❌ Error clearing history: %s\n\n", luup_get_last_error());
                }
                continue;
            } else if (input == "/history") {
                char* history = luup_agent_get_history_json(g_agent);
                if (history) {
                    printf("\n%s\n\n", history);
                    luup_free_string(history);
                } else {
                    printf("❌ Error getting history: %s\n\n", luup_get_last_error());
                }
                continue;
            } else {
                printf("Unknown command. Type /help for available commands.\n\n");
                continue;
            }
        }
        
        // Generate response with streaming
        printf("Assistant: ");
        fflush(stdout);
        
        luup_error_t err = luup_agent_generate_stream(
            g_agent,
            input.c_str(),
            stream_callback,
            nullptr
        );
        
        if (err != LUUP_SUCCESS) {
            printf("\n❌ Error: %s\n", luup_get_last_error());
        }
        
        printf("\n\n");
    }
    
    printf("\n👋 Goodbye!\n");
    
    // Cleanup
    luup_agent_destroy(g_agent);
    luup_model_destroy(model);
    
    return 0;
}

