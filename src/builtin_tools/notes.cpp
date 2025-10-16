/**
 * @file notes.cpp
 * @brief Built-in notes tool implementation
 */

#include "../../include/luup_agent.h"
#include "../core/internal.h"
#include <nlohmann/json.hpp>
#include <string>
#include <fstream>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <algorithm>

using json = nlohmann::json;

extern void luup_set_error(luup_error_t code, const char* message);

// Storage structure for notes
struct NotesStorage {
    json data;
    std::string storage_path;
    int next_id;
    
    NotesStorage() : next_id(1) {
        data = json::object();
        data["notes"] = json::array();
    }
    
    bool load_from_file() {
        if (storage_path.empty()) {
            return false;
        }
        
        try {
            std::ifstream file(storage_path);
            if (file.is_open()) {
                file >> data;
                file.close();
                
                // Update next_id based on existing notes
                if (data.contains("notes") && data["notes"].is_array()) {
                    for (const auto& note : data["notes"]) {
                        if (note.contains("id") && note["id"].is_number()) {
                            int id = note["id"].get<int>();
                            if (id >= next_id) {
                                next_id = id + 1;
                            }
                        }
                    }
                }
                return true;
            }
        } catch (const std::exception& e) {
            // File doesn't exist or is invalid, start fresh
        }
        return false;
    }
    
    bool save_to_file() {
        if (storage_path.empty()) {
            return true; // Memory only mode
        }
        
        try {
            std::ofstream file(storage_path);
            if (file.is_open()) {
                file << data.dump(2);
                file.close();
                return true;
            }
        } catch (const std::exception& e) {
            return false;
        }
        return false;
    }
};

static std::string get_current_timestamp() {
    time_t now = time(nullptr);
    char buf[100];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
    return std::string(buf);
}

