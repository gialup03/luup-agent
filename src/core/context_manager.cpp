/**
 * @file context_manager.cpp
 * @brief Conversation history and context window management
 */

#include "../../include/luup_agent.h"
#include "internal.h"
#include <string>
#include <vector>
#include <sstream>

// Message structure
struct Message {
    std::string role;
    std::string content;
};

// Format conversation history into a prompt string
// Uses a simple chat template format
std::string format_chat_history(const std::vector<Message>& history) {
    std::ostringstream oss;
    
    for (const auto& msg : history) {
        if (msg.role == "system") {
            oss << "System: " << msg.content << "\n\n";
        } else if (msg.role == "user") {
            oss << "User: " << msg.content << "\n\n";
        } else if (msg.role == "assistant") {
            oss << "Assistant: " << msg.content << "\n\n";
        }
    }
    
    // Add the prompt for assistant to respond
    oss << "Assistant: ";
    
    return oss.str();
}

// Estimate token count (rough approximation: 1 token ≈ 4 characters)
size_t estimate_token_count(const std::string& text) {
    return text.size() / 4;
}

// Check if context window is getting full
bool is_context_full(const std::vector<Message>& history, size_t context_size, float threshold) {
    std::string formatted = format_chat_history(history);
    size_t estimated_tokens = estimate_token_count(formatted);
    return estimated_tokens >= (context_size * threshold);
}

// TODO: Future enhancements:
// - Proper tokenization using model's tokenizer
// - Automatic summarization when context fills
// - Sliding window strategies
// - Token budget management

