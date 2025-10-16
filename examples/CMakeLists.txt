# Examples CMakeLists.txt

# Basic chat example
add_executable(basic_chat basic_chat.cpp)
target_link_libraries(basic_chat PRIVATE luup_agent)

# Tool calling example
add_executable(tool_calling tool_calling.cpp)
target_link_libraries(tool_calling PRIVATE luup_agent)

# Built-in tools demo
add_executable(builtin_tools_demo builtin_tools_demo.cpp)
target_link_libraries(builtin_tools_demo PRIVATE luup_agent)

# Remote API demo
add_executable(remote_api_demo remote_api_demo.cpp)
target_link_libraries(remote_api_demo PRIVATE luup_agent)

# Install examples (optional)
install(TARGETS basic_chat tool_calling builtin_tools_demo remote_api_demo
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/examples
)

