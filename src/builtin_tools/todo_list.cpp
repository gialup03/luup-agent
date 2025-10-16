/**
 * @file todo_list.cpp
 * @brief Built-in todo list tool implementation
 */

#include "../../include/luup_agent.h"
#include "../core/internal.h"
#include <nlohmann/json.hpp>
#include <string>
#include <fstream>
#include <ctime>
#include <cstdlib>
#include <cstring>

using json = nlohmann::json;

extern void luup_set_error(luup_error_t code, const char* message);

// Storage structure for todo list
struct TodoListStorage {
    json data;
    std::string storage_path;
    int next_id;
    
    TodoListStorage() : next_id(1) {
        data = json::object();
        data["todos"] = json::array();
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
                
                // Update next_id based on existing todos
                if (data.contains("todos") && data["todos"].is_array()) {
                    for (const auto& todo : data["todos"]) {
                        if (todo.contains("id") && todo["id"].is_number()) {
                            int id = todo["id"].get<int>();
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

static char* todo_tool_callback(const char* params_json, void* user_data) {
    auto storage = static_cast<TodoListStorage*>(user_data);
    
    try {
        json params = json::parse(params_json);
        std::string operation = params.value("operation", "list");
        
        if (operation == "add") {
            // Add new todo
            std::string title = params.value("title", "");
            if (title.empty()) {
                json error;
                error["error"] = "Title is required";
                return strdup(error.dump().c_str());
            }
            
            json todo;
            todo["id"] = storage->next_id++;
            todo["title"] = title;
            todo["status"] = "pending";
            todo["created"] = get_current_timestamp();
            
            storage->data["todos"].push_back(todo);
            storage->save_to_file();
            
            json result;
            result["success"] = true;
            result["message"] = "Todo added successfully";
            result["todo"] = todo;
            return strdup(result.dump().c_str());
            
        } else if (operation == "list") {
            // List all todos
            json result;
            result["todos"] = storage->data["todos"];
            return strdup(result.dump().c_str());
            
        } else if (operation == "complete") {
            // Mark todo as complete
            int id = params.value("id", 0);
            if (id == 0) {
                json error;
                error["error"] = "Todo ID is required";
                return strdup(error.dump().c_str());
            }
            
            bool found = false;
            for (auto& todo : storage->data["todos"]) {
                if (todo["id"] == id) {
                    todo["status"] = "completed";
                    todo["completed"] = get_current_timestamp();
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                json error;
                error["error"] = "Todo not found";
                return strdup(error.dump().c_str());
            }
            
            storage->save_to_file();
            
            json result;
            result["success"] = true;
            result["message"] = "Todo marked as completed";
            return strdup(result.dump().c_str());
            
        } else if (operation == "delete") {
            // Delete todo
            int id = params.value("id", 0);
            if (id == 0) {
                json error;
                error["error"] = "Todo ID is required";
                return strdup(error.dump().c_str());
            }
            
            auto& todos = storage->data["todos"];
            bool found = false;
            for (size_t i = 0; i < todos.size(); i++) {
                if (todos[i]["id"] == id) {
                    todos.erase(todos.begin() + i);
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                json error;
                error["error"] = "Todo not found";
                return strdup(error.dump().c_str());
            }
            
            storage->save_to_file();
            
            json result;
            result["success"] = true;
            result["message"] = "Todo deleted successfully";
            return strdup(result.dump().c_str());
            
        } else {
            json error;
            error["error"] = "Unknown operation: " + operation;
            return strdup(error.dump().c_str());
        }
        
    } catch (const std::exception& e) {
        json error;
        error["error"] = std::string("Todo tool error: ") + e.what();
        return strdup(error.dump().c_str());
    }
}

extern "C" {

luup_error_t luup_agent_enable_builtin_todo(
    luup_agent* agent,
    const char* storage_path)
{
    if (!agent) {
        luup_set_error(LUUP_ERROR_INVALID_PARAM, "Invalid agent handle");
        return LUUP_ERROR_INVALID_PARAM;
    }
    
    try {
        // Create storage
        auto storage = new TodoListStorage();
        if (storage_path) {
            storage->storage_path = storage_path;
            storage->load_from_file();
        }
        
        // Define tool
        luup_tool tool;
        tool.name = "todo";
        tool.description = "Manage todo list: add, list, complete, or delete tasks";
        tool.parameters_json = 
            "{"
            "  \"type\": \"object\","
            "  \"properties\": {"
            "    \"operation\": {"
            "      \"type\": \"string\","
            "      \"enum\": [\"add\", \"list\", \"complete\", \"delete\"],"
            "      \"description\": \"Operation to perform\""
            "    },"
            "    \"title\": {"
            "      \"type\": \"string\","
            "      \"description\": \"Todo title (required for 'add')\""
            "    },"
            "    \"id\": {"
            "      \"type\": \"number\","
            "      \"description\": \"Todo ID (required for 'complete' and 'delete')\""
            "    }"
            "  },"
            "  \"required\": [\"operation\"]"
            "}";
        
        // Register tool
        luup_error_t result = luup_agent_register_tool(
            agent,
            &tool,
            todo_tool_callback,
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
