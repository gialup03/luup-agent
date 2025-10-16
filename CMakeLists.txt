cmake_minimum_required(VERSION 3.18)
project(luup-agent VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(LUUP_BUILD_TESTS "Build tests" ON)
option(LUUP_BUILD_EXAMPLES "Build examples" ON)
option(LUUP_BUILD_BINDINGS "Build language bindings" OFF)

# Platform detection and GPU backend configuration
if(APPLE)
    set(GGML_METAL ON CACHE BOOL "Enable Metal backend")
    message(STATUS "Platform: macOS - Enabling Metal backend")
elseif(WIN32 OR UNIX)
    # Try to find CUDA
    find_package(CUDAToolkit QUIET)
    if(CUDAToolkit_FOUND)
        set(GGML_CUDA ON CACHE BOOL "Enable CUDA backend")
        message(STATUS "CUDA found - Enabling CUDA backend")
    endif()
    
    # Try to find ROCm
    find_path(ROCM_PATH "bin/hipcc"
        HINTS
            $ENV{ROCM_PATH}
            /opt/rocm
    )
    if(ROCM_PATH)
        set(GGML_HIPBLAS ON CACHE BOOL "Enable ROCm backend")
        message(STATUS "ROCm found - Enabling ROCm backend")
    endif()
    
    # Vulkan as universal fallback
    find_package(Vulkan QUIET)
    if(Vulkan_FOUND)
        set(GGML_VULKAN ON CACHE BOOL "Enable Vulkan backend")
        message(STATUS "Vulkan found - Enabling Vulkan backend")
    endif()
endif()

# Dependencies
message(STATUS "Configuring dependencies...")

# Check for system-installed llama.cpp
find_package(llama CONFIG QUIET)
if(llama_FOUND)
    message(STATUS "Using system-installed llama.cpp")
else()
    message(STATUS "llama.cpp not found, using submodule/FetchContent")
    
    # Check if submodule exists
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/llama.cpp/CMakeLists.txt")
        message(STATUS "Using llama.cpp from submodule")
        add_subdirectory(external/llama.cpp)
    else()
        message(STATUS "Fetching llama.cpp from GitHub")
        include(FetchContent)
        FetchContent_Declare(
            llama
            GIT_REPOSITORY https://github.com/ggerganov/llama.cpp.git
            GIT_TAG master
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(llama)
    endif()
endif()

# Header-only dependencies via FetchContent
include(FetchContent)

# nlohmann/json
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

# cpp-httplib
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.14.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(httplib)

# Core library sources
set(LUUP_AGENT_SOURCES
    src/core/model.cpp
    src/core/agent.cpp
    src/core/tool_calling.cpp
    src/core/context_manager.cpp
    src/core/error_handling.cpp
    src/backends/local_llama.cpp
    src/backends/remote_api.cpp
    src/builtin_tools/todo_list.cpp
    src/builtin_tools/notes.cpp
    src/builtin_tools/summarization.cpp
    src/version.cpp
)

# Build shared library
add_library(luup_agent SHARED ${LUUP_AGENT_SOURCES})

target_include_directories(luup_agent
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(luup_agent
    PRIVATE
        llama
        httplib::httplib
        nlohmann_json::nlohmann_json
)

# Add pthread on Unix systems
if(UNIX)
    target_link_libraries(luup_agent PRIVATE pthread)
endif()

# Set library properties
set_target_properties(luup_agent PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER include/luup_agent.h
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(luup_agent PRIVATE LUUP_EXPORT)
    target_compile_definitions(luup_agent INTERFACE LUUP_IMPORT)
endif()

# Tests
if(LUUP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Examples
if(LUUP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Bindings
if(LUUP_BUILD_BINDINGS)
    add_subdirectory(bindings)
endif()

# Installation
include(GNUInstallDirs)

install(TARGETS luup_agent
    EXPORT luup-agent-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT luup-agent-targets
    FILE luup-agent-targets.cmake
    NAMESPACE luup::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/luup-agent
)

# Create package config
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "luup-agent-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/luup-agent-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/luup-agent
)

# Print configuration summary
message(STATUS "")
message(STATUS "luup-agent configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build tests: ${LUUP_BUILD_TESTS}")
message(STATUS "  Build examples: ${LUUP_BUILD_EXAMPLES}")
message(STATUS "  Build bindings: ${LUUP_BUILD_BINDINGS}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "")