static char* notes_tool_callback(const char* params_json, void* user_data) {
    auto storage = static_cast<NotesStorage*>(user_data);
    
    try {
        json params = json::parse(params_json);
        std::string operation = params.value("operation", "list");
        
        if (operation == "create") {
            // Create new note
            std::string content = params.value("content", "");
            if (content.empty()) {
                json error;
                error["error"] = "Content is required";
                return strdup(error.dump().c_str());
            }
            
            json note;
            note["id"] = storage->next_id++;
            note["content"] = content;
            note["created"] = get_current_timestamp();
            
            // Handle tags
            if (params.contains("tags") && params["tags"].is_array()) {
                note["tags"] = params["tags"];
            } else {
                note["tags"] = json::array();
            }
            
            storage->data["notes"].push_back(note);
            storage->save_to_file();
            
            json result;
            result["success"] = true;
            result["message"] = "Note created successfully";
            result["note"] = note;
            return strdup(result.dump().c_str());
            
        } else if (operation == "read") {
            // Read specific note
            int id = params.value("id", 0);
            if (id == 0) {
                json error;
                error["error"] = "Note ID is required";
                return strdup(error.dump().c_str());
            }
            
            for (const auto& note : storage->data["notes"]) {
                if (note["id"] == id) {
                    json result;
                    result["note"] = note;
                    return strdup(result.dump().c_str());
                }
            }
            
            json error;
            error["error"] = "Note not found";
            return strdup(error.dump().c_str());
            
        } else if (operation == "update") {
            // Update existing note
            int id = params.value("id", 0);
            if (id == 0) {
                json error;
                error["error"] = "Note ID is required";
                return strdup(error.dump().c_str());
            }
            
            bool found = false;
            for (auto& note : storage->data["notes"]) {
                if (note["id"] == id) {
                    // Update content if provided
                    if (params.contains("content")) {
                        note["content"] = params["content"];
                    }
                    
                    // Update tags if provided
                    if (params.contains("tags") && params["tags"].is_array()) {
                        note["tags"] = params["tags"];
                    }
                    
                    note["modified"] = get_current_timestamp();
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                json error;
                error["error"] = "Note not found";
                return strdup(error.dump().c_str());
            }
            
            storage->save_to_file();
            
            json result;
            result["success"] = true;
            result["message"] = "Note updated successfully";
            return strdup(result.dump().c_str());
            
        } else if (operation == "delete") {
            // Delete note
            int id = params.value("id", 0);
            if (id == 0) {
                json error;
                error["error"] = "Note ID is required";
                return strdup(error.dump().c_str());
            }
            
            auto& notes = storage->data["notes"];
            bool found = false;
            for (size_t i = 0; i < notes.size(); i++) {
                if (notes[i]["id"] == id) {
                    notes.erase(notes.begin() + i);
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                json error;
                error["error"] = "Note not found";
                return strdup(error.dump().c_str());
            }
            
            storage->save_to_file();
            
            json result;
            result["success"] = true;
            result["message"] = "Note deleted successfully";
            return strdup(result.dump().c_str());
            
        } else if (operation == "search") {
            // Search notes by content or tags
            std::string query = params.value("query", "");
            json matching_notes = json::array();
            
            for (const auto& note : storage->data["notes"]) {
                bool matches = false;
                
                // Search in content
                std::string content = note.value("content", "");
                std::string content_lower = content;
                std::string query_lower = query;
                std::transform(content_lower.begin(), content_lower.end(), content_lower.begin(), ::tolower);
                std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);
                
                if (content_lower.find(query_lower) != std::string::npos) {
                    matches = true;
                }
                
                // Search in tags
                if (!matches && note.contains("tags") && note["tags"].is_array()) {
                    for (const auto& tag : note["tags"]) {
                        std::string tag_str = tag.get<std::string>();
                        std::string tag_lower = tag_str;
                        std::transform(tag_lower.begin(), tag_lower.end(), tag_lower.begin(), ::tolower);
                        
                        if (tag_lower.find(query_lower) != std::string::npos) {
                            matches = true;
                            break;
                        }
                    }
                }
                
                if (matches || query.empty()) {
                    matching_notes.push_back(note);
                }
            }
            
            json result;
            result["notes"] = matching_notes;
            result["count"] = matching_notes.size();
            return strdup(result.dump().c_str());
            
        } else if (operation == "list") {
            // List all notes (same as search with empty query)
            json result;
            result["notes"] = storage->data["notes"];
            result["count"] = storage->data["notes"].size();
            return strdup(result.dump().c_str());
            
        } else {
            json error;
            error["error"] = "Unknown operation: " + operation;
            return strdup(error.dump().c_str());
        }
        
    } catch (const std::exception& e) {
        json error;
        error["error"] = std::string("Notes tool error: ") + e.what();
        return strdup(error.dump().c_str());
    }
}

extern "C" {

luup_error_t luup_agent_enable_builtin_notes(
    luup_agent* agent,
    const char* storage_path)
{
    if (!agent) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "Invalid agent handle");
        return LUUP_ERROR_INVALID_PARAM;
    }
    
    try {
        // Create storage
        auto storage = new NotesStorage();
        if (storage_path) {
            storage->storage_path = storage_path;
            storage->load_from_file();
        }
        
        // Define tool
        luup_tool tool;
        tool.name = "notes";
        tool.description = "Manage notes: create, read, update, delete, or search notes with tags";
        tool.parameters_json = 
            "{"
            "  \"type\": \"object\","
            "  \"properties\": {"
            "    \"operation\": {"
            "      \"type\": \"string\","
            "      \"enum\": [\"create\", \"read\", \"update\", \"delete\", \"search\", \"list\"],"
            "      \"description\": \"Operation to perform\""
            "    },"
            "    \"content\": {"
            "      \"type\": \"string\","
            "      \"description\": \"Note content (required for 'create', optional for 'update')\""
            "    },"
            "    \"id\": {"
            "      \"type\": \"number\","
            "      \"description\": \"Note ID (required for 'read', 'update', 'delete')\""
            "    },"
            "    \"tags\": {"
            "      \"type\": \"array\","
            "      \"items\": {\"type\": \"string\"},"
            "      \"description\": \"Tags for the note (optional)\""
            "    },"
            "    \"query\": {"
            "      \"type\": \"string\","
            "      \"description\": \"Search query for 'search' operation\""
            "    }"
            "  },"
            "  \"required\": [\"operation\"]"
            "}";
        
        // Register tool
        luup_error_t result = luup_agent_register_tool(
            agent,
            &tool,
            notes_tool_callback,
            storage
        );
        
        if (result != LUUP_SUCCESS) {
            delete storage;
            return result;
        }
        
        return LUUP_SUCCESS;
        
    } catch (const std::exception& e) {
        luup_set_error(LUUP_ERROR_OUT_OF_MEMORY, e.what());
        return LUUP_ERROR_OUT_OF_MEMORY;
    }
}

} // extern "C"
