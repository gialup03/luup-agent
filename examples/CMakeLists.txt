# Examples CMakeLists.txt

# Basic chat example
add_executable(basic_chat basic_chat.cpp)
target_link_libraries(basic_chat PRIVATE luup_agent)

# Tool calling example
add_executable(tool_calling tool_calling.cpp)
target_link_libraries(tool_calling PRIVATE luup_agent)

# Install examples (optional)
install(TARGETS basic_chat tool_calling
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/examples
)

